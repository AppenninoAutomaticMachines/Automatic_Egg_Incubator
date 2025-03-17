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
import subprocess
from collections import deque


'''
    i comandi devono essere quanto più univoci possibile! perhé ho visto che se metto CW e CCW alla domanda indexof() arduino non li sa distinguere
    self.queue_command("ACK", "1") - self.queue_command("ACK", "1")
    self.queue_command("HTR01", self.current_heater_output_control)  # @<HTR01, True># @<HTR01, False>#
    self.queue_command("HUMER01", self.current_humidifier_output_control) # @<HUMER01, True># @<HUMER01, False>#
    self.queue_command("STPR01", "MCCW") # move_counter_clock_wise
    self.queue_command("STPR01", "MCW") # move_clock_wise 
    self.queue_command("STPR01", "STOP") # stop motor   
'''

# ARDUINO serial communication - setup #
portSetup = "/dev/ttyACM0"
portSetup = "/dev/ttyUSB0"
baudrateSetup = 19200
timeout = 0.1

identifiers = ["TMP", "HUM", "HTP", "IND"]  # Global variable


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
    update_statistics = QtCore.pyqtSignal(list) # Signal to update the Statistics View Window
    update_spinbox_value = QtCore.pyqtSignal(str, float)  # Signal to update spinbox (name, value)
    update_motor = QtCore.pyqtSignal(list) # Signal to update the motor view
    
    def __init__(self):
        super().__init__()
        self.running = True
        self.serial_thread = SerialThread()
        self.serial_thread.data_received.connect(self.process_serial_data)
        self.current_data = []  # Holds the most recent data received from SerialThread
        
        self.alive_to_arduino_state = False
        self.alive_to_arduino_time_interval_sec = 1 # invio un ack ad arduino ogni secondo
        self.last_alive_to_arduino_time = time.time()
        
        self.saving_interval = 1 # default: every minute
        self.last_saving_time = datetime.now()
        self.arduino_readings_timestamp  = time.time() # per ricordarmi l'istante di tempo in cui arduino manda i dati
        
        self.command_list = [] # List to hold the pending commands
        
        # State variables to handle inputs from MainWindow
        self.current_button = None # notifica del pulsante premuto
        self.spinbox_values = {} # notifica dello spinbox cambiato
        
        # TEMPERATURE CONTROLLER
        # Constants
        self.INVALID_VALUES = [-127.0, 85.0]  # Known error values
        self.THRESHOLD = 0.5  # Acceptable variation from the valid range
        self.VALID_RANGE_TEMPERATURE = (5.0, 45.0)  # Expected temperature range
        
        # Error tracking
        self.ERROR_TIME_LIMIT = 10  # Tempo in secondi oltre il quale generare un warning
        # Stato del sistema
        self.error_counter = 0  # Conta tutti gli errori mai verificati
        self.error_timestamps = {}  # Memorizza il primo timestamp di errore per ogni sensore
        self.warned_sensors = set()  # Tiene traccia dei sensori già segnalati con un warning

        self.thc = self.HysteresisController(lower_limit = 37.5, upper_limit = 37.8) # temperature hysteresis controller
        self.current_heater_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        self.hhc = self.HysteresisController(lower_limit = 20.0, upper_limit = 50.0) # humidity hysteresis controller
        self.current_humidifier_output_control = False # variabile che mi ricorda lo stato attuale dell'heater
        
        self.eggTurnerMotor = self.StepperMotor("Egg_Turner_Stepper_Motor")
        self.eggTurnerMotor.setFunctionInterval(30) # Set default interval to 3600s 
        
        # BUTTON HANDLING
        self.move_CW_motor_btn = False # = not pressed
        self.move_CCW_motor_btn = False # = not pressed
		
		# PLOT
        self.remove_erroneous_values_from_plot = True
        
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
                
            # parte che gira periodica, quindi ci metto la gestione del motore siccome deve funzionare periodicamente per il timer.
            self.eggTurnerMotor.update()
            
            if self.eggTurnerMotor.getNewCommand() is not None: # checking if there's a command
                print(f"Processing command: {self.eggTurnerMotor.getNewCommand()}") 
                
                if (self.eggTurnerMotor.getNewCommand() == "automatic_CCW_rotation_direction"
                    or
                    self.eggTurnerMotor.getNewCommand() == "manual_CCW_rotation_direction"):
                    self.queue_command("STPR01", "MCCW") # move_counter_clock_wise
                    
                if (self.eggTurnerMotor.getNewCommand() == "automatic_CW_rotation_direction"
                    or
                    self.eggTurnerMotor.getNewCommand() == "manual_CW_rotation_direction"):
                    self.queue_command("STPR01", "MCW") # move_clock_wise           
                    
                if self.eggTurnerMotor.getNewCommand() == "stop":
                    self.queue_command("STPR01", "STOP") # stop motor 
                
                self.eggTurnerMotor.resetNewCommand()
            
            if self.eggTurnerMotor.getUpdateMotorData():
                all_values = []
                # [timePassed timeToNextTurn turnsCounter]
                all_values = [self.eggTurnerMotor.getTimeSinceLastRotation()] + \
                                [self.eggTurnerMotor.getTimeUntilNextRotation()] + \
                                [self.eggTurnerMotor.getTurnsCounter()]
                self.update_motor.emit(all_values)
                
                self.eggTurnerMotor.resetUpdateMotorData()
            
            # GESTIONE DELL'ALIVE BIT verso arduino
            if (time.time() - self.last_alive_to_arduino_time) >= self.alive_to_arduino_time_interval_sec:
                self.last_alive_to_arduino_time = time.time()
                self.alive_to_arduino_state = not self.alive_to_arduino_state
                self.queue_command("ALIVE", self.alive_to_arduino_state)
              
            self.msleep(100)
            
    def check_errors(self,sensor_data):
        """Verifica errori, aggiorna il contatore e traccia il tempo degli errori persistenti."""

        current_time = time.time()
        new_errors = []  # Sensori che entrano in errore ora
        resolved_errors = []  # Sensori che sono tornati normali

        # Analizza il dizionario ricevuto
        #print(sensor_data)
        for sensor, value in sensor_data.items():
            # Verifica se il valore è un errore
            if any(abs(value - iv) < self.THRESHOLD for iv in self.INVALID_VALUES) or not (self.VALID_RANGE_TEMPERATURE[0] <= value <= self.VALID_RANGE_TEMPERATURE[1]):
                # Se è un nuovo errore, aggiorna il contatore e salva il timestamp
                if sensor not in self.error_timestamps:
                    self.error_timestamps[sensor] = current_time
                    self.error_counter += 1
                    new_errors.append(sensor)  # Sensore appena entrato in errore
            else:
                # Se il sensore era in errore ma ora non lo è più, lo rimuoviamo dal tracking
                if sensor in self.error_timestamps:
                    del self.error_timestamps[sensor]
                    self.warned_sensors.discard(sensor)  # Resetta anche il warning
                    resolved_errors.append(sensor)

        # Stampa solo se ci sono nuovi errori o warning per errori persistenti
        if new_errors:
            print(f"⚠ ERRORE: Sensori appena entrati in errore: {new_errors}")
            print(f"Totale errori rilevati finora: {self.error_counter}")

        for sensor, start_time in self.error_timestamps.items():
            if current_time - start_time > self.ERROR_TIME_LIMIT and sensor not in self.warned_sensors:
                print(f"⚠ WARNING: Il sensore '{sensor}' è in errore da più di {self.ERROR_TIME_LIMIT} secondi!")
                self.warned_sensors.add(sensor)  # Segnala il warning solo una volta
            
    def filter_temperatures(self, temperatures):
        """Remove outliers and keep only valid temperature values."""
        return [
            temp for temp in temperatures
            if not any(abs(temp - iv) < self.THRESHOLD for iv in self.INVALID_VALUES)
            and self.VALID_RANGE_TEMPERATURE[0] <= temp <= self.VALID_RANGE_TEMPERATURE[1]
        ]
            
    def queue_command(self, cmd, value):
        self.command_list.append((cmd, value))

    def stop(self):
        self.running = False
        self.serial_thread.stop()
        self.serial_thread.wait()
        
    def handle_button_click(self, button_name):
        print(f"[MainSoftwareThread] Processing button {button_name}")
        self.current_button = button_name
        
        if self.current_button == "forceEggsTurn_motor_btn":
            self.eggTurnerMotor.forceEggsRotation()
        
        if self.current_button == "move_CW_motor_btn":
            self.move_CW_motor_btn = not self.move_CW_motor_btn # ogni volta in cui lo premo, toggle dello stato
            
            if self.move_CW_motor_btn:
                self.eggTurnerMotor.moveCWContinuous()
            elif not self.move_CW_motor_btn:
                self.eggTurnerMotor.stop()
            
        if self.current_button == "move_CCW_motor_btn":
            self.move_CCW_motor_btn = not self.move_CCW_motor_btn
            
            if self.move_CCW_motor_btn:
                self.eggTurnerMotor.moveCCWContinuous()
            elif not self.move_CCW_motor_btn:
                self.eggTurnerMotor.stop()
				
        if self.current_button == "reset_statistics_T_btn":
            self.thc.reset_all_values()
            
        if self.current_button == "plotAllDays_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_TEMPERATURES', self.VALID_RANGE_TEMPERATURE[0], self.VALID_RANGE_TEMPERATURE[1], self.remove_erroneous_values_from_plot]
            process = subprocess.Popen(command)
            print("Subprocess started and main program continues...")
            
        if self.current_button == "plotToday_temp_T_btn":
            command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_TEMPERATURES', self.VALID_RANGE_TEMPERATURE[0], self.VALID_RANGE_TEMPERATURE[1], self.remove_erroneous_values_from_plot]
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
        self.arduino_readings_timestamp = time.time()
        self.current_data = new_data
        #print(new_data)
        # [{'TMP01': 19.5}, {'TMP02': 19.1}, {'TMP03': 19.8}, {'TMP04': 20.1}, {'HUM01': 19.5}, {'HTP01': 19.5}]
        # Extract data into specific categories - questi che seguono sono tutti dictionaries
        current_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("TMP")} # {'TMP01': 22.3, 'TMP02': 22.2, 'TMP03': 22.3, 'TMP04': 22.4}
        current_humidities = {k: v for d in new_data for k, v in d.items() if k.startswith("HUM")}
        current_humidities_temperatures = {k: v for d in new_data for k, v in d.items() if k.startswith("HTP")}
        current_inductors_feedbacks = {k: v for d in new_data for k, v in d.items() if k.startswith("IND")} #{'IND_CCW': 1, 'IND_CW': 1}
        
        
        # TEMPERATURE CONTROLLER SECTION
        # faccio l'update qui: ogni votla che arrivano dati nuovi li elaboro, anche nel controllore
        # al controllore di temperatura passo solo temperature filtrate, ovvero i valori dentro il range di temperatura corretto
        
        filtered_temperatures = self.filter_temperatures(list(current_temperatures.values()))
        if filtered_temperatures:            
            self.thc.update(filtered_temperatures)
            #print(list(current_temperatures.values()))
            if self.current_heater_output_control != self.thc.get_output_control(): # appena cambia il comando logico all'heater, allora invio il comando ad Arduino            
                self.current_heater_output_control = self.thc.get_output_control()
                self.queue_command("HTR01", self.current_heater_output_control)  # @<HTR01, True># @<HTR01, False>#
            else:
			# caso in cui non ho NEMMENO UNA temperatura valida, tutti in errore. Allora spengo il riscaldatore.
                self.queue_command("HTR01", False)  # @<HTR01, True># @<HTR01, False>#
			# metto un contatore: se questa cosa è vera per più di 20 ccili, allora off
			# FILTRAGGIO?? NON VORREI CHE CI SINAO DEGLI SPIKE...magari  se sto in questo else per più di un tot di secondi, allora dò il false, significa che l'errore è probabile che sia persistente.
			
        # Check for persistent errors
        #print(current_temperatures)
        self.check_errors(current_temperatures)
                
            
            
        # HUMIDITY CONTROLLER SECTION
        if current_humidities:
            self.hhc.update(list(current_humidities.values()))
            if self.current_humidifier_output_control != self.hhc.get_output_control():
                self.current_humidifier_output_control = self.hhc.get_output_control()
                self.queue_command("HUMER01", self.current_humidifier_output_control) # @<HUMER01, True># @<HUMER01, False>#
            
            
        # Emit the data to update the view        
        # Collecting all values into a single list
        # questo all_values è semplicemente una lista [17.8, 17.9, 18.0, 17.9, 17.8, 17.9] dove SO IO ad ogni posto cosa è associato...passiamo solo i valori (non bellissimo...)
        # i mean value servono per pubblicare il valore che il controllore usa per fare effettivamente il controllo e lo metto nella sezione di isteresi
        # list() se passi un dizionario, mentre [] se vuoi aggiugnere alla lista elementi singoli
        if current_temperatures and current_humidities:
            # all_values for MAIN VIEW page
            all_values = []
            all_values = list(current_temperatures.values()) + \
                        list(current_humidities.values()) + \
                        list(current_humidities_temperatures.values()) + \
                        [self.thc.get_mean_value()] + \
                        [self.hhc.get_mean_value()] + \
						[self.thc.get_output_control()] # pubblico l'heater status
                        
            self.update_view.emit(all_values)
            
            # all_values for STATISTICS page
            all_values = []
            all_values.append(self.thc.get_min_value())
            all_values.append(self.thc.get_mean_value())
            all_values.append(self.thc.get_max_value())
            all_values.append(self.thc.get_on_count())
            all_values.append(self.thc.get_off_count())
            all_values.append(self.thc.get_time_on())
            all_values.append(self.thc.get_time_off())
            
            all_values.append(self.hhc.get_min_value())
            all_values.append(self.hhc.get_mean_value())
            all_values.append(self.hhc.get_max_value())
            all_values.append(self.hhc.get_on_count())
            all_values.append(self.hhc.get_off_count())
            all_values.append(self.hhc.get_time_on())
            all_values.append(self.hhc.get_time_off())
            
            self.update_statistics.emit(all_values)
        
        # INDUCTOR SECTION
        """
            IND_CCW - IND_CW induttori posizioni limite counterClockWise - clockWise
            IND_HOR induttore posizionamento orizzontale
            IND_DR door (apertura/chiusura della porta)
        """
        if current_inductors_feedbacks: # that is checking if the dictionary is not empty
            #print(current_inductors_feedbacks)
            
            value = current_inductors_feedbacks.get("IND_CCW") # to handle cases where the key might not exist safely, you can use .get() - questa funzione tira fuori il valore, non la key!
            if value is not None and value == 1:
                # perché nella mia convenzione value == 1 significa rising edge della posizione ragginuta
                self.eggTurnerMotor.acknowledgeFromExternal("IND_CCW")
                
            value = current_inductors_feedbacks.get("IND_CW") # to handle cases where the key might not exist safely, you can use .get()
            if value is not None and value == 1:
                self.eggTurnerMotor.acknowledgeFromExternal("IND_CW")
        
        
        
             
        
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
            
            self.reset_timer_to_do = True
            self.measuring_time_interval_sec = 1 # ogni n secondi aggiorno i conteggi di tempo on/off dell'attuatore
            self.last_measuring_time = time.time() # si ricorda appena ho preso il campione precedente
            self.effective_time_difference = 0.0
            # questi sono i contatori di timer
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
            # reset timers all'avvio
            if self.reset_timer_to_do:
                self.last_measuring_time = time.time()
                self.reset_timer_to_do = False
                
                
            # Safety check: If limits are equal, force OFF state
            if self.lower_limit == self.upper_limit:
                self._output_control = False
                return
        
            # Ensure values is a list for consistency
            if not isinstance(values, list):
                values = [values]
            
            # Calculate the mean of the values
            self._mean_value = round(sum(values) / len(values), 1)
            
            #print(f"Mean Value: {self._mean_value}")
            
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
                
            # update time tracking (continuous)
            elapsed_time = time.time() - self.last_measuring_time
            if elapsed_time >= self.measuring_time_interval_sec:
                if self._output_control:
                    self.time_on += elapsed_time
                else:
                    self.time_off += elapsed_time
                self.last_measuring_time = time.time()
                
            # If state changed, update count transitions
            if new_output != self._output_control:
                elapsed_time = time.time() - self.last_measuring_time
                if self._output_control:
                    self.time_on += elapsed_time
                    self.off_count += 1
                else:
                    self.time_off += elapsed_time
                    self.on_count += 1
                
                self._output_control = new_output
                self.last_measuring_time = time.time()
                
            
                
        
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
            return round(self.time_on, 0)

        def get_time_off(self):
            return round(self.time_off, 0)

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
            
    class StepperMotor:
        def __init__(self, name="Stepper1"):
            self.name = name
            self.running = False
            
            """
                self.main_state:
                1 = INITIALIZATION
                10 MANUAL   _MODE
                30 AUTOMATIC_MODE
            
            """
            self.main_state = 1
            
            self.automatic_state = 1
            
            self.manual_state = 1
            
            '''
                Vogio gestire tutto con una variabile...anziché avere direzione e movimento, ne ho una sola
                Il tribolo è lo stop. Not moving, ma in che direzione? allora faccio due valori della variabile, che sono loro che si ricordano
                da dove mi stavo fermando
                stopped_from_CCW_rotation_direction
                stopped_from_CW_rotation_direction
                
                self.rotation_state 
                not_defined
                CCW_reached
                CW_reached
                CCW_rotation_direction
                CW_rotation_direction
                laying_horizontal_position
                
            '''
            self.rotation_state = "not_defined"
            
            
            self.last_execution_time = time.time()
            self.auto_function_interval_sec = 3600  # Default to 1 hour
            self.last_ack_time = None
            self.alarm_triggered = False
            self.turnsCounter = 0
            
            self.stop_command = False
            
            self.new_command = None
            self.update_motor_data = False
            self.acknowledge_from_external = None            
            
            self.force_change_rotation_flag = False
            
            self.rotation_in_progress_timeout_sec = 120 # 2min timeout
            
            # TIMING
            self.last_motor_data_update_sec = time.time()
            self.motor_data_update_interval_sec = 15 # indica ogni quanti secondi viene fatto l'update dei dati relativi al motore

        def moveCWContinuous(self):
            self.main_state = 10
            self.manual_state = 20
            print(f"{self.name}: Moving cw continuously.")

        def moveCCWContinuous(self):
            self.main_state = 10
            self.manual_state = 30
            print(f"{self.name}: Moving ccw continuously.")

        def stop(self):
            self.stop_command = True
            print(f"{self.name}: Stopping motor.")
        
        def setFunction1(self):
            self.main_state = 30
            self.last_ack_time = time.time()
            self.alarm_triggered = False
            print(f"{self.name}: Automatic function 1 enabled.")
        
        def stopFunction1(self):
            self.main_state = 0
            print(f"{self.name}: Stopping automatic function 1.")
        
        def setFunctionInterval(self, interval):
            self.auto_function_interval_sec = interval
            print(f"{self.name}: Automatic function interval set to {interval} seconds.")
        
        def acknowledgeFromExternal(self, ack):
            self.last_ack_time = time.time()
            self.acknowledge_from_external = ack  
            print(f"Acknowledge received from external:{ack}")          
            
        def resetNewCommand(self):
            self.new_command = None
            
        def resetUpdateMotorData(self):
            self.update_motor_data = None
        
        def getRotationDirection(self):
            return self.rotation_direction
        
        def getTimeSinceLastRotation(self):
            return round((time.time() - self.last_execution_time) / 60, 1)# in minutes
        
        def getTimeUntilNextRotation(self):
            return round(max(0, (self.auto_function_interval_sec - (time.time() - self.last_execution_time)) / 60), 1) # in minutes
        
        def getTurnsCounter(self):
            return self.turnsCounter
        
        def getUpdateMotorData(self):
            return self.update_motor_data
        
        def getNewCommand(self):
            return self.new_command
        
        def forceEggsRotation(self):
            self.force_change_rotation_flag = True
            self.main_state = 30 # go back in automatic, if you were in manual
        
        def update(self):
            current_time = time.time()
            
            if self.main_state == 1: # INITIALIZATION
                if self.acknowledge_from_external is not None:                    
                    if self.acknowledge_from_external == "IND_CCW": 
                        self.rotation_state = "CCW_reached"
                        print("Homing done. Reached the Counter Clock Wise limit switch")
                        self.acknowledge_from_external = None # reset
                        
                    if self.acknowledge_from_external == "IND_CW":
                        self.rotation_state = "CW_reached"
                        print("Homing done. Reached the Clock Wise limit switch")                        
                        self.acknowledge_from_external = None # reset
                        
                    self.last_execution_time = current_time # salvo il tempo, perché così la prossima FULL TURN avviene contando il tempo da quando ho finito l'homing
                    self.main_state = 30 # di default andiamo in modalità automatica
                else:
                    pass
                
            elif self.main_state == 10: # MANUAL MODE
                    if self.manual_state == 1: #waiting for command
                        pass
                        
                    if self.manual_state == 20: # CW continuous moving
                        self.rotation_state = "CW_rotation_direction"
                        self.new_command = "manual_" + self.rotation_state
                        self.manual_state = 50 
                        
                    if self.manual_state == 30: # CCW continuous moving
                        self.rotation_state = "CCW_rotation_direction"
                        self.new_command = "manual_" + self.rotation_state
                        self.manual_state = 50 
                        
                    if self.manual_state == 50: # rotation in progress
                        # IF ack from limit switch --> comunica che è arrivato ack, ma di fatto si ferma da solo per Arduino
                        if self.acknowledge_from_external is not None:         
                            if self.acknowledge_from_external == "IND_CCW" and self.rotation_state == "CCW_rotation_direction":
                                self.rotation_state = "CCW_reached"
                                print("Reached the IND_CCW limit switch")
                                self.acknowledge_from_external = None # reset
                                self.manual_state = 1
                                
                            if self.acknowledge_from_external == "IND_CW" and self.rotation_state == "CW_rotation_direction":
                                self.rotation_state = "CW_reached"
                                print("Reached the IND_CW limit switch")
                                self.acknowledge_from_external = None # reset
                                self.manual_state = 1
                        
                        # IF stop command, then stop
                        if self.stop_command:
                            self.manual_state = 60
                            self.stop_command = False
                        
                    if self.manual_state == 60: # STOPPING
                        self.new_command = "stop"
                        
                        if self.rotation_state == "CCW_rotation_direction":
                            self.rotation_state = "stopped_from_CCW_rotation_direction"
                            
                        if self.rotation_state == "CW_rotation_direction":
                            self.rotation_state = "stopped_from_CW_rotation_direction"
                            
                        self.manual_state = 1
            
            elif self.main_state == 30: # AUTOMATIC MODE                          
                if (current_time - self.last_execution_time >= self.auto_function_interval_sec or self.force_change_rotation_flag):
                    
                    if self.rotation_state == "CCW_reached" or self.rotation_state == "CCW_rotation_direction" or self.rotation_state == "stopped_from_CCW_rotation_direction":
                        self.rotation_state = "CW_rotation_direction"
                        
                    elif self.rotation_state == "CW_reached" or self.rotation_state == "CW_rotation_direction" or self.rotation_state == "stopped_from_CW_rotation_direction":
                        self.rotation_state = "CCW_rotation_direction"
                        
                    print(f"{self.name}: Changing rotation direction to {self.rotation_state}.")                    
                    self.last_execution_time = current_time
                    self.turnsCounter += 1
                    self.new_command = "automatic_" + self.rotation_state
                    print(f"{'Activating' if (self.rotation_state == "CW_rotation_direction" or self.rotation_state == "CCW_rotation_direction") else 'Deactivating'} diagnostics")
                    
                # ogni tot diciamo che è passato il tempo per fare un update dei dati del motore: il programma, da fuori, riceve il segnale e prende i dati
                #print(time.time() - self.last_motor_data_update_sec)
                if ((time.time() - self.last_motor_data_update_sec) >= self.motor_data_update_interval_sec # update periodico
                    or 
                    self.force_change_rotation_flag # forzatura dell'update quando forzo un giro a mano
                    or
                    (self.new_command is not None and "automatic_" in self.new_command) # forzatura dell'update se non è scaduto il tempo ma se è scattato il comando di turn
                    ):
                    self.update_motor_data = True
                    self.last_motor_data_update_sec = time.time()
                    
                    
                if self.force_change_rotation_flag:
                    self.force_change_rotation_flag = False # resetting
                    
                    
                if (self.rotation_state == "CCW_rotation_direction" or self.rotation_state == "CW_rotation_direction"):
                    # diagnostic: attesa del feedback
                    if (time.time() - self.last_execution_time) >= self.rotation_in_progress_timeout_sec:
                        # manda comando di STOP
                        self.new_command = "stop"
                        
                    if self.acknowledge_from_external is not None:         
                        if self.acknowledge_from_external == "IND_CCW" and self.rotation_state == "CCW_rotation_direction":
                            self.rotation_state = "CCW_reached"
                            print(f"{'Activating' if (self.rotation_state == "CW_rotation_direction" or self.rotation_state == "CCW_rotation_direction") else 'Deactivating'} diagnostics")
                            print("Reached the IND_CCW limit switch")
                            self.acknowledge_from_external = None # reset
                            
                        if self.acknowledge_from_external == "IND_CW" and self.rotation_state == "CW_rotation_direction":
                            self.rotation_state = "CW_reached"
                            print(f"{'Activating' if (self.rotation_state == "CW_rotation_direction" or self.rotation_state == "CCW_rotation_direction") else 'Deactivating'} diagnostics")
                            print("Reached the IND_CW limit switch")
                            self.acknowledge_from_external = None # reset
                            
                else:
                    pass
                
            print(f"{self.main_state} {self.manual_state} {self.rotation_state}")
                            
                        
                    
                
                
            '''
            if self.last_ack_time and (current_time - self.last_ack_time > 180):  # 3 minutes timeout
                if not self.alarm_triggered:
                    self.stop()
                    print(f"{self.name}: ERROR! No acknowledgment received for 2 minutes. Stopping motor and raising alarm!")
                    self.alarm_triggered = True
            '''
            
            if self.acknowledge_from_external is not None:   
                '''
                    I feedback degli induttori sono msi asincroni dal process serial data dentro qusta variabile. Se arriva per qualche motivo mentre
                    sono in uno stato dove non me lo aspetto, allora non viene resettato, rimane in variabile e dopo apena entro in modalità automatica
                    la variabile l'ha ancora in pancia e quindi dà una falsa lettura. Allora, alla fine del ciclo, se non l'ho usato, lo resetto.
                '''                  
                self.acknowledge_from_external = None # reset
           

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
        self.main_software_thread.update_statistics.connect(self.update_statistics_data)
        self.main_software_thread.update_motor.connect(self.update_display_motor_data)
        
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
        self.ui.reset_statistics_T_btn.clicked.connect(lambda: self.emit_button_signal(self.ui.reset_statistics_T_btn.objectName()))
        
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
            #self.ui.temperature4_2.setText(f"{all_data[5]} °C") PER TEMPERATURA DA UMIDITA
            self.ui.heatCtrlVal.setText(f"{all_data[6]} °C")
            self.ui.humCtrlVal.setText(f"{all_data[7]} °C")
            if {all_data[8]} == True:
                self.ui.heaterStatus.setText(f"Heating ON!")
            else:
                self.ui.heaterStatus.setText(f"OFF")
            #self.ui.temperature4_2.setText(f"{all_data[3]} °C") PER TEMPERATURA DA UMIDITA
            
    def update_statistics_data(self, all_data):
        # da implementare la parte di update delle statistiche
        if len(all_data) > 0:
            self.ui.minTemp_T.setText(f"{all_data[0]} °C")
            self.ui.meanTemp_T.setText(f"{all_data[1]} °C")
            self.ui.maxTemp_T.setText(f"{all_data[2]} °C")
            self.ui.onCounter_T.setText(f"{all_data[3]}")
            self.ui.offCounter_T.setText(f"{all_data[4]}")
			# VISUALIZZAZIONE DEI TEMPI: il programma di base mi manda dei secondi. E' qui che stampo la stringa opportunamente in min o h
            self.ui.timeOn_T.setText(self.format_time(all_data[5]))
            self.ui.timeOFF_T.setText(self.format_time(all_data[6]))
            self.ui.minHum_H.setText(f"{all_data[7]} %")
            self.ui.meanHum_H.setText(f"{all_data[8]} %")
            self.ui.maxHum_H.setText(f"{all_data[9]} %")
            self.ui.onCounter_H.setText(f"{all_data[10]}")
            self.ui.offCounter_H.setText(f"{all_data[11]}")
            self.ui.timeOn_H.setText(f"{all_data[12]} s")
            self.ui.timeOFF_H.setText(f"{all_data[13]} s")
            
        pass
	
    def format_time(value):
        if value < 60:
            return(f"{value} sec")
        elif value < 3600:
            minutes = value // 60
            seconds = value % 60
            return(f"{minutes} min {seconds} sec" if seconds else f"{minutes} min")
        else:
            hours = value // 3600
            minutes = (value % 3600) // 60
            return(f"{hours}h {minutes}min" if minutes else f"{hours}h")
            
    def update_display_motor_data(self, all_data):
        if len(all_data) > 0:
            self.ui.timePassed.setText(f"{all_data[0]} min")
            self.ui.timeToNextTurn.setText(f"{all_data[1]} min")
            self.ui.turnsCounter.setText(f"{all_data[2]}")

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
