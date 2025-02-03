import sys
import random
from PyQt5 import QtCore, QtWidgets
from eggsIncubatorGUI import Ui_MainWindow  # Import the UI class directly

import serial
import re
import os
from datetime import datetime, timedelta
import csv
import time
import queue

# ARDUINO serial communication - setup #
portSetup = "/dev/ttyACM0"
baudrateSetup = 19200
timeout = 0.1

identifiers = ["TMP", "HUM", "HTP"]  # Global variable


class SerialThread(QtCore.QThread):
    data_received = QtCore.pyqtSignal(list)

    def __init__(self, port = portSetup, baudrate = baudrateSetup):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.command_queue = queue.Queue()  # Queue for outgoing commands
        self.retry_interval = 1 #sleeping time in seconds
        self.max_retries = 5
        self.running = True
        self.serial_port = None  # To store the serial port object

    def open_serial_port(self):
        retries = 0
        while retries < self.max_retries:
            try:
                self.serial_port = serial.Serial(self.port, self.baudrate, timeout=1)
                print("Serial port opened successfully")
                # Discard any initial data to avoid decode errors
                self.serial_port.reset_input_buffer()
                return True
            except serial.SerialException as e:
                print(f"Failed to open serial port: {e}. Retrying in {self.retry_interval} seconds...")
                time.sleep(self.retry_interval)
                retries += 1
        return False
    
    def run(self):
        if not self.open_serial_port():
            print("Unable to open serial port after multiple attempts.")
            return

        buffer = ''
        startRx = False
        saving = False
        identifiers_data_list = []
        
        try: 
            while self.running:
                # Reading serial data
                if self.serial_port.in_waiting > 0:
                    dataRead = self.serial_port.read().decode('utf-8')
                    if dataRead == '@':
                        startRx = True
                    if startRx:
                        if dataRead == '<':
                            saving = True
                        if saving:
                            buffer += dataRead
                        if dataRead == '>':
                            saving = False
                            self.decode_message(buffer, identifiers_data_list)
                            buffer = ''
                        if dataRead == '#':
                            startRx = False
                            if identifiers_data_list:
                                self.data_received.emit(identifiers_data_list)
                                identifiers_data_list.clear()
                
                # Sending serial data
                while not self.command_queue.empty():
                    command = self.command_queue.get()
                    self.serial_port.write(command.encode('utf-8'))
                    print(f"Sent to Arduino: {command}")
                        
        except serial.SerialException as e:
            print(f"Failed to open serial port: {e}")
        finally:
            self.close_serial_port()
            
    def add_command(self, cmd, value):
        formatted_command = f"@<{cmd}, {value}>#"
        self.command_queue.put(formatted_command)

    def stop(self):
        self.running = False
        self.wait()  # Ensure the thread finishes before returning
        self.close_serial_port()
        
    def close_serial_port(self):
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.close()
                print("Serial port closed successfully")
            except Exception as e:
                print(f"Error closing serial port: {e}")

    @staticmethod
    def decode_message(buffer, identifiers_data_list):
        match = re.match(r"<([^,]+),([^>]+)>", buffer)
        if match:
            infoName_part = match.group(1)
            infoData_part = match.group(2)

            if SerialThread.is_number(infoData_part):
                number = float(infoData_part) if '.' in infoData_part else int(infoData_part)
                for identifier in identifiers:
                    if infoName_part.startswith(identifier):
                        identifiers_data_list.append({infoName_part: number})
                        break
            else:
                print(f"Non-numeric data: {infoData_part}")
        else:
            print("The input string does not match the expected format.")

    @staticmethod
    def is_number(s):
        try:
            float(s)
            return True
        except ValueError:
            return False


