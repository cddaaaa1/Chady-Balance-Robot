from flask import Flask, request
import requests
from flask_cors import CORS  

app = Flask(__name__)
CORS(app) 

@app.route('/move', methods=['POST'])
def move():
    command = request.json['command']
    response = requests.post('http://172.20.10.4:8000/send_command', json={'command': command}) #IP address of raspberry pi
    return response.text

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5001)

