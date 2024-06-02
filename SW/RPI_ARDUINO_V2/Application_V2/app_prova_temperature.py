from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import random
import time
import threading
import queue
import serial
import re


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

def monitoringArduino_task():
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
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
                dataRead = serial_port.read()
                dataRead = dataRead.decode('utf-8')
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
            
            # Check for new data in the queue
            if not temperature_queue.empty():
                data = temperature_queue.get()
                
                # Process the temperature data
                data.update({'mainHeater': random.choice([True, False])})
                data.update({'auxHeater': random.choice([True, False])})
                data.update({'machineState': random.randint(0, 5)})
                # Send data to HTML page
                socketio.emit('data_update', data)
        except Exception as e:
            print(f"Error in periodic_task: {e}")

def send_async_messages():
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
        
def send_async_messages_FILO():
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

if __name__ == '__main__':
    threading.Thread(target=monitoringArduino_task).start()
    threading.Thread(target=periodic_task).start()
    threading.Thread(target=send_async_messages).start()
    threading.Thread(target=send_async_messages_FILO).start()
    socketio.run(app, host='0.0.0.0', port=5000)