class MainSoftwareThread(QtCore.QThread):
    update_view = QtCore.pyqtSignal(list)  # Signal to update the view (MainWindow)

    def __init__(self):
        super().__init__()
        self.running = True
        self.serial_thread = SerialThread()
        self.serial_thread.data_received.connect(self.process_serial_data)
        self.current_data = []  # Holds the most recent data received from SerialThread
        
        self.saving_interval = 1 # default: every minute
        self.last_saving_time = datetime.now()
        
        self.command_list = [] # List to hold the pending commands
        
        # State variables to handle inputs from MainWindow
        self.current_button = None
        self.current_speedRPM_spinBox_value = None
        self.current_maxHysteresisValue_spinBox_value = None
        self.current_minHysteresisValue_spinBox_value = None
        
        #--- Create Machine_Statistic folder ---#
        machine_statistics_folder_path = "Machine_Statistics"
        if not os.path.exists(machine_statistics_folder_path):
                os.makedirs(machine_statistics_folder_path)

        # --- Create Images folder ---#
        temperatures_folder_path = os.path.join(machine_statistics_folder_path, 'Temperatures')
        if not os.path.exists(temperatures_folder_path):
            os.makedirs(temperatures_folder_path)

        humidity_folder_path = os.path.join(machine_statistics_folder_path, 'Humidity')
        if not os.path.exists(humidity_folder_path):
            os.makedirs(humidity_folder_path)

    def run(self):
        # Start the SerialThread
        self.serial_thread.start()
        
        
        while self.running:
            if self.command_list:
                for cmd, value in self.command_list:
                    self.serial_thread.add_command(cmd, value)
                    print(f"{cmd}, {value}")
                self.command_list.clear()
                
            # Example of using the button and spinbox values in the main loop
            if self.current_button is not None:
                print(f"Processing button: {self.current_button}")
                
                if self.current_button == "move_CW_btn":
                    self.queue_command("CMD01", 1)
                if self.current_button == "move_CCW_btn":
                    self.queue_command("CMD02", 0)
                if self.current_button == "reset_btn":
                    self.queue_command("CMD03", 0)
                    
                self.current_button = None  # Reset after processing

            if self.current_speedRPM_spinBox_value is not None:
                print(f"[MainSoftwareThread] Processing spinbox value: {self.current_speedRPM_spinBox_value} ({type(self.current_speedRPM_spinBox_value)})")
                self.current_speedRPM_spinBox_value = None  # Reset after processing

            if self.current_maxHysteresisValue_spinBox_value is not None:
                print(f"[MainSoftwareThread] Processing spinbox value: {self.current_maxHysteresisValue_spinBox_value} ({type(self.current_maxHysteresisValue_spinBox_value)})")
                self.current_maxHysteresisValue_spinBox_value = None  # Reset after processing
                
            if self.current_minHysteresisValue_spinBox_value is not None:
                print(f"[MainSoftwareThread] Processing spinbox value: {self.current_minHysteresisValue_spinBox_value} ({type(self.current_minHysteresisValue_spinBox_value)})")
                self.current_minHysteresisValue_spinBox_value = None  # Reset after processing
                
            self.msleep(100)
            
    def queue_command(self, cmd, value):
        self.command_list.append((cmd, value))
        print(self.command_list)

    def stop(self):
        self.running = False
        self.serial_thread.stop()
        self.serial_thread.wait()
        
    def handle_button_click(self, button_name):
        """Handle button click signals from MainWindow."""
        print(f"Received button click: {button_name}")
        self.current_button = button_name

    def handle_speedRPM_spinBox_value(self, spinbox_name, value):
        """Handle spinbox value change signals from MainWindow."""
        #print(f"[MainSoftwareThread] {spinbox_name} value changed to {value}")
        self.current_speedRPM_spinBox_value = value
        
    def handle_maxHysteresisValue_spinBox_value(self, spinbox_name, value):
        """Handle spinbox value change signals from MainWindow."""
        #print(f"[MainSoftwareThread] {spinbox_name} value changed to {value}")
        self.current_maxHysteresisValue_spinBox_value = value
        
    def handle_minHysteresisValue_spinBox_value(self, spinbox_name, value):
        """Handle spinbox value change signals from MainWindow."""
        #print(f"[MainSoftwareThread] {spinbox_name} value changed to {value}")
        self.current_minHysteresisValue_spinBox_value = value

    def process_serial_data(self, new_data):
        self.current_data = new_data
        # print(new_data)
        # Extract data into specific categories
        current_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("TMP")} # dictionary: <class 'dict'> dict_values([23.7, 21.1, 20.4, 21.7]) 
        current_humidities = {k: v for d in new_data for k, v in d.items() if k.startswith("HUM")}
        current_humidities_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("HTP")}
        # Emit the data to update the view
        # OLD self.update_view.emit(list(current_temperatures.values()))
        
        # Collecting all values into a single list
        all_values = list(current_temperatures.values()) + \
                    list(current_humidities.values()) + \
                    list(current_humidities_temperatures.values())
        self.update_view.emit(all_values)
             
        
        # SAVING DATA IN FILES
        if ((datetime.now() - self.last_saving_time) >= timedelta(minutes = self.saving_interval)):
                start_time = time.perf_counter()
                
                self.save_data_to_files('Temperatures', current_temperatures) #{'TMP01': 23.1, 'TMP02': 23.1, 'TMP03': 23.1}
                self.save_data_to_files('Humidity', current_humidities) #{'HUM01': 52.5}
                self.last_saving_time = datetime.now()
                print(f"Saved data! {self.last_saving_time}")
                
                end_time = time.perf_counter()
                print(f"Time requested for saving data [milli-seconds]: {(end_time - start_time)*1000}")
                
        
    def save_data_to_files(self, data_type, data_dictionary): #passo un dictionary di temperature/humidities, dimensione variabile per gestire più o meno sensori dinamicamente
        now = datetime.now()
        current_date = now.strftime('%Y-%m-%d')
        
        machine_statistics_folder_path = "Machine_Statistics"
        # faccio selezione del folder da cui pescare i dati.
        if data_type == 'Temperatures':
            folder_path = os.path.join(machine_statistics_folder_path, 'Temperatures')
        elif data_type == 'Humidity':
            folder_path = os.path.join(machine_statistics_folder_path, 'Humidity')
        else:
            raise ValueError("Invalid path configuration")	
            
        file_path = os.path.join(folder_path, f"{current_date}.csv")

        # Initialize the CSV file with headers if it doesn't exist
        if not os.path.exists(file_path):
            with open(file_path, mode='w', newline='') as file: # write mode
                writer = csv.writer(file)

                # Initialize the list with 'Timestamp' as the first element
                result_list = ['Timestamp']

                # Append the keys from the dictionary to the list
                result_list.extend(data_dictionary.keys())

                writer.writerow(result_list)

        timestamp = now.strftime('%Y-%m-%d %H:%M:%S')

        with open(file_path, mode='a', newline='') as file: # append mode
            writer = csv.writer(file)

            # Initialize the list with the timestamp as the first element
            values_list = [timestamp]

            # Append the values from the dictionary to the list
            values_list.extend(data_dictionary.values())
            writer.writerow(values_list)


