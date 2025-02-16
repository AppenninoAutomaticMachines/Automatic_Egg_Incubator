import sys
import random
from PyQt5 import QtCore, QtWidgets
from eggsIncubatorGUI import Ui_MainWindow  # Import the UI class directly

import serial
import re
import os
from datetime import datetime, timedeltaimport sys
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
import subprocess

'''
    @<HTR01, True>#   heater 
    @<HUMER01, True># humidifier
'''

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
                print("Estabilishing serial connection....")
                self.serial_port = serial.Serial(self.port, self.baudrate, timeout=1)
                time.sleep(3)
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
    update_spinbox_value = QtCore.pyqtSignal(str, float)  # Signal to update spinbox (name, value)
    
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
        self.current_button = None # notifica del pulsante premuto
        self.spinbox_values = {} # notifica dello spinbox cambiato
        
        self.thc = self.HysteresisController(lower_limit = 37.5, upper_limit = 37.8) # temperature hysteresis controller
        self.current_heater_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        self.hhc = self.HysteresisController(lower_limit = 20.0, upper_limit = 50.0) # humidity hysteresis controller
        self.current_humidifier_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        
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
                
            self.msleep(100)
            
    def queue_command(self, cmd, value):
        self.command_list.append((cmd, value))

    def stop(self):
        self.running = False
        self.serial_thread.stop()
        self.serial_thread.wait()
        
    def handle_button_click(self, button_name):
        print(f"[MainSoftwareThread] Processing button {button_name}")
        self.current_button = button_name
        
        if self.current_button == "move_CW__motor_btn":
            self.queue_command("CMD01", 1)
            
        if self.current_button == "move_CCW_motor_btn":
            self.queue_command("CMD02", 0)
            
        if self.current_button == "reset_motor_btn":
            self.queue_command("CMD03", 0)
            
        if self.current_button == "plotAllDays_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_TEMPERATURES']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotToday_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_TEMPERATURES']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotAllDays_humidity_H_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_HUMIDITY']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotToday_humidity_H_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_HUMIDITY']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
        
    def handle_float_spinBox_value(self, spinbox_name, value):
        rounded_value = round(value, 1)
        print(f"[MainSoftwareThread] Processing spinbox {spinbox_name} of value: {rounded_value} ({type(rounded_value)})")
        self.spinbox_values[spinbox_name] = rounded_value       
        
        if spinbox_name == "speedRPM_motor_spinBox":
            self.current_speedRPM_spinBox_value = rounded_value
        elif spinbox_name == "maxHysteresisValue_temperature_spinBox":
            if rounded_value <= self.thc.get_lower_limit():
                self.thc.set_upper_limit(rounded_value)
                self.thc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("minHysteresisValue_temperature_spinBox", rounded_value)
            else:
                self.thc.set_upper_limit(rounded_value)
                
        elif spinbox_name == "minHysteresisValue_temperature_spinBox":
            if rounded_value >= self.thc.get_upper_limit():
                self.thc.set_upper_limit(rounded_value)
                self.thc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("maxHysteresisValue_temperature_spinBox", rounded_value)
            else:
                self.thc.set_lower_limit(rounded_value)
                
        elif spinbox_name == "maxHysteresisValue_humidity_spinBox":
            if rounded_value <= self.hhc.get_lower_limit():
                self.hhc.set_upper_limit(rounded_value)
                self.hhc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("minHysteresisValue_humidity_spinBox", rounded_value)
            else:
                self.hhc.set_upper_limit(rounded_value)
                
        elif spinbox_name == "minHysteresisValue_humidity_spinBox":
            if rounded_value >= self.hhc.get_upper_limit():
                self.hhc.set_upper_limit(rounded_value)
                self.hhc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("maxHysteresisValue_humidity_spinBox", rounded_value)
            else:
                self.hhc.set_lower_limit(rounded_value)
            
    def handle_intialization_step(self, value_name, value):
        rounded_value = round(value, 1)
        if value_name == "maxHysteresisValue_temperature_spinBox":
            self.thc.set_upper_limit(rounded_value)
        elif value_name == "minHysteresisValue_temperature_spinBox":
            self.thc.set_lower_limit(rounded_value)
        elif value_name == "maxHysteresisValue_humidity_spinBox":
            self.hhc.set_upper_limit(rounded_value)
        elif value_name == "minHysteresisValue_humidity_spinBox":
            self.hhc.set_lower_limit(rounded_value)

    def process_serial_data(self, new_data):
        self.current_data = new_data
        # print(new_data)
        # Extract data into specific categories
        current_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("TMP")} # dictionary: <class 'dict'> dict_values([23.7, 21.1, 20.4, 21.7]) 
        current_humidities = {k: v for d in new_data for k, v in d.items() if k.startswith("HUM")}
        current_humidities_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("HTP")}
        
        # TEMPERATURE CONTROLLER SECTION
        # faccio l'update qui: ogni votla che arrivano dati nuovi li elaboro, anche nel controllore
        self.thc.update(list(current_temperatures.values()))
        if self.current_heater_output_control != self.thc.get_output_control(): # appena cambia il comando logico all'heater, allora invio il comando ad Arduino            
            self.current_heater_output_control = self.thc.get_output_control()
            self.queue_command("HTR01", self.current_heater_output_control)  # @<HTR01, True># @<HTR01, False>#
            
        # HUMIDITY CONTROLLER SECTION
        self.hhc.update(list(current_humidities.values()))
        if self.current_humidifier_output_control != self.hhc.get_output_control():
            self.current_humidifier_output_control = self.hhc.get_output_control()
            self.queue_command("HUMER01", self.current_humidifier_output_control) # @<HUMER01, True># @<HUMER01, False>#
            
            
        # Emit the data to update the view        
        # Collecting all values into a single list
        # questo all_values è semplicemente una lista [17.8, 17.9, 18.0, 17.9, 17.8, 17.9] dove SO IO ad ogni posto cosa è associato...passiamo solo i valori (non bellissimo...)
        # i mean value servono per pubblicare il valore che il controllore usa per fare effettivamente il controllo e lo metto nella sezione di isteresi
        # list() se passi un dizionario, mentre [] se vuoi aggiugnere alla lista elementi singoli
        all_values = list(current_temperatures.values()) + \
                    list(current_humidities.values()) + \
                    list(current_humidities_temperatures.values()) + \
                    [self.thc.get_mean_value()] + \
                    [self.hhc.get_mean_value()]
                    
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
            
    class HysteresisController:
        def __init__(self, lower_limit, upper_limit):
            self.lower_limit = lower_limit
            self.upper_limit = upper_limit
            self._min_value = float('inf')  # Track the minimum value observed
            self._max_value = float('-inf') # Track the maximum value observed
            self._mean_value = None         # Mean value of the monitored inputs
            self._output_control = False    # False = OFF, True = ON
            
            # Statistics tracking
            self.on_count = 0
            self.off_count = 0
            self.time_on = 0.0
            self.time_off = 0.0
            self._last_switch_time = time.time()
        
        # Method to set the lower limit
        def set_lower_limit(self, lower_limit):
            self.lower_limit = lower_limit
            if self.lower_limit == self.upper_limit:
                self._output_control = False  # Safety enforcement
        
        # Method to get the lower limit
        def get_lower_limit(self):
            return self.lower_limit
        
        # Method to set the upper limit
        def set_upper_limit(self, upper_limit):
            self.upper_limit = upper_limit
            if self.lower_limit == self.upper_limit:
                self._output_control = False  # Safety enforcement
        
        # Method to get the upper limit
        def get_upper_limit(self):
            return self.upper_limit

        # Update method to process input values and apply hysteresis logic
        def update(self, values):
            # Safety check: If limits are equal, force OFF state
            if self.lower_limit == self.upper_limit:
                self._output_control = False
                return
        
            # Ensure values is a list for consistency
            if not isinstance(values, list):
                values = [values]
            
            # Calculate the mean of the values
            self._mean_value = round(sum(values) / len(values), 1)
            
            # Update the max and min values reached
            for value in values:
                self._max_value = max(self._max_value, value)
                self._min_value = min(self._min_value, value)
            
            # Check if output state changes
            new_output = self._output_control
            if self._mean_value > self.upper_limit:
                new_output = False  # Turn OFF if mean is above upper limit
            elif self._mean_value < self.lower_limit:
                new_output = True   # Turn ON if mean is below lower limit
            
            # If state changed, update time tracking and count transitions
            if new_output != self._output_control:
                elapsed_time = time.time() - self._last_switch_time
                if self._output_control:
                    self.time_on += elapsed_time
                    self.off_count += 1
                else:
                    self.time_off += elapsed_time
                    self.on_count += 1
                
                self._output_control = new_output
                self._last_switch_time = time.time()
        
        # Method to get the output control (True or False)
        def get_output_control(self):
            return self._output_control
        
        # Method to get the maximum value reached
        def get_max_value(self):
            return self._max_value
        
        # Method to get the minimum value reached
        def get_min_value(self):
            return self._min_value

        # Method to get the mean value of the last monitored inputs
        def get_mean_value(self):
            return self._mean_value

        # Method to reset min/max values
        def reset_max_value(self):
            self._max_value = float('-inf')
        
        def reset_min_value(self):
            self._min_value = float('inf')

        def reset_all_values(self):
            self._min_value = float('inf')
            self._max_value = float('-inf')

        # Method to get the number of ON/OFF transitions
        def get_on_count(self):
            return self.on_count

        def get_off_count(self):
            return self.off_count

        # Method to get the total time spent ON/OFF
        def get_time_on(self):
            return self.time_on

        def get_time_off(self):
            return self.time_off

        # Method to reset statistics
        def reset_time_statistics(self):
            self.on_count = 0
            self.off_count = 0
            self.time_on = 0.0
            self.time_off = 0.0
            self._last_switch_time = time.time()
        
        def reset_absolute_temperatures_statistics(self):
            self._max_value = float('-inf')
            self._min_value = float('inf')


