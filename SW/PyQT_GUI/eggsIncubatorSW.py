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

# ARDUINO serial communication - setup #
portSetup = "/dev/ttyACM0"
baudrateSetup = 19200
timeout = 0.1

identifiers = ["TMP", "HUM", "HTP"]  # Global variable


class SerialThread(QtCore.QThread):
    data_received = QtCore.pyqtSignal(list)

    def __init__(self, port=portSetup, baudrate=baudrateSetup):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = True
        self.serial_port = None  # To store the serial port object

    def run(self):
        try:
            self.serial_port = serial.Serial(self.port, self.baudrate, timeout=1)
            print("Serial port opened successfully")
            buffer = ''
            startTransmission = False
            saving = False
            identifiers_data_list = []

            while self.running:
                if self.serial_port.in_waiting > 0:
                    dataRead = self.serial_port.read().decode('utf-8')
                    if dataRead == '@':
                        startTransmission = True
                    if startTransmission:
                        if dataRead == '<':
                            saving = True
                        if saving:
                            buffer += dataRead
                        if dataRead == '>':
                            saving = False
                            self.decode_message(buffer, identifiers_data_list)
                            buffer = ''
                        if dataRead == '#':
                            startTransmission = False
                            if identifiers_data_list:
                                self.data_received.emit(identifiers_data_list)
                                identifiers_data_list.clear()
        except serial.SerialException as e:
            print(f"Failed to open serial port: {e}")
        finally:
            self.close_serial_port()

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

    def stop(self):
        self.running = False
        self.serial_thread.stop()
        self.serial_thread.wait()

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
    def __init__(self, main_software_thread):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.main_software_thread = main_software_thread
        self.main_software_thread.update_view.connect(self.update_display_data)

        # Connect buttons to example handlers
        self.ui.move_CW_btn.clicked.connect(self.handle_move_cw)
        self.ui.move_CCW_btn.clicked.connect(self.handle_move_ccw)
        self.ui.reset_btn.clicked.connect(self.handle_reset)

        # Connect radio buttons and spinBox to handlers
        self.ui.maxValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.meanValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.minValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.speedRPM_spinBox.valueChanged.connect(self.handle_spinbox)

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

    def handle_spinbox(self):
        value = self.ui.speedRPM_spinBox.value()
        print(f"SpinBox value changed to: {value}")

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