class MainWindow(QtWidgets.QMainWindow):
    # Define custom signals - this is done to send button/spinBox and other custom signals to other thread MainSoftwareThread: use Qt Signals
    button_clicked = QtCore.pyqtSignal(str)  # Emits button name
    speedRPM_spinBox_value_changed = QtCore.pyqtSignal(str, float)  # Emits spinbox value  // METTI INT se intero
    maxHysteresisValue_spinBox_value_changed = QtCore.pyqtSignal(str, float) 
    minHysteresisValue_spinBox_value_changed = QtCore.pyqtSignal(str, float) 
    
    def __init__(self, main_software_thread):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.main_software_thread = main_software_thread
        self.main_software_thread.update_view.connect(self.update_display_data)
        
        # Connect signals to main software thread slots
        self.button_clicked.connect(self.main_software_thread.handle_button_click)
        self.speedRPM_spinBox_value_changed.connect(self.main_software_thread.handle_speedRPM_spinBox_value)
        self.maxHysteresisValue_spinBox_value_changed.connect(self.main_software_thread.handle_maxHysteresisValue_spinBox_value)
        self.minHysteresisValue_spinBox_value_changed.connect(self.main_software_thread.handle_minHysteresisValue_spinBox_value)


        # Connect buttons to handlers that emit signals
        self.ui.move_CW_btn.clicked.connect(lambda: self.emit_button_signal("move_CW_btn"))
        self.ui.move_CCW_btn.clicked.connect(lambda: self.emit_button_signal("move_CCW_btn"))
        self.ui.layHorizontal_btn.clicked.connect(lambda: self.emit_button_signal("layHorizontal_btn"))
        self.ui.forceEggsTurn_btn.clicked.connect(lambda: self.emit_button_signal("forceEggsTurn_btn"))
        self.ui.reset_btn.clicked.connect(lambda: self.emit_button_signal("reset_btn"))

        # Connect radio buttons to emit its values
        self.ui.maxValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.meanValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.minValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        
        # Connect spinBox to emit its values
        self.ui.speedRPM_spinBox.valueChanged.connect(lambda value: self.emit_spinbox_signal("speedRPM_spinBox", value))
        self.ui.maxHysteresisValue_spinBox.valueChanged.connect(lambda value: self.emit_spinbox_signal("maxHysteresisValue_spinBox", value))
        self.ui.minHysteresisValue_spinBox.valueChanged.connect(lambda value: self.emit_spinbox_signal("minHysteresisValue_spinBox", value))
        
    def emit_button_signal(self, button_name):
        """Emit signal with the name of the button clicked."""
        self.button_clicked.emit(button_name)
        print(f"Button clicked: {button_name}")

    def emit_spinbox_signal(self, spinbox_name, value):
        """Emit signal with the name of the spinbox and its value."""
        value = float(value)  # Cast value to float explicitly
        #print(f"[MainWindow] Emitting signal from {spinbox_name} with value: {value}")
        if spinbox_name == "speedRPM_spinBox":
            self.speedRPM_spinBox_value_changed.emit(spinbox_name, value)
        elif spinbox_name == "maxHysteresisValue_spinBox":
            self.maxHysteresisValue_spinBox_value_changed.emit(spinbox_name, value)
        elif spinbox_name == "minHysteresisValue_spinBox":
            self.minHysteresisValue_spinBox_value_changed.emit(spinbox_name, value)

    def update_display_data(self, all_data):
        # Update the temperature labels in the GUI
        if len(all_data) >= 6: # perché il numero??
            self.ui.temperature1_4.setText(f"{all_data[0]} °C")
            self.ui.temperature2_4.setText(f"{all_data[1]} °C")
            self.ui.temperature3_5.setText(f"{all_data[2]} °C")
            self.ui.temperature4_2.setText(f"{all_data[3]} °C")
            self.ui.humidity1.setText(f"{all_data[4]} %")
            #self.ui.temperature4_2.setText(f"{all_data[3]} °C") PER TEMPERATURA DA UMIDITA

    def handle_radio_button(self):
        sender = self.sender()
        if sender.isChecked():
            print(f"Radio button '{sender.text()}' selected")

    def handle_speedRPM_spinBox(self):
        value = self.ui.speedRPM_spinBox.value()
        print(f"speedRPM_spinBox value changed to: {value}")

    def handle_maxHysteresisValue_spinBox(self):
        value = self.ui.maxHysteresisValue_spinBox.value()
        print(f"maxHysteresisValue_spinBox value changed to: {value}")
        
    def handle_minHysteresisValue_spinBox(self):
        value = self.ui.minHysteresisValue_spinBox.value()
        print(f"minHysteresisValue_spinBox value changed to: {value}")

    def handle_move_cw(self):
        print("Move clockwise button clicked")

    def handle_move_ccw(self):
        print("Move counter-clockwise button clicked")

    def handle_reset(self):
        print("Reset button clicked")

    def closeEvent(self, event):
        # Ensure the threads are stopped when the window is closed
        self.main_software_thread.stop()
        self.main_software_thread.wait()
        super().closeEvent(event)


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)

    # Initialize the main software thread
    main_software_thread = MainSoftwareThread()

    # Initialize and show the main window
    window = MainWindow(main_software_thread)
    window.show()

    # Start the main software thread
    main_software_thread.start()

    sys.exit(app.exec_())
