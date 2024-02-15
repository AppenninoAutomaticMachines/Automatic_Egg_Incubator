import warnings
import serial
import serial.tools.list_ports
import time

# https://pythonpyqt.com/how-to-install-pyqt5-in-pycharm/

ports = serial.tools.list_ports.comports()
serialInst = serial.Serial()

portsList = []

for onePort in ports:
    portsList.append(str(onePort))
    print(str(onePort))

val = input("Select Port: COM")

for x in range(0, len(portsList)):
    if portsList[x].startswith("COM" + str(val)):
        portVar = "COM" + str(val)
        print(portVar)

serialInst.baudrate = 9600
serialInst.port = portVar
serialInst.open()

final=[]
commandsToSend = []

commandsToSend.append("ab")
commandsToSend.append("cd")
commandsToSend.append("ef")
commandsToSend.append("zz")
# ['aa', 'va', 'ft']

def sendingCommandsToIno():
    dummy = True
    startCommandTransmission = False
    print("1")
    while serialInst.inWaiting() <= 0:  #waiting for Arduino to be ready to receive commands
        dummy = not dummy # do nothing.
    print("2")
    if serialInst.inWaiting() > 0:
        print("3")
        incomingReading = serialInst.read_until(b'#')  # python legge wa#
        print(incomingReading)
        print(type(incomingReading))
        print(len(incomingReading))

        decodedString = incomingReading.decode("utf-8")
        print(decodedString)
        print(type(decodedString))
        print(len(decodedString))
        print("4")
        if decodedString == 'wa#':  # can start the tranismission
            print("5")
            for i in range(0, len(commandsToSend)):
                print(str(i))
                strToSend = commandsToSend[i] + "#"
                serialInst.write(strToSend.encode("utf-8"))
                print(strToSend)
                # wait for received
                while serialInst.inWaiting() <= 0:  # waiting for Arduino to be ready to receive commands
                    dummy = not dummy
                print("6")
                if serialInst.inWaiting() > 0:
                    incomingReading = serialInst.read_until(b'#')
                    decodedString = incomingReading.decode("utf-8")
                    print(decodedString)
                    # in teoria non c'è bisogno del check...
                    if decodedString == 'rd#':
                        print("Writing next command")
                    else:
                        print("error")
while True:
    sendingCommandsToIno()
    print("999")
    while serialInst.inWaiting() <= 0:  # waiting for Arduino to be ready to receive commands
        dummy = True
    while serialInst:
        bytesToRead = serialInst.inWaiting()
        if serialInst.inWaiting() > 0:
            s = serialInst.read(1)
            print(s)

    exit()




