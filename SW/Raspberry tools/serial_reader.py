import serial
import time

# Adjust these settings for your setup
SERIAL_PORT = '/dev/ttyACM0'       # e.g., 'COM3' on Windows or '/dev/ttyUSB0' on Linux oppure portSetup = "/dev/ttyACM0"
BAUD_RATE = 9600           # Must match Arduino's Serial.begin() rate
BAUD_RATE = 18200

def read_from_arduino():
    try:
        # Open serial port
        arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # wait for the connection to initialize

        print("Connected to Arduino. Listening for data...\n")
        while True:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8', errors='ignore').strip()
                print(f"Received: {line}")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("Exiting program.")
    finally:
        if 'arduino' in locals() and arduino.is_open:
            arduino.close()
            print("Serial connection closed.")

if __name__ == '__main__':
    read_from_arduino()
