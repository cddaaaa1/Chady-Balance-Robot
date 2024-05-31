import React from 'react';
import axios from 'axios';

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
     <div>
       <h1>Robot Control</h1>
       <button onClick={() => sendCommand('switch_to_auto')}>Automatic Mode</button>
       <button onClick={() => sendCommand('switch_to_manual')}>Manual Mode</button>
       <button onClick={() => sendCommand('forward')}>Move Forward</button>
       <button onClick={() => sendCommand('backward')}>Move Backward</button>
       <button onClick={() => sendCommand('left')}>Move Left</button>
       <button onClick={() => sendCommand('right')}>Move Right</button>
       <button onClick={() => sendCommand('stop')}>Stop</button>
     </div>
   );
 }

 export default App;