class MainWindow(QtWidgets.QMainWindow):
    # Define custom signals - this is done to send button/spinBox and other custom signals to other thread MainSoftwareThread: use Qt Signals
    button_clicked = QtCore.pyqtSignal(str)  # Emits button name
    float_spinBox_value_changed = QtCore.pyqtSignal(str, float)  # Emits spinbox value  // METTI INT se intero
    initialization_step = QtCore.pyqtSignal(str, float)
    
    def __init__(self, main_software_thread):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.main_software_thread = main_software_thread
        self.main_software_thread.update_view.connect(self.update_display_data)
        
        # Connect signals to main software thread slots
        self.button_clicked.connect(self.main_software_thread.handle_button_click)
        self.float_spinBox_value_changed.connect(self.main_software_thread.handle_float_spinBox_value)
        self.initialization_step.connect(self.main_software_thread.handle_intialization_step)
        self.main_software_thread.update_spinbox_value.connect(self.update_spinbox)


        # Connect buttons to handlers that emit signals
        self.ui.move_CW_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.move_CW_motor_btn.objectName()))
        self.ui.move_CCW_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.move_CCW_motor_btn.objectName()))
        self.ui.layHorizontal_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.layHorizontal_motor_btn.objectName()))
        self.ui.forceEggsTurn_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.forceEggsTurn_motor_btn.objectName()))
        self.ui.reset_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.reset_motor_btn.objectName()))
        
        self.ui.plotAllDays_temp_T_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotAllDays_temp_T_btn.objectName()))
        self.ui.plotToday_temp_T_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotToday_temp_T_btn.objectName()))
        self.ui.plotAllDays_humidity_H_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotAllDays_humidity_H_btn.objectName()))
        self.ui.plotToday_humidity_H_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotToday_humidity_H_btn.objectName()))

        # Connect radio buttons to emit its values
        self.ui.heaterOFF_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.heaterAUTO_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.heaterON_radioBtn.toggled.connect(self.handle_radio_button)
        
        self.ui.humidifierOFF_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.humidifierAUTO_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.humidifierON_radioBtn.toggled.connect(self.handle_radio_button)
        
        # Connect spinBox to emit its values
        self.ui.speedRPM_motor_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.speedRPM_motor_spinBox.objectName(), value))
        
        # Temperature Hysteresis
        self.ui.maxHysteresisValue_temperature_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.maxHysteresisValue_temperature_spinBox.objectName(), value))
        self.ui.minHysteresisValue_temperature_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.minHysteresisValue_temperature_spinBox.objectName(), value))
        
        # Humidity Hysteresis
        self.ui.maxHysteresisValue_humidity_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.maxHysteresisValue_humidity_spinBox.objectName(), value))
        self.ui.minHysteresisValue_humidity_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.minHysteresisValue_humidity_spinBox.objectName(), value))
        
        # Connect to sen initialization values to the mainSoftwareThread
        self.emit_initialization_values(self.ui.maxHysteresisValue_temperature_spinBox.objectName(), self.ui.maxHysteresisValue_temperature_spinBox.value())
        self.emit_initialization_values(self.ui.minHysteresisValue_temperature_spinBox.objectName(), self.ui.minHysteresisValue_temperature_spinBox.value())
        
        self.emit_initialization_values(self.ui.maxHysteresisValue_humidity_spinBox.objectName(), self.ui.maxHysteresisValue_humidity_spinBox.value())
        self.emit_initialization_values(self.ui.minHysteresisValue_humidity_spinBox.objectName(), self.ui.minHysteresisValue_humidity_spinBox.value())
        
    def emit_initialization_values(self, spinbox_name, value):
        self.initialization_step.emit(spinbox_name, value)
        
        
    def emit_button_signal(self, button_name):
        self.button_clicked.emit(button_name)
         #print(f"Button clicked: {button_name}")

    def emit_float_spinbox_signal(self, spinbox_name, value):
        #value = float(value)  # Cast value to float explicitly
        self.float_spinBox_value_changed.emit(spinbox_name, value)
        #print(f"[MainWindow] Emitting signal from {spinbox_name} with value: {value}")

    def update_display_data(self, all_data):
        # Update the temperature labels in the GUI
        if len(all_data) >= 6: # perché il numero??
            self.ui.temperature1_T.setText(f"{all_data[0]} °C")
            self.ui.temperature2_T.setText(f"{all_data[1]} °C")
            self.ui.temperature3_T.setText(f"{all_data[2]} °C")
            self.ui.temperature4_T.setText(f"{all_data[3]} °C")
            self.ui.humidity1_H.setText(f"{all_data[4]} %")
            self.ui.heatCtrlVal.setText(f"{all_data[5]} °C")
            self.ui.humCtrlVal.setText(f"{all_data[6]} °C")
            #self.ui.temperature4_2.setText(f"{all_data[3]} °C") PER TEMPERATURA DA UMIDITA

    def update_spinbox(self, spinbox_name, value):
        """ Update the spinbox in the GUI safely from another thread """
        spinbox = getattr(self.ui, spinbox_name, None)  # Get the spinbox dynamically

        if spinbox:  # Ensure the spinbox exists
            spinbox.setValue(value)  # Set the new value safely in the GUI thread
            
    def handle_radio_button(self):
        sender = self.sender()
        if sender.isChecked():
            print(f"Radio button '{sender.text()}' selected")

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

