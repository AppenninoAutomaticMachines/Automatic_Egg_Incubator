import serial
import time

# Configure the serial port
port = "/dev/ttyUSB0"
baudrate = 9600
timeout = 0.1

def read_from_arduino(serial_port):
    buffer = ''
    while True:
        try:
            if serial_port.in_waiting > 0:
                data = serial_port.read()
                data = data.decode('utf-8')
                if data == '#':
                    print(buffer)
                    buffer = ''
                else:
                    buffer += data
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
            ser = serial.Serial(port, baudrate, timeout=timeout)
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
