'''
With new identifiers.
'''

import matplotlib
from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
import random
import time
import threading
import queue
import serial
import re
import psutil  # Import psutil for system statistics
import easygui
import os
import json
import csv
import shutil
from datetime import datetime, timedelta
import zipfile

import pandas as pd
import matplotlib.pyplot as plt
import webbrowser
import platform
import subprocess
import sys
import logging

# Configure the serial port
port = "COM6"
#port = "/dev/ttyUSB0"
port = "/dev/ttyACM0"
baudrate = 19200
timeout = 0.1

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

temperature_queue = queue.Queue()
humidity_queue = queue.Queue()

dataQueueFromArduino = queue.Queue() # contiene sia temperature che umidità:
'''
è fondamentale rispettare questo ordine. Il riempimento dei dizionari lo segue!

["TMP01", "TMP02", "TMP03", "TMP04", "HUM01", "HTP01"]

qui definisco solo gli identificatori che devono essere riuniti in gruppi
'''

identifiers = ["TMP", "HUM", "HTP"]

matplotlib.use('Agg')


@app.route('/')
def index():
    return render_template('index.html')


@app.route('/machine_stats')
def machine_stats():
    return render_template('machine_stats.html')


def monitoringArduino_task():
    global simulationActive
    if simulationActive:
        # l'idea è pubblicare le temperature sulla temperature_queue ogni 2 secondi
        try:
            while True:
                # Simulate data coming from Arduino
                combined_data_temp = {}
                for item in range(0, 3):
                    sensor_key = f'TMP0{item + 1}'
                    combined_data_temp[sensor_key] = round(random.uniform(200, 300) / 10, 2)
                temperature_queue.put(combined_data_temp)
                
                combined_data_humidity = {}
                for item in range(0, 1):
                    sensor_key = f'HUM0{item + 1}'
                    combined_data_humidity[sensor_key] = round(random.uniform(100, 800) / 10, 2)
                humidity_queue.put(combined_data_humidity)
                time.sleep(2)
        except Exception as e:
            print(f"An error occurred: {e}")
    else:
        try:
            ser = serial.Serial(port, baudrate)
            print("Serial port opened successfully")
            while True:
                read_from_arduino(ser)
        except serial.SerialException as e:
            print(f"Failed to open serial port: {e}")
        except KeyboardInterrupt:
            print("Program interrupted")
        finally:
            try:
                ser.close()
                print("Serial port closed")
            except Exception as e:
                print(f"Error closing serial port: {e}")


def read_from_arduino(serial_port):
    buffer = ''
    startTransmission = False
    saving = False
    dataDict = {}
    identifiers_data_list = []
    
    combined_data = {} #dizionario che lascio qui per inizare a collezionare i dati. Lo pubblico sulla queue appena ho raccolto tutto da arduino

    while True:
        try:
            if serial_port.in_waiting > 0:
                dataRead = serial_port.read().decode('utf-8')
                if dataRead == '@':
                    startTransmission = True
                if startTransmission:
                    if dataRead == '<':
                        saving = True
                    if saving:
                        buffer += dataRead
                    if dataRead == '>':
                        saving = False
                        decodeMessage(buffer, identifiers_data_list)
                        buffer = ''                        
                            
                    if dataRead == '#':
                        startTransmission = False
                        
                        for item in identifiers_data_list:
                            combined_data.update(item)
                        identifiers_data_list.clear()
                        
                        
                        dataQueueFromArduino.put(combined_data) # pubblico sulla queue alla fine di tutta la ricezione
        except UnicodeDecodeError as e:
            print(f"Decode error: {e}")
            # Clear the input buffer to avoid repeated errors
            serial_port.reset_input_buffer()
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            break


def is_number(s):
    try:
        float(s)  # Try to convert to float
        return True
    except ValueError:
        return False