import csv
import time
import queue
import subprocess

'''
    @<HTR01, True>#   heater 
    @<HUMER01, True># humidifier
'''

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
                print("Estabilishing serial connection....")
                self.serial_port = serial.Serial(self.port, self.baudrate, timeout=1)
                time.sleep(3)
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
    update_spinbox_value = QtCore.pyqtSignal(str, float)  # Signal to update spinbox (name, value)
    
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
        self.current_button = None # notifica del pulsante premuto
        self.spinbox_values = {} # notifica dello spinbox cambiato
        
        self.thc = self.HysteresisController(lower_limit = 37.5, upper_limit = 37.8) # temperature hysteresis controller
        self.current_heater_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        self.hhc = self.HysteresisController(lower_limit = 20.0, upper_limit = 50.0) # humidity hysteresis controller
        self.current_humidifier_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        
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
                
            self.msleep(100)
            
    def queue_command(self, cmd, value):
        self.command_list.append((cmd, value))

    def stop(self):
        self.running = False
        self.serial_thread.stop()
        self.serial_thread.wait()
        
    def handle_button_click(self, button_name):
        print(f"[MainSoftwareThread] Processing button {button_name}")
        self.current_button = button_name
        
        if self.current_button == "move_CW__motor_btn":
            self.queue_command("CMD01", 1)
            
        if self.current_button == "move_CCW_motor_btn":
            self.queue_command("CMD02", 0)
            
        if self.current_button == "reset_motor_btn":
            self.queue_command("CMD03", 0)
            
        if self.current_button == "plotAllDays_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_TEMPERATURES']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotToday_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_TEMPERATURES']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotAllDays_humidity_H_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_HUMIDITY']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotToday_humidity_H_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_HUMIDITY']
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
        
    def handle_float_spinBox_value(self, spinbox_name, value):
        rounded_value = round(value, 1)
        print(f"[MainSoftwareThread] Processing spinbox {spinbox_name} of value: {rounded_value} ({type(rounded_value)})")
        self.spinbox_values[spinbox_name] = rounded_value       
        
        if spinbox_name == "speedRPM_motor_spinBox":
            self.current_speedRPM_spinBox_value = rounded_value
        elif spinbox_name == "maxHysteresisValue_temperature_spinBox":
            if rounded_value <= self.thc.get_lower_limit():
                self.thc.set_upper_limit(rounded_value)
                self.thc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("minHysteresisValue_temperature_spinBox", rounded_value)
            else:
                self.thc.set_upper_limit(rounded_value)
                
        elif spinbox_name == "minHysteresisValue_temperature_spinBox":
            if rounded_value >= self.thc.get_upper_limit():
                self.thc.set_upper_limit(rounded_value)
                self.thc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("maxHysteresisValue_temperature_spinBox", rounded_value)
            else:
                self.thc.set_lower_limit(rounded_value)
                
        elif spinbox_name == "maxHysteresisValue_humidity_spinBox":
            if rounded_value <= self.hhc.get_lower_limit():
                self.hhc.set_upper_limit(rounded_value)
                self.hhc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("minHysteresisValue_humidity_spinBox", rounded_value)
            else:
                self.hhc.set_upper_limit(rounded_value)
                
        elif spinbox_name == "minHysteresisValue_humidity_spinBox":
            if rounded_value >= self.hhc.get_upper_limit():
                self.hhc.set_upper_limit(rounded_value)
                self.hhc.set_lower_limit(rounded_value)
                self.update_spinbox_value.emit("maxHysteresisValue_humidity_spinBox", rounded_value)
            else:
                self.hhc.set_lower_limit(rounded_value)
            
    def handle_intialization_step(self, value_name, value):
        rounded_value = round(value, 1)
        if value_name == "maxHysteresisValue_temperature_spinBox":
            self.thc.set_upper_limit(rounded_value)
        elif value_name == "minHysteresisValue_temperature_spinBox":
            self.thc.set_lower_limit(rounded_value)
        elif value_name == "maxHysteresisValue_humidity_spinBox":
            self.hhc.set_upper_limit(rounded_value)
        elif value_name == "minHysteresisValue_humidity_spinBox":
            self.hhc.set_lower_limit(rounded_value)

    def process_serial_data(self, new_data):
        self.current_data = new_data
        # print(new_data)
        # Extract data into specific categories
        current_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("TMP")} # dictionary: <class 'dict'> dict_values([23.7, 21.1, 20.4, 21.7]) 
        current_humidities = {k: v for d in new_data for k, v in d.items() if k.startswith("HUM")}
        current_humidities_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("HTP")}
        
        # TEMPERATURE CONTROLLER SECTION
        # faccio l'update qui: ogni votla che arrivano dati nuovi li elaboro, anche nel controllore
        self.thc.update(list(current_temperatures.values()))
        if self.current_heater_output_control != self.thc.get_output_control(): # appena cambia il comando logico all'heater, allora invio il comando ad Arduino            
            self.current_heater_output_control = self.thc.get_output_control()
            self.queue_command("HTR01", self.current_heater_output_control)  # @<HTR01, True># @<HTR01, False>#
            
        # HUMIDITY CONTROLLER SECTION
        self.hhc.update(list(current_humidities.values()))
        if self.current_humidifier_output_control != self.hhc.get_output_control():
            self.current_humidifier_output_control = self.hhc.get_output_control()
            self.queue_command("HUMER01", self.current_humidifier_output_control) # @<HUMER01, True># @<HUMER01, False>#
            
            
        # Emit the data to update the view        
        # Collecting all values into a single list
        # questo all_values è semplicemente una lista [17.8, 17.9, 18.0, 17.9, 17.8, 17.9] dove SO IO ad ogni posto cosa è associato...passiamo solo i valori (non bellissimo...)
        # i mean value servono per pubblicare il valore che il controllore usa per fare effettivamente il controllo e lo metto nella sezione di isteresi
        # list() se passi un dizionario, mentre [] se vuoi aggiugnere alla lista elementi singoli
        all_values = list(current_temperatures.values()) + \
                    list(current_humidities.values()) + \
                    list(current_humidities_temperatures.values()) + \
                    [self.thc.get_mean_value()] + \
                    [self.hhc.get_mean_value()]
                    
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
            
    class HysteresisController:
        def __init__(self, lower_limit, upper_limit):
            self.lower_limit = lower_limit
            self.upper_limit = upper_limit
            self._min_value = float('inf')  # Track the minimum value observed
            self._max_value = float('-inf') # Track the maximum value observed
            self._mean_value = None         # Mean value of the monitored inputs
            self._output_control = False    # False = OFF, True = ON
            
            # Statistics tracking
            self.on_count = 0
            self.off_count = 0
            self.time_on = 0.0
            self.time_off = 0.0
            self._last_switch_time = time.time()
        
        # Method to set the lower limit
        def set_lower_limit(self, lower_limit):
            self.lower_limit = lower_limit
            if self.lower_limit == self.upper_limit:
                self._output_control = False  # Safety enforcement
        
        # Method to get the lower limit
        def get_lower_limit(self):
            return self.lower_limit
        
        # Method to set the upper limit
        def set_upper_limit(self, upper_limit):
            self.upper_limit = upper_limit
            if self.lower_limit == self.upper_limit:
                self._output_control = False  # Safety enforcement
        
        # Method to get the upper limit
        def get_upper_limit(self):
            return self.upper_limit

        # Update method to process input values and apply hysteresis logic
        def update(self, values):
            # Safety check: If limits are equal, force OFF state
            if self.lower_limit == self.upper_limit:
                self._output_control = False
                return
        
            # Ensure values is a list for consistency
            if not isinstance(values, list):
                values = [values]
            
            # Calculate the mean of the values
            self._mean_value = round(sum(values) / len(values), 1)
            
            # Update the max and min values reached
            for value in values:
                self._max_value = max(self._max_value, value)
                self._min_value = min(self._min_value, value)
            
            # Check if output state changes
            new_output = self._output_control
            if self._mean_value > self.upper_limit:
                new_output = False  # Turn OFF if mean is above upper limit
            elif self._mean_value < self.lower_limit:
                new_output = True   # Turn ON if mean is below lower limit
            
            # If state changed, update time tracking and count transitions
            if new_output != self._output_control:
                elapsed_time = time.time() - self._last_switch_time
                if self._output_control:
                    self.time_on += elapsed_time
                    self.off_count += 1
                else:
                    self.time_off += elapsed_time
                    self.on_count += 1
                
                self._output_control = new_output
                self._last_switch_time = time.time()
        
        # Method to get the output control (True or False)
        def get_output_control(self):
            return self._output_control
        
        # Method to get the maximum value reached
        def get_max_value(self):
            return self._max_value
        
        # Method to get the minimum value reached
        def get_min_value(self):
            return self._min_value

        # Method to get the mean value of the last monitored inputs
        def get_mean_value(self):
            return self._mean_value

        # Method to reset min/max values
        def reset_max_value(self):
            self._max_value = float('-inf')
        
        def reset_min_value(self):
            self._min_value = float('inf')

        def reset_all_values(self):
            self._min_value = float('inf')
            self._max_value = float('-inf')

        # Method to get the number of ON/OFF transitions
        def get_on_count(self):
            return self.on_count

        def get_off_count(self):
            return self.off_count

        # Method to get the total time spent ON/OFF
        def get_time_on(self):
            return self.time_on

        def get_time_off(self):
            return self.time_off

        # Method to reset statistics
        def reset_time_statistics(self):
            self.on_count = 0
            self.off_count = 0
            self.time_on = 0.0
            self.time_off = 0.0
            self._last_switch_time = time.time()
        
        def reset_absolute_temperatures_statistics(self):
            self._max_value = float('-inf')
            self._min_value = float('inf')


