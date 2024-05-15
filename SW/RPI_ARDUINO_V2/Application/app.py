from flask import Flask, render_template, jsonify, request
from flask_socketio import SocketIO, emit
import time
import random
from threading import Thread

app = Flask(__name__)
socketio = SocketIO(app)

clients = []

# Placeholder data
data = {
    "temp1": 22.5,
    "temp2": 23.0,
    "temp3": 21.7,
    "mainHeater": False,
    "auxHeater": False,
    "machineState": 0
}

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/data', methods=['GET'])
def get_data():
    return jsonify(data)

@socketio.on('connect')
def handle_connect(auth):
    session_id = request.sid
    clients.append(session_id)
    print(f"Client connected: {session_id}")

@socketio.on('disconnect')
def handle_disconnect():
    session_id = request.sid
    if session_id in clients:
        clients.remove(session_id)
    print(f"Client disconnected: {session_id}")

@socketio.on('command')
def handle_command(json):
    cmd = json['cmd']
    session_id = request.sid
    if cmd == 'start':
        data['machineState'] = 1
        emit('message', "Machine started", room=session_id)
    elif cmd == 'stop':
        data['machineState'] = 0
        emit('message', "Machine stopped", room=session_id)
    elif cmd == 'restart':
        data['machineState'] = 2
        emit('message', "Machine restarted", room=session_id)
    else:
        emit('message', "Unknown command", room=session_id)
        return

    emit('data_update', data, room=session_id)

@socketio.on('number')
def handle_number(json):
    number = json['number']
    session_id = request.sid
    try:
        number = float(number)
        emit('message', f"Received number: {number}", room=session_id)
        # Example: you could update data based on the received number
        data['temp1'] = number
        emit('data_update', data, room=session_id)
    except ValueError:
        emit('message', "Invalid number", room=session_id)

# Simulate sensor data updates
def update_sensors():
    while True:
        data['temp1'] += random.uniform(-0.5, 0.5)
        data['temp2'] += random.uniform(-0.5, 0.5)
        data['temp3'] += random.uniform(-0.5, 0.5)
        data['mainHeater'] = not data['mainHeater']
        data['auxHeater'] = not data['auxHeater']
        data['machineState'] = random.randint(0, 2)
        for client in clients:
            socketio.emit('data_update', data, room=client)
        time.sleep(5)

if __name__ == '__main__':
    Thread(target=update_sensors).start()
    socketio.run(app, host='0.0.0.0', port=5000)
