from flask import Flask, request, jsonify, session
from flask_cors import CORS
from flask_session import Session
from flask_sqlalchemy import SQLAlchemy
import os
import requests

app = Flask(__name__)
CORS(app, supports_credentials=True)

RASPBERRY_PI_IP = '172.20.10.2'

# Configurations for session management
app.config['SECRET_KEY'] = os.urandom(24)
app.config['SESSION_TYPE'] = 'filesystem'
app.config['SESSION_FILE_DIR'] = './.flask_session/'
app.config['SESSION_PERMANENT'] = False
app.config['SESSION_USE_SIGNER'] = True
app.config['SESSION_COOKIE_SECURE'] = False   #http used
Session(app)

# Database configurations
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///users.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)
Session(app)

class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(150), unique=True, nullable=False)
    password = db.Column(db.String(150), nullable=False)

class PowerUsage(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(150), nullable=False)
    charge_soc = db.Column(db.Float, nullable=False)
    timestamp = db.Column(db.DateTime, server_default=db.func.now())

with app.app_context():
    db.create_all()

@app.route('/login', methods=['POST'])
def login():
    data = request.get_json()
    if not data or 'username' not in data or 'password' not in data:
        return jsonify({'error': 'Missing username or password'}), 400

    username = data['username']
    password = data['password']
    user = User.query.filter_by(username=username, password=password).first()
    if user:
        session['username'] = username
        session.permanent = True  # Ensure the session is permanent
        print(f"User {username} logged in. Session: {session}")
        return jsonify({'status': 'Logged in', 'username': username}), 200
    return jsonify({'error': 'Invalid username or password'}), 400


@app.route('/logout', methods=['POST'])
def logout():
    session.clear()
    return jsonify({'status': 'Logged out'}), 200


@app.route('/register', methods=['POST'])
def register():
    data = request.get_json()
    if not data or 'username' not in data or 'password' not in data:
        return jsonify({'error': 'Missing username or password'}), 400

    username = data['username']
    password = data['password']
    if User.query.filter_by(username=username).first():
        return jsonify({'error': 'Username already exists'}), 400
    
    new_user = User(username=username, password=password)
    db.session.add(new_user)
    db.session.commit()
    session['username'] = username  # Log in the user immediately after registration
    return jsonify({'status': 'User registered successfully', 'username': username}), 200

@app.route('/users', methods=['GET'])
def list_users():
    users = User.query.all()
    return jsonify([{'username': user.username, 'password': user.password} for user in users]), 200

@app.route('/move', methods=['POST'])
def move():
    command = request.json.get('command')
    try:
        response = requests.post(f'http://{RASPBERRY_PI_IP}/controller', data={'cmd': command}, timeout=5)
        return response.text
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/set_variable', methods=['POST'])
def set_variable():
    data = request.json
    if 'variable' in data and 'value' in data:
        variable = data['variable']
        value = data['value']
        try:
            response = requests.post(f'http://{RASPBERRY_PI_IP}:8000/set_variable', json={'variable': variable, 'value': value}, timeout=5)
            return response.text
        except requests.RequestException as e:
            return jsonify({'error': str(e)}), 500
    return jsonify({'error': 'Variable or value not provided'}), 400

@app.route('/data', methods=['GET'])
def get_data():
    try:
        response = requests.get(f'http://{RASPBERRY_PI_IP}:8000/data', timeout=5)
        return response.json() if response.status_code == 200 else ('Failed to retrieve data', 500)
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/get_variables', methods=['GET'])
def get_variables():
    try:
        response = requests.get(f'http://{RASPBERRY_PI_IP}:8000/get_variables', timeout=5)
        return response.json() if response.status_code == 200 else ('Failed to retrieve variables', 500)
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/color', methods=['POST'])
def color():
    color = request.json.get('color')
    if color:
        session['colors'] = session.get('colors', []) + [color]
        try:
            response = requests.post(f'http://{RASPBERRY_PI_IP}:8000/color', json={'color': color}, timeout=5)
            return response.text
        except requests.RequestException as e:
            return jsonify({'error': str(e)}), 500
    return jsonify({'error': 'No color provided'}), 400

@app.route('/battery', methods=['GET'])
def get_battery_data():
    try:
        response = requests.get(f'http://{RASPBERRY_PI_IP}:8000/battery', timeout=5)
        if response.status_code == 200:
            return jsonify(response.json())
        else:
            return 'Failed to retrieve battery data', 500
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500


@app.route('/reset_battery', methods=['POST'])
def reset_battery():
    action = request.json.get('action')
    try:
        response = requests.post(f'http://{RASPBERRY_PI_IP}:8000/reset_battery', json={'action': action}, timeout=5)
        return response.json() if response.status_code == 200 else ('Failed to reset battery', 500)
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5001)