class MainWindow(QtWidgets.QMainWindow):
    # Define custom signals - this is done to send button/spinBox and other custom signals to other thread MainSoftwareThread: use Qt Signals
    button_clicked = QtCore.pyqtSignal(str)  # Emits button name
    float_spinBox_value_changed = QtCore.pyqtSignal(str, float)  # Emits spinbox value  // METTI INT se intero
    initialization_step = QtCore.pyqtSignal(str, float)
    
    def __init__(self, main_software_thread):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.main_software_thread = main_software_thread
        self.main_software_thread.update_view.connect(self.update_display_data)
        
        # Connect signals to main software thread slots
        self.button_clicked.connect(self.main_software_thread.handle_button_click)
        self.float_spinBox_value_changed.connect(self.main_software_thread.handle_float_spinBox_value)
        self.initialization_step.connect(self.main_software_thread.handle_intialization_step)
        self.main_software_thread.update_spinbox_value.connect(self.update_spinbox)


        # Connect buttons to handlers that emit signals
        self.ui.move_CW_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.move_CW_motor_btn.objectName()))
        self.ui.move_CCW_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.move_CCW_motor_btn.objectName()))
        self.ui.layHorizontal_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.layHorizontal_motor_btn.objectName()))
        self.ui.forceEggsTurn_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.forceEggsTurn_motor_btn.objectName()))
        self.ui.reset_motor_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.reset_motor_btn.objectName()))
        
        self.ui.plotAllDays_temp_T_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotAllDays_temp_T_btn.objectName()))
        self.ui.plotToday_temp_T_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotToday_temp_T_btn.objectName()))
        self.ui.plotAllDays_humidity_H_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotAllDays_humidity_H_btn.objectName()))
        self.ui.plotToday_humidity_H_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.plotToday_humidity_H_btn.objectName()))

        # Connect radio buttons to emit its values
        self.ui.heaterOFF_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.heaterAUTO_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.heaterON_radioBtn.toggled.connect(self.handle_radio_button)
        
        self.ui.humidifierOFF_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.humidifierAUTO_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.humidifierON_radioBtn.toggled.connect(self.handle_radio_button)
        
        # Connect spinBox to emit its values
        self.ui.speedRPM_motor_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.speedRPM_motor_spinBox.objectName(), value))
        
        # Temperature Hysteresis
        self.ui.maxHysteresisValue_temperature_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.maxHysteresisValue_temperature_spinBox.objectName(), value))
        self.ui.minHysteresisValue_temperature_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.minHysteresisValue_temperature_spinBox.objectName(), value))
        
        # Humidity Hysteresis
        self.ui.maxHysteresisValue_humidity_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.maxHysteresisValue_humidity_spinBox.objectName(), value))
        self.ui.minHysteresisValue_humidity_spinBox.valueChanged.connect(lambda value: self.emit_float_spinbox_signal(self.ui.minHysteresisValue_humidity_spinBox.objectName(), value))
        
        # Connect to sen initialization values to the mainSoftwareThread
        self.emit_initialization_values(self.ui.maxHysteresisValue_temperature_spinBox.objectName(), self.ui.maxHysteresisValue_temperature_spinBox.value())
        self.emit_initialization_values(self.ui.minHysteresisValue_temperature_spinBox.objectName(), self.ui.minHysteresisValue_temperature_spinBox.value())
        
        self.emit_initialization_values(self.ui.maxHysteresisValue_humidity_spinBox.objectName(), self.ui.maxHysteresisValue_humidity_spinBox.value())
        self.emit_initialization_values(self.ui.minHysteresisValue_humidity_spinBox.objectName(), self.ui.minHysteresisValue_humidity_spinBox.value())
        
    def emit_initialization_values(self, spinbox_name, value):
        self.initialization_step.emit(spinbox_name, value)
        
        
    def emit_button_signal(self, button_name):
        self.button_clicked.emit(button_name)
         #print(f"Button clicked: {button_name}")

    def emit_float_spinbox_signal(self, spinbox_name, value):
        #value = float(value)  # Cast value to float explicitly
        self.float_spinBox_value_changed.emit(spinbox_name, value)
        #print(f"[MainWindow] Emitting signal from {spinbox_name} with value: {value}")

    def update_display_data(self, all_data):
        # Update the temperature labels in the GUI
        if len(all_data) >= 6: # perché il numero??
            self.ui.temperature1_T.setText(f"{all_data[0]} °C")
            self.ui.temperature2_T.setText(f"{all_data[1]} °C")
            self.ui.temperature3_T.setText(f"{all_data[2]} °C")
            self.ui.temperature4_T.setText(f"{all_data[3]} °C")
            self.ui.humidity1_H.setText(f"{all_data[4]} %")
            self.ui.heatCtrlVal.setText(f"{all_data[5]} °C")
            self.ui.humCtrlVal.setText(f"{all_data[6]} °C")
            #self.ui.temperature4_2.setText(f"{all_data[3]} °C") PER TEMPERATURA DA UMIDITA

    def update_spinbox(self, spinbox_name, value):
        """ Update the spinbox in the GUI safely from another thread """
        spinbox = getattr(self.ui, spinbox_name, None)  # Get the spinbox dynamically

        if spinbox:  # Ensure the spinbox exists
            spinbox.setValue(value)  # Set the new value safely in the GUI thread
            
    def handle_radio_button(self):
        sender = self.sender()
        if sender.isChecked():
            print(f"Radio button '{sender.text()}' selected")

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
