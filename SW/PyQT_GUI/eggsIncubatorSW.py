import sys
import random
from PyQt5 import QtCore, QtWidgets
from incubatorGUI import Ui_MainWindow  # Import the UI class directly

import serial
import re

# ARDUINO serial communication - setup #
#port = "/dev/ttyUSB0"
portSetup = "/dev/ttyACM0"
baudrateSetup = 19200
timeout = 0.1

identifiers = ["TMP", "HUM", "HTP"]  # Global variable

class mainSoftwareThread(QtCore.QThread):
    # Signal to send updated temperature values to the GUI
    update_temperatures = QtCore.pyqtSignal(list)

    def __init__(self):
        super().__init__()
        self.running = True
        self.current_data = []  # Holds the most recent data received from SerialThread

    def run(self):
        while self.running:
            if self.current_data:
                # Extract data into specific categories
                currentTemperatures = {}
                currentHumidities = {}
                currentHumiditiesTemperature = {}

                for item in self.current_data:
                    for key, value in item.items():
                        if key.startswith("TMP"):
                            currentTemperatures[key] = value
                        elif key.startswith("HUM"):
                            currentHumidities[key] = value
                        elif key.startswith("HTP"):
                            currentHumiditiesTemperature[key] = value

                # Example: Emit temperatures to update GUI
                self.update_temperatures.emit(list(currentTemperatures.values()))
                print(list(currentTemperatures.values()))

                # Clear the data after processing
                self.current_data = []

            self.msleep(150)  # Adjust the delay as needed

    def stop(self):
        self.running = False

    def receive_data(self, new_data):
        self.current_data = new_data

class SerialThread(QtCore.QThread):
    # Signal to send decoded data to the TemperatureThread
    data_received = QtCore.pyqtSignal(list)

    def __init__(self, port = portSetup, baudrate = baudrateSetup):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.running = True

    def run(self):
        try:
            with serial.Serial(self.port, self.baudrate, timeout=1) as ser:
                print("Serial port opened successfully")
                buffer = ''
                startTransmission = False
                saving = False
                identifiers_data_list = []

                while self.running:
                    if ser.in_waiting > 0:
                        dataRead = ser.read().decode('utf-8')
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
                                    #print(identifiers_data_list)
                                    identifiers_data_list.clear()
        except serial.SerialException as e:
            print(f"Failed to open serial port: {e}")

    def stop(self):
        self.running = False

    @staticmethod
    def decode_message(buffer, identifiers_data_list):
        match = re.match(r"<([^,]+),([^>]+)>", buffer)
        if match:
            infoName_part = match.group(1)
            infoData_part = match.group(2)

            if SerialThread.is_number(infoData_part):
                # Convert to float or int
                if '.' in infoData_part:
                    number = float(infoData_part)
                else:
                    number = int(infoData_part)

                # Add to identifiers_data_list if matches identifiers
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
            float(s)  # Try to convert to float
            return True
        except ValueError:
            return False

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)

        # Set up the main software thread
        self.mainSoftware_thread = mainSoftwareThread()
        self.mainSoftware_thread.update_temperatures.connect(self.update_temperature_display)
        self.mainSoftware_thread.start()

        # Set up the serial thread
        self.serial_thread = SerialThread(port = portSetup, baudrate = baudrateSetup)
        self.serial_thread.data_received.connect(self.mainSoftware_thread.receive_data)
        self.serial_thread.start()

        # Connect buttons to example handlers
        self.ui.move_CW_btn.clicked.connect(self.handle_move_cw)
        self.ui.move_CCW_btn.clicked.connect(self.handle_move_ccw)
        self.ui.reset_btn.clicked.connect(self.handle_reset)

        # Connect radio buttons and spinBox to handlers
        self.ui.maxValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.meanValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.minValueTempControl_radioBtn.toggled.connect(self.handle_radio_button)
        self.ui.speedRPM_spinBox.valueChanged.connect(self.handle_spinbox)

    def update_temperature_display(self, temperatures):
        # Update the temperature labels in the GUI
        self.ui.temperature1_4.setText(f"{temperatures[0]} °C")
        self.ui.temperature2_4.setText(f"{temperatures[1]} °C")
        self.ui.temperature3_5.setText(f"{temperatures[2]} °C")
        self.ui.temperature4_2.setText(f"{temperatures[3]} °C")

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
        self.mainSoftware_thread.stop()
        self.mainSoftware_thread.wait()
        self.serial_thread.stop()
        self.serial_thread.wait()
        super().closeEvent(event)


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