def decodeMessage(buffer, identifiers_data_list):
    match = re.match(r"<([^,]+),([^>]+)>", buffer)
    if match:
        infoName_part = match.group(1)
        infoData_part = match.group(2)

        if is_number(infoData_part):
            # Convert the number part to float or int
            if '.' in infoData_part:
                number = float(infoData_part)
            else:
                number = int(infoData_part)

            
            # Questi sono quelli che vengono messi nella struttura standard
            global identifiers
            for identifier in identifiers:
                if infoName_part.startswith(identifier):
                    identifiers_data_list.append({infoName_part: number})
                    break
            # lascio aperta la possibilità che se qualcosa non è in lista identifiers viene gestito a mano qui.

            # AGGIUNGERE QUI ALTRE INFO CHE VENGONO DA ARDUINO, ma ad occhio direi di metterle tutte dentro #
        else:
            ciao = False
            # print("Info Name part:", infoName_part)
            # print("Info Data part:", infoData_part)
    else:
        print("The input string does not match the expected format.")

def save_data_to_files(data_type, data_dictionary): #passo un dictionary di temperature/humidities, dimensione variabile per gestire più o meno sensori dinamicamente
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


def periodic_task():
    while True:
        try:
            # Perform other actions here
            # print("Periodic task is doing other things...")

            '''
            Appena arrivano nuove temperature, chiama la funzione che controlla tutta la parte di temperature e heaters.
            + fa memorizzazione dei dati nei file esterni.
            combo box per la modalità di controllo (temperatura massima, media...)
            + fai esclusione delle temperature non lette correttamente, se ci sono.
            in uscita avrai unicamente i comandi di on/off degli attuatori. (le tempistiche le gestisci qui...)

            L'invio dei comandi ad arduino lo voglio mantenere asyncrono: stampo i comando dove ne ho bisogno (solo nel periodic task...concorrenza...)


            chiamata alla funzione ceh controlla lo stepper (guarda se deve girare e se si manda il comando). Controllo parametri
            impostati a video + manuale + lay_horizontal


            '''

            # Check for new data in the queue
            if not dataQueueFromArduino.empty(): 
                # GETTING DATA FROM QUEUE
                dataQueueFromArduinoDict = dataQueueFromArduino.get()
                #print(dataQueueFromArduinoDict)
                
                # Liste di dati da accumulare
                currentTemperatures = {}
                currentHumidities = {}
                currentHumiditiesTemperature = {} # lista che contiene le temperature dai DHT22
                
                # Loop through the first 4 items of the original dictionary and add them to the new dictionary
                for i, (key, value) in enumerate(dataQueueFromArduinoDict.items()):
                    for identifier in identifiers:
                        if key.startswith("TMP"): 
                            currentTemperatures[key] = value
                        elif key.startswith("HUM"):
                            currentHumidities[key] = value
                        elif key.startswith("HTP"):
                            currentHumiditiesTemperature[key] = value
                        else:
                            break  
                '''
                print("currentTemperatures:")
                print(currentTemperatures)
                
                print("currentHumidities:")
                print(currentHumidities)
                
                print("currentHumiditiesTemperature:")
                print(currentHumiditiesTemperature)
                '''
                # END GETTING DATA FROM QUEUE  
                
                             
                
                # TEMPERATURE & HUMIDITY HANDLING - [PERIODIC TASK]
                
                # END TEMPERATURE & HUMIDITY HANDLING - [PERIODIC TASK]       
                
                
                # STATISTICS- [PERIODIC TASK]
                temperatureTracker.update_temperatures(currentTemperatures)
                #humidity_statistics(currentHumidities)
                #timing_statistics()
                #egg_statistics()
                # END STATISTICS - [PERIODIC TASK]         
                
                
                
                # SAVING DATA IN FILES
                save_data_to_files('Temperatures', currentTemperatures) #{'TMP01': 23.1, 'TMP02': 23.1, 'TMP03': 23.1}
                save_data_to_files('Humidity', currentHumidities) #{'HUM01': 52.5}

                # SENDING DATA TO WEB PAGES
                if current_page == 'index':                   
                    data = {}
                    data.update(currentTemperatures)
                    data.update(currentHumidities)
                    data.update(currentHumiditiesTemperature)
                    
                    data.update({'mainHeater': random.choice([True, False])})
                    data.update({'auxHeater': random.choice([True, False])})
                    data.update({'machineState': random.randint(0, 5)})

                    data.update({'sendHigherHysteresisLimit_currentValue': configuration["tmp_higherHysteresisLimit"]})
                    data.update({'sendLowerHysteresisLimit_currentValue': configuration["tmp_lowerHysteresisLimit"]})
                    
                    # Send data to HTML page
                    socketio.emit('data_update', data)
                
                elif current_page == 'machine_stats':
                    data = {}
                    data.update(currentTemperatures)
                    data.update(currentHumidities)
                    data.update(currentHumiditiesTemperature)
                    
                    data.update({'minTemperature': temperatureTracker.get_min_temp()})
                    data.update({'meanTemperature': temperatureTracker.get_mean_temp()})
                    data.update({'maxTemperature': temperatureTracker.get_max_temp()})
                    data.update({'absoluteMinTemperature': temperatureTracker.get_abs_min_temp()})
                    data.update({'absoluteMaxTemperature': temperatureTracker.get_abs_max_temp()})
                    
                    # timing shold be give in  seconds (HTML function that handles the conversion in hh:mm:ss)
                    data.update({'timeLastTurn': random.randint(20, 300)})
                    data.update({'numberOfEggTurns': random.randint(80, 100)})
                    data.update({'elapsedTimeFromLastTurn': random.randint(2500, 7200)})
                    data.update({'missingTimeToNextTurn': random.randint(10000, 50000)})
                    
                    # Send data to HTML page
                    socketio.emit('machine_stats', data)

                    
                
        except Exception as e:
            print(f"Error in periodic_task: {e}")


