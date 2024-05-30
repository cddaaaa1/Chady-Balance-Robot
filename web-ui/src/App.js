import React from 'react';
import axios from 'axios';
import './App.css';  

function App() {
  const sendCommand = (command) => {
    axios.post('http://172.20.10.6:5001/move', { command }) // IP address of the server
      .then(response => {
        console.log(response.data);
      })
      .catch(error => {
        console.error("There was an error sending the command!", error);
      });
  };

  return (
    <div className="App">
      <h1>Robot Control</h1>
      <div className="button-container">
        <button className="control-button button1" onClick={() => sendCommand('switch_to_auto')}>Automatic Mode</button>
        <button className="control-button button2" onClick={() => sendCommand('switch_to_manual')}>Manual Mode</button>
        <button className="control-button button3" onClick={() => sendCommand('forward')}>Move Forward</button>
        <button className="control-button button4" onClick={() => sendCommand('backward')}>Move Backward</button>
        <button className="control-button button5" onClick={() => sendCommand('left')}>Move Left</button>
        <button className="control-button button6" onClick={() => sendCommand('right')}>Move Right</button>
        <button className="control-button button7" onClick={() => sendCommand('stop')}>Stop</button>
      </div>
    </div>
  );
}

export default App;

