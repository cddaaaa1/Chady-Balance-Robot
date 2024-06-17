import requests
from flask import Flask, request

app = Flask(__name__)

@app.route('/send_command', methods=['POST'])
def send_command():
    command = request.json['command']
    response = requests.get(f'http://172.20.10.3/?command={command}') #IP address of esp32
    return response.text

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)