def send_async_messages_task_fnct():
    colors = ['red', 'orange', 'yellow', 'green', 'blue']
    messages = [
        "Critical: System failure detected!",
        "Warning: High temperature recorded.",
        "Note: System maintenance required.",
        "Info: All systems operational."
    ]
    priorities = {
        'red': 0,
        'orange': 1,
        'yellow': 2,
        'green': 3,
        'blue': 4
    }
    while True:
        message = random.choice(messages)
        color = random.choice(colors)
        socketio.emit('async_message', {'text': message, 'color': color})
        time.sleep(random.randint(5, 15))


def send_async_messages_task_fnct_FILO():
    while True:
        message = "Messaggio da funzione Aync Di Filo"
        color = 'red'
        socketio.emit('async_message', {'text': message, 'color': color})
        time.sleep(10)
        
def web_page_initialization():
    # funzione che chiamo una volta all'avvio per inviare i dati che ho appena caricato alla pagina web di controllo, in particolare i valori delle selezioni dei controlli:
    # Send data to HTML page
    data = {}
    data.update({'flag1': configuration["flag1"]})
    data.update({'flag2': configuration["flag2"]})
    data.update({'temperatureControlOption': configuration["temperatureControlOption"]}) # maxValueOption meanValueOption minValueOption
    socketio.emit('initial_state', data)
    print(data)
    return True

@socketio.on('page_load')
def handle_page_load(data):
    print(data)
    global current_page
    current_page = data['page']
    
    # ogni volta in cui carico la pagina, voglio inviare nuovamente la configurazione per visualizzare correttamente le selezioni.
    if current_page == 'index': 
        web_page_initialization();
        
        
@socketio.on('command')
def handle_command(data):
    print(f"Received command: {data['cmd']} from button {data['button']}")
    if data['cmd'] == 'start':
        # Handle start command
        print("Handling start command")
    elif data['cmd'] == 'stop':
        # Handle stop command
        print("Handling stop command")
    elif data['cmd'] == 'reset':
        # Handle reset command
        print("Handling reset command")
        socketio.emit('clear_messages')
    elif data['cmd'] == 'load_parameters':
        print("Handling load_parameters command")
        select_file()
    elif data['cmd'] == 'save_parameters':
        print("Handling save_parameters command")
        global configuration
        print(configuration)
        save_current_parameter_configuration()
        folder = './currentConfiguration'
        # Open the most recent file in the default text editor
        try:
            open_most_recent_file_in_editor(folder)
        except FileNotFoundError as e:
            print(e)
        except NotImplementedError as e:
            print(e)
    elif data['cmd'] == 'show_parameters':
        print("Handling show_parameters command")
        # Define the folder
        folder = './currentConfiguration'

        # Open the most recent file in the default text editor
        try:
            open_most_recent_file_in_editor(folder)
        except FileNotFoundError as e:
            print(e)
        except NotImplementedError as e:
            print(e)
        


