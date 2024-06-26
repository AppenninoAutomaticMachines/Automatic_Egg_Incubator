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
from datetime import datetime



# Configure the serial port
port = "/dev/ttyUSB0"
baudrate = 19200
timeout = 0.1


app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

temperature_queue = queue.Queue()

@app.route('/')
def index():
    return render_template('index.html')
    
@app.route('/machine_stats')
def machine_stats():
    return render_template('machine_stats.html')

@socketio.on('request_stats')
def handle_request_stats():
    cpu_usage = psutil.cpu_percent(interval=1)
    memory_info = psutil.virtual_memory()
    memory_usage = memory_info.percent
    disk_usage = psutil.disk_usage('/').percent
    stats = {
        'cpu_usage': cpu_usage,
        'memory_usage': memory_usage,
        'disk_usage': disk_usage
    }
    emit('machine_stats', stats)

def monitoringArduino_task():
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
    temp_data_list = []
   
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
                        decodeMessage(buffer, temp_data_list)
                        buffer = ''
                        
                        if len(temp_data_list) == 3:
                            # se ho collezionato le 3 temperature
                            combined_data = {}
                            for item in temp_data_list:
                                combined_data.update(item)
                            temperature_queue.put(combined_data)
                            temp_data_list.clear()
                    if dataRead == '#':
                       startTransmission = False
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
        
def decodeMessage(buffer, temp_data_list):
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

            # Example actions based on infoName_part
            if infoName_part in ["TMP01", "TMP02", "TMP03"]:
                temp_data_list.append({infoName_part: number})
                
            # AGGIUNGERE QUI ALTRE INFO CHE VENGONO DA ARDUINO #
        else:
            ciao = False
            #print("Info Name part:", infoName_part)
            #print("Info Data part:", infoData_part)  
    else:
        print("The input string does not match the expected format.")


def periodic_task():
    while True:
        try:
            # Perform other actions here
            #print("Periodic task is doing other things...")
            
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
            if not temperature_queue.empty():
                data = temperature_queue.get()
                
                # Process the temperature data
                data.update({'mainHeater': random.choice([True, False])})
                data.update({'auxHeater': random.choice([True, False])})
                data.update({'machineState': random.randint(0, 5)})
                
                data.update({'sendHigherHysteresisLimit_currentValue': random.randint(0, 5)})
                data.update({'sendLowerHysteresisLimit_currentValue': random.randint(0, 5)})
                # Send data to HTML page
                socketio.emit('data_update', data)
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


@socketio.on('number')
def handle_number(data):
    number = data.get('number')
    print(f"Received number: {number}")
    # Handle the number as needed
    emit('message', {'text': f'Received number: {number}', 'color': 'blue'})

@socketio.on('higherHysteresisLimit')
def handle_higher_hysteresis_limit(data):
    higher_hysteresis_limit = data.get('higherHysteresisLimit')
    print(f"Received higher hysteresis limit: {higher_hysteresis_limit}")
    # Handle the higher hysteresis limit as needed
    emit('message', {'text': f'Received higher hysteresis limit: {higher_hysteresis_limit}', 'color': 'green'})

@socketio.on('lowerHysteresisLimit')
def handle_lower_hysteresis_limit(data):
    lower_hysteresis_limit = data.get('lowerHysteresisLimit')
    print(f"Received lower hysteresis limit: {lower_hysteresis_limit}")
    # Handle the lower hysteresis limit as needed
    emit('message', {'text': f'Received lower hysteresis limit: {lower_hysteresis_limit}', 'color': 'orange'})
    
@socketio.on('flag1')
def handle_flag(data):
    flag1 = data.get('flag1', False)
    # Handle the flag logic here
    print(f'Flag1: {flag1}')
    
@socketio.on('flag2')
def handle_flag(data):
    flag2 = data.get('flag2', False)
    # Handle the flag logic here
    print(f'Flag2: {flag2}')

@socketio.on('option')
def handle_option(data):
    option = data.get('option', '')
    # Handle the option logic here
    print(f'Option selected: {option}')
   
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
        
#---- HANDLING PERSISTENT PARAMETERS ----#    
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
        
        
        
        
#--------- GLOBAL VARIABLES SECTION ---------# 


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




#--------- END GLOBAL VARIABLES SECTION ---------# 



if __name__ == '__main__':
    threading.Thread(target=monitoringArduino_task).start()
    threading.Thread(target=periodic_task).start()
    threading.Thread(target=send_async_messages_task_fnct).start()
    threading.Thread(target=send_async_messages_task_fnct_FILO).start()
    socketio.run(app, host='0.0.0.0', port=5000)
