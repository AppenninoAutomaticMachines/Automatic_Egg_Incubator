import serial
import time
import re

# Configure the serial port
port = '/dev/ttyUSB0'
baudrate = 19200
timeout = 0.1

def is_number(s):
    try:
        float(s)  # Try to convert to float
        return True
    except ValueError:
        return False
        
def decodeMessage(buffer):
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

            # Example actions based on string_part
            if infoName_part == "TMP01":         
                print("Info Name part:", infoName_part)
                print("Info Data part:", infoData_part)   
                    
            if infoName_part == "TMP02":         
                print("Info Name part:", infoName_part)
                print("Info Data part:", infoData_part) 
                 
            if infoName_part == "TMP03":         
                print("Info Name part:", infoName_part)
                print("Info Data part:", infoData_part)  
            # Add more conditions based on `string_part` as needed
        else:
            print("Info Name part:", infoName_part)
            print("Info Data part:", infoData_part)  
    else:
        print("The input string does not match the expected format.")

            
def read_from_arduino(serial_port):
    buffer = ''
    bufferForUsage = ''
    saving = False
    while True:
        try:
            if serial_port.in_waiting > 0:
                data = serial_port.read().decode('utf-8')
                if data == '<':
                	saving = True
                if saving:
                	buffer += data
                if data == '>':
                    saving = False
                    decodeMessage(buffer)                    
                    buffer = ''
                if data == '#':
                	print()
                	
        except UnicodeDecodeError as e:
            print(f"Decode error: {e}")
            # Clear the input buffer to avoid repeated errors
            serial_port.reset_input_buffer()
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            break

try:
    while True:
        try:
            # Open the serial port
            ser = serial.Serial(port, baudrate)
            print("Serial port opened successfully")
            read_from_arduino(ser)
        except serial.SerialException as e:
            print(f"Failed to open serial port: {e}")
        finally:
            try:
                ser.close()
            except Exception as e:
                print(f"Error closing serial port: {e}")
        
        print("Retrying in 5 seconds...")
        time.sleep(5)
except KeyboardInterrupt:
    print("Program interrupted")