@socketio.on('number')
def handle_number(data):
    number = data.get('number')
    print(f"Received number: {number}")
    # Handle the number as needed
    emit('message', {'text': f'Received number: {number}', 'color': 'blue'})


@socketio.on('higherHysteresisLimit')
def handle_higher_hysteresis_limit(data):
    higher_hysteresis_limit = float(data.get('higherHysteresisLimit'))
    global configuration
    configuration["tmp_higherHysteresisLimit"] = higher_hysteresis_limit    
    
    print(f"Received higher hysteresis limit: {higher_hysteresis_limit}")
    # Handle the higher hysteresis limit as needed
    emit('message', {'text': f'Received higher hysteresis limit: {higher_hysteresis_limit}', 'color': 'green'})


@socketio.on('lowerHysteresisLimit')
def handle_lower_hysteresis_limit(data):
    lower_hysteresis_limit = float(data.get('lowerHysteresisLimit'))
    global configuration
    configuration["tmp_lowerHysteresisLimit"] = lower_hysteresis_limit
    
    print(f"Received lower hysteresis limit: {lower_hysteresis_limit}")
    # Handle the lower hysteresis limit as needed
    emit('message', {'text': f'Received lower hysteresis limit: {lower_hysteresis_limit}', 'color': 'orange'})


@socketio.on('flag1')
def handle_flag(data):
    flag1 = data.get('flag1', False)
    global configuration
    configuration["flag1"] = flag1  
    # Handle the flag logic here
    print(f'Flag1: {flag1}')


@socketio.on('flag2')
def handle_flag(data):
    flag2 = data.get('flag2', False)
    global configuration
    configuration["flag2"] = flag2
    # Handle the flag logic here
    print(f'Flag2: {flag2}')


@socketio.on('option')
def handle_option(data):
    option = data.get('option', '')
    global configuration
    configuration["temperatureControlOption"] = option
    # Handle the option logic here
    print(f'Option selected: {option}')
    
    
# SOCKETIO REQUESTS FROM MACHINE STATISTICS PAGE #
@socketio.on('refresh_absolute_temperatures')
def handle_request_stats():
    temperatureTracker.reset_absolute_extremes_to_mean() # problema di concorrenza con periodic task
    print("refresh_absolute_temperatures")
    
@socketio.on('plot_allDaysData_Temperatures')
def handle_request_stats():
    print("plot_allDaysData_Temperatures")
    #command = ['C:/Users/pietr/PycharmProjects/pythonProject/.venv/Scripts/python.exe', 'provaMatplotlib_inetractive.py', 'PLOT_ALL_DAYS_DATA_TEMPERATURES']
    command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_TEMPERATURES']
    process = subprocess.Popen(command)
    print("Subprocess started and main program continues...")

@socketio.on('plot_currentDayData_Temperatures')
def handle_request_stats():
    print("plot_currentDayData_Temperatures")
    command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_TEMPERATURES']
    process = subprocess.Popen(command)
    print("Subprocess started and main program continues...")

@socketio.on('plot_allDaysData_Humidity')
def handle_request_stats():
    print("plot_allDaysData_Humidity")
    command = ['python3', 'appInteractivePlots.py', 'PLOT_ALL_DAYS_DATA_HUMIDITY']
    process = subprocess.Popen(command)
    print("Subprocess started and main program continues...")

@socketio.on('plot_currentDayData_Humidity')
def handle_request_stats():
    print("plot_currentDayData_Humidity")
    command = ['python3', 'appInteractivePlots.py', 'PLOT_CURRENT_DAY_DATA_HUMIDITY']
    process = subprocess.Popen(command)
    print("Subprocess started and main program continues...")


def process_number(number):
    # Implement your logic to process the number
    print(f"Processing number: {number}")


def process_higher_hysteresis_limit(limit):
    # Implement your logic to process the higher hysteresis limit
    print(f"Processing higher hysteresis limit: {limit}")


def process_lower_hysteresis_limit(limit):
    # Implement your logic to process the lower hysteresis limit
    print(f"Processing lower hysteresis limit: {limit}")


def send_async_message(message, color):
    '''
    colors = ['red', 'orange', 'yellow', 'green', 'blue']
    messages = [
        "Critical: System failure detected!",
        "Warning: High temperature recorded.",
        "Note: System maintenance required.",
        "Info: All systems operational."
    ]
    '''
    socketio.emit('async_message', {'text': message, 'color': color})


def select_file():
    file_path = easygui.fileopenbox()

    # Check if a file was selected
    if file_path:
        # Check if the file has a .json extension
        if file_path.endswith('.json'):
            print(f"Selected file: {file_path}")
            send_async_message(f"Selected file: {file_path}", 'blue')
            global configuration
            configuration = load_json_file(file_path)
        else:
            print("Error: Selected file is not a .json file.")
            send_async_message("Error: Selected file is not a .json file.", 'red')
    else:
        print("No file selected")
        send_async_message("No file selected", 'red')


# ---- HANDLING PERSISTENT PARAMETERS ----#
def save_current_parameter_configuration():
    # Get the current time formatted as a string
    now = datetime.now()
    timestamp = now.strftime('%Y%m%d_%H%M%S')

    # Define the folder and file name
    folder = './currentConfiguration'
    os.makedirs(folder, exist_ok=True)  # Create the folder if it doesn't exist
    file_name = f'currentConfiguration_{timestamp}.json'
    file_path = os.path.join(folder, file_name)

    # Save the dictionary to a JSON file
    global configuration
    with open(file_path, 'w') as json_file:
        json.dump(configuration, json_file, indent=4)

    print(f"Configuration saved to {file_path}")

def load_json_file(file_path):
    with open(file_path, 'r') as f:
        current_content = json.load(f)
        print(current_content)
        return current_content


def get_most_recent_file(folder_path):
    files = [f for f in os.listdir(folder_path) if f.endswith('.json')]
    if not files:
        return None
    files = [os.path.join(folder_path, f) for f in files]
    most_recent_file = max(files, key=os.path.getmtime)
    return most_recent_file


def load_configuration_startup():
    ''' 
        Questa è la funzione di startup nel senso che è quella che viene chiamata all'avvio.    
        Prima guarda se ci sono dei parametri salvati nella cartella currentConfig, se non ce
        ne sono allora passa a caricare il primo file nella cartella di defaultConfig.
    '''
    default_config_folder = 'defaultConfiguration'
    current_config_folder = 'currentConfiguration'

    # Check if currentConfiguration folder is empty
    if not os.listdir(current_config_folder):
        print("currentConfiguration folder is empty. Loading from defaultConfiguration.")
        # Load a file from defaultConfiguration
        default_files = [f for f in os.listdir(default_config_folder) if f.endswith('.json')]
        if not default_files:
            raise FileNotFoundError("No JSON files found in defaultConfiguration folder.")
        # Load the first file (or you can implement logic to select a specific file)
        default_file_path = os.path.join(default_config_folder, default_files[0])
        return load_json_file(default_file_path), default_file_path
    else:
        # Load the most recent JSON file from currentConfiguration
        print("Loading the most recent JSON file from currentConfiguration.")
        most_recent_file = get_most_recent_file(current_config_folder)
        if not most_recent_file:
            raise FileNotFoundError("No JSON files found in currentConfiguration folder.")
        return load_json_file(most_recent_file), most_recent_file

def open_most_recent_file_in_editor(folder):
    most_recent_file = get_most_recent_file(folder)
    
    # Determine the platform and open the file in the default text editor
    if os.name == 'nt':  # For Windows
        os.startfile(most_recent_file)
    elif os.name == 'posix':  # For Linux and macOS
        opener = 'open' if sys.platform == 'darwin' else 'xdg-open'
        os.system(f'{opener} "{most_recent_file}"')
    else:
        raise NotImplementedError("Opening files is not implemented for this operating system.")
        
        

#--- OBJECT to implement local persistence ---#
class TemperatureTracker:
    def __init__(self):
        self.temp_dict = {}
        self.abs_min = None
        self.abs_max = None
    
    def update_temperatures(self, new_temp_dict):
        self.temp_dict = new_temp_dict
        self.update_absolute_extremes()
    
    def get_max_temp(self):
        if not self.temp_dict:
            return None
        return max(self.temp_dict.values())
    
    def get_min_temp(self):
        if not self.temp_dict:
            return None
        return min(self.temp_dict.values())
    
    def get_mean_temp(self):
        if not self.temp_dict:
            return None
        return sum(self.temp_dict.values()) / len(self.temp_dict)
        
    def get_abs_min_temp(self):
        if not self.abs_min:
            return None
        return self.abs_min
        
    def get_abs_max_temp(self):
        if not self.abs_max:
            return None
        return self.abs_max
        
    def update_absolute_extremes(self):
        if not self.temp_dict:
            self.abs_min = None
            self.abs_max = None
            return
        
        current_min = self.get_min_temp()
        current_max = self.get_max_temp()
        
        if self.abs_min is None or current_min < self.abs_min:
            self.abs_min = current_min
        if self.abs_max is None or current_max > self.abs_max:
            self.abs_max = current_max
    
    def reset_absolute_extremes_to_mean(self):
        mean_temp = self.get_mean_temp()
        self.abs_min = mean_temp
        self.abs_max = mean_temp


# --------- GLOBAL VARIABLES SECTION ---------#
# Global variable to store the current 
current_page = None


global simulationActive
simulationActive = False

# PERSISTENT VARIABLES SECTION # 
# Load all those variables being memorized in the configuration files (.json)
global configuration
configuration, filename = load_configuration_startup()
print(f"Loaded configuration from: {filename}")

'''
tmp_higherHysteresisLimit = configuration["tmp_higherHysteresisLimit"]
print(f"tmp_higherHysteresisLimit: {tmp_higherHysteresisLimit}")

tmp_lowerHysteresisLimit = configuration["tmp_lowerHysteresisLimit"]
print(f"tmp_lowerHysteresisLimit: {tmp_lowerHysteresisLimit}")

tmp_higherPercentageToActivateAuxiliaryHeater = configuration["tmp_higherPercentageToActivateAuxiliaryHeater"]
print(f"tmp_higherPercentageToActivateAuxiliaryHeater: {tmp_higherPercentageToActivateAuxiliaryHeater}")

tmp_lowerPercentageToActivateAuxiliaryHeater = configuration["tmp_lowerPercentageToActivateAuxiliaryHeater"]
print(f"tmp_lowerPercentageToActivateAuxiliaryHeater: {tmp_lowerPercentageToActivateAuxiliaryHeater}")

stpr_secondsBtwEggsTurn = configuration["stpr_secondsBtwEggsTurn"]
print(f"stpr_secondsBtwEggsTurn: {stpr_secondsBtwEggsTurn}")

stpr_defaultSpeed = configuration["stpr_defaultSpeed"]
print(f"stpr_defaultSpeed: {stpr_defaultSpeed}")
'''
print()

temperatureTracker = TemperatureTracker()

# --------- END GLOBAL VARIABLES SECTION ---------#

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




if __name__ == '__main__':    
    threading.Thread(target=monitoringArduino_task).start()
    threading.Thread(target=periodic_task).start()
    threading.Thread(target=send_async_messages_task_fnct).start()
    threading.Thread(target=send_async_messages_task_fnct_FILO).start()
    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True) #- per nuove versioni di flask socket IO
    
    
