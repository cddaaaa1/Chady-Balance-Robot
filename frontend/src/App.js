import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { Line } from 'react-chartjs-2';
import Modal from 'react-modal';
import logo from './Logo.png';


const SERVER_IP = 'http://172.20.10.7:5001';

Modal.setAppElement('#root');

function App() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [message, setMessage] = useState('');
  const [modalIsOpen, setModalIsOpen] = useState(false);
  const [notification, setNotification] = useState('');
  const [selectedVariable, setSelectedVariable] = useState('');

  const [pitchData, setPitchData] = useState({
    labels: [],
    datasets: [
      {
        label: 'Pitch',
        data: [],
        borderColor: 'rgb(75, 192, 192)',
        backgroundColor: 'rgba(75, 192, 192, 0.5)',
      }
    ]
  });

  const [velocityData, setVelocityData] = useState({
    labels: [],
    datasets: [
      {
        label: 'Velocity',
        data: [],
        borderColor: 'rgb(255, 99, 132)',
        backgroundColor: 'rgba(255, 99, 132, 0.5)',
      }
    ]
  });

  const [batteryData, setBatteryData] = useState({
    chargeSoc: 0,
    PM: 0,
    PL: 0,
  });

  const [variables, setVariables] = useState({
    vertical_kp: 0,
    vertical_kd: 0,
    velocity_kp: 0,
    velocity_ki: 0,
    turn_kp: 0,
    turn_kd: 0,
    turn_speed: 0,
    turn_direction: 0,
    target_velocity: 0,
    target_angle: 0,
    bias: 0,
  });

  const handleLoginRegister = () => {
    const payload = { username, password };
    axios.post(`${SERVER_IP}/login`, payload, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
    .then(response => {
      setLoggedIn(true);
      setMessage(`Welcome, ${response.data.username}`);
      showNotification(`Welcome, ${response.data.username}`);
      closeModal();
    })
    .catch(error => {
      if (error.response && error.response.status === 400 && error.response.data.error === 'Invalid username or password') {
        axios.post(`${SERVER_IP}/register`, payload, {
          headers: {
            'Content-Type': 'application/json'
          }
        })
        .then(response => {
          setMessage(`Registration successful: Welcome, ${username}`);
          showNotification(`Registration successful: Welcome, ${username}`);
          setLoggedIn(true);
          closeModal();
        })
        .catch(regError => {
          setMessage(`Registration failed: ${regError.response.data.error}`);
        });
      } else {
        setMessage(`Login failed: ${error.response.data.error}`);
      }
    });
  };

  const logout = () => {
    axios.post(`${SERVER_IP}/logout`, {}, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
    .then(response => {
      setLoggedIn(false);
      setUsername('');
      setPassword('');
      setMessage('Logged out');
      showNotification('Logged out');
    })
    .catch(error => {
      setMessage('Logout failed');
      console.error('Logout error:', error);
    });
  };

  const showNotification = (msg) => {
    setNotification(msg);
    setTimeout(() => {
      setNotification('');
    }, 1000);
  };

  const openModal = () => {
    setModalIsOpen(true);
    setMessage('');
  };

  const closeModal = () => {
    setModalIsOpen(false);
  };

  useEffect(() => {
    const interval = setInterval(() => {
      axios.get(`${SERVER_IP}/data`)
        .then(response => {
          const data = response.data;
          const newTime = new Date().toLocaleTimeString();
  
          setPitchData(prev => {
            const newLabels = [...prev.labels, newTime].slice(-20);
            const newData = [...prev.datasets[0].data, data.pitch].slice(-20);
            return {
              ...prev,
              labels: newLabels,
              datasets: [{ ...prev.datasets[0], data: newData }]
            };
          });
  
          setVelocityData(prev => {
            const newLabels = [...prev.labels, newTime].slice(-20);
            const newData = [...prev.datasets[0].data, data.velocity].slice(-20);
            return {
              ...prev,
              labels: newLabels,
              datasets: [{ ...prev.datasets[0], data: newData }]
            };
          });
        })
        .catch(error => console.error('Error fetching data:', error));
    }, 1000);
  
    return () => clearInterval(interval);
  }, []);
  

  useEffect(() => {
    const fetchBatteryData = () => {
      axios.get(`${SERVER_IP}/battery`)
      .then(response => {
        console.log("Battery data received:", response.data); 
        const { chargeSoc, PM, PL } = response.data;
        setBatteryData({ chargeSoc, PM, PL });
      })
      .catch(error => console.error('Error fetching battery data:', error));    
    };

    const batteryDataInterval = setInterval(fetchBatteryData, 1000);
    return () => clearInterval(batteryDataInterval);
  }, []);

  useEffect(() => {
    const handleKeyDown = (event) => {
      switch (event.key) {
        case 'ArrowUp':
          sendCommand('forward');
          break;
        case 'ArrowLeft':
          sendCommand('left');
          break;
        case 'ArrowDown':
          sendCommand('backward');
          break;
        case 'ArrowRight':
          sendCommand('right');
          break;
        case '1':
          sendCommand('switch_to_auto');
          break;
        case '2':
          sendCommand('switch_to_manual');
          break;
        case ' ':
          sendCommand('stop');
          break;
        default:
          break;
      }
    };

    window.addEventListener('keydown', handleKeyDown);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, []);

  const handleVariableChange = (name, value) => {
    setVariables(prev => ({
      ...prev,
      [name]: value
    }));
  };

  const fetchVariables = () => {
    axios.get(`${SERVER_IP}/get_variables`)
      .then(response => {
        setVariables(response.data);
      })
      .catch(error => {
        console.error('Error fetching variables:', error);
      });
  };

  const setVariable = (name, value) => {
    const payload = { variable: name, value: parseFloat(value) };
    axios.post(`${SERVER_IP}/set_variable`, payload, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
    .then(response => {
      console.log('Variable set successfully:', response.data);
    })
    .catch(error => {
      console.error('Error setting variable:', error);
    });
  };

  const setAllVariables = () => {
    Object.keys(variables).forEach(key => {
      setVariable(key, variables[key]);
    });
  };

  const clearAllVariables = () => {
    const clearedVariables = {};
    Object.keys(variables).forEach(key => {
      clearedVariables[key] = 0;
      setVariable(key, 0);
    });
    setVariables(clearedVariables);
  };

  const sendCommand = (command) => {
    axios.post(`${SERVER_IP}/move`, { command }, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
      .then(response => {
        console.log(`Command sent successfully: ${command}`, response.data);
      })
      .catch(error => {
        console.error('Error sending command:', error);
      });
  };  
  
  const sendColor = (color) => {
    axios.post(`${SERVER_IP}/color`, { color }, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
      .then(response => {
        console.log(`Color sent successfully: ${color}`, response.data);
      })
      .catch(error => {
        console.error('Error sending command:', error);
      });
  };

  const resetBattery = (action) => {
    axios.post(`${SERVER_IP}/reset_battery`, { action }, {
      headers: {
        'Content-Type': 'application/json'
      }
    })
    .then(response => {
      console.log(`Battery reset successfully: ${action}`, response.data);
    })
    .catch(error => {
      console.error('Error resetting battery:', error);
    });
  };

  const cardStyle = {
    backgroundColor: '#fff',
    borderRadius: '8px',
    boxShadow: '0 4px 8px rgba(0, 0, 0, 0.1)',
    padding: '10px',
    marginBottom: '10px',
    fontSize: '12px',
  };

  const chartContainerStyle = {
    height: 'calc(100% - 40px)', 
    width: '100%',
    position: 'relative',
    paddingTop: '5px', 
  };

  const buttonStyle = {
    marginRight: '5px',
    marginBottom: '5px',
    padding: '10px 15px',
    backgroundColor: '#fff',
    color: '#000',
    border: '2px solid #000',
    borderRadius: '5px',
    cursor: 'pointer',
    fontSize: '14px',
  };

  const buttonHoverStyle = {
    backgroundColor: '#f0f0f0',
  };

  const buttonActiveStyle = {
    backgroundColor: '#e0e0e0',
  };

  const doorColorButtonStyle = {
    marginRight: '10px',
    marginBottom: '10px',
    padding: '10px 20px',
    color: 'white',
    border: 'none',
    borderRadius: '5px',
    cursor: 'pointer',
    fontSize: '16px',
  };

  const inputStyle = {
    marginBottom: '10px',
    padding: '10px',
    width: '100%',
    borderRadius: '5px',
    border: '1px solid #ccc',
  };

  const labelStyle = {
    marginRight: '10px',
    fontWeight: 'bold',
  };

  const commandContainerStyle = {
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'flex-start',
  };

  const commandRowStyle = {
    display: 'flex',
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: '10px',
  };

  const progressContainerStyle = {
    width: '200px', 
    backgroundColor: '#ddd',
    borderRadius: '8px',
    overflow: 'hidden', 
    marginLeft: '10px', 
  };

  const progressBarStyle = {
    height: '20px', 
    backgroundColor: '#4CAF50', 
    textAlign: 'center', 
    lineHeight: '20px',
    color: 'white', 
    width: `${batteryData.chargeSoc}%`, 
    transition: 'width 0.5s ease-in-out', 
  };

  const infoContainerStyle = {
    display: 'flex',
    alignItems: 'center',
    gap: '20px', 
  };

  return (
    <div style={{ padding: '20px', fontFamily: 'Arial, sans-serif', backgroundColor: '#f4f4f4', position: 'relative' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
        <div style={{ display: 'flex', alignItems: 'center' }}>
          <h1 style={{ margin: '0 10px 0 0' }}>chady</h1>
          <img src={logo} alt="Logo" style={{ width: '50px', height: '50px' }} />
        </div>
        <div style={infoContainerStyle}>
          <div>
            <div>Charge SOC: {batteryData.chargeSoc.toFixed(2)}%</div>
            <div style={progressContainerStyle}>
              <div style={progressBarStyle}>{batteryData.chargeSoc.toFixed(0)}%</div>
            </div>
          </div>
          <div>Motor Power: {batteryData.PM.toFixed(2)} W</div>
          <div>Logic Power: {batteryData.PL.toFixed(2)} W</div>
        </div>
        <div>
          <button onClick={openModal} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Login/Register</button>
          <button onClick={logout} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Logout</button>
          {notification && <div style={{ backgroundColor: '#e0f7fa', padding: '10px', borderRadius: '5px', marginTop: '10px', textAlign: 'center' }}>{notification}</div>}
        </div>
      </div>

      <Modal
        isOpen={modalIsOpen}
        onRequestClose={closeModal}
        contentLabel="Login/Register"
        style={{
          overlay: {
            backgroundColor: 'rgba(0, 0, 0, 0.5)',
          },
          content: {
            maxWidth: '400px',
            maxHeight: '350px',
            margin: 'auto',
            borderRadius: '8px',
            padding: '20px',
            boxShadow: '0 4px 8px rgba(0, 0, 0, 0.1)',
            textAlign: 'center',
          }
        }}
      >
        <h2>Login/Register</h2>
        <input
          type="text"
          placeholder="Username"
          value={username}
          onChange={(e) => setUsername(e.target.value)}
          style={inputStyle}
        />
        <input
          type="password"
          placeholder="Password"
          value={password}
          onChange={(e) => setPassword(e.target.value)}
          style={inputStyle}
        />
        <button onClick={handleLoginRegister} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Submit</button>
        <button onClick={closeModal} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Close</button>
        {message && <p style={{ textAlign: 'center', marginTop: '10px' }}>{message}</p>}
      </Modal>

      <div style={{ marginLeft: '20px' }}>
          <button onClick={() => resetBattery('reset_to_full')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Reset Battery to Full</button>
          <button onClick={() => resetBattery('reset_based_on_voltage')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Reset Based on Voltage</button>
        </div>
      <div style={{ display: 'flex', flexWrap: 'wrap', justifyContent: 'space-between' }}>
        <div style={{ flex: '1 1 30%', marginBottom: '20px', marginRight: '30px' }}>
          <div style={{ ...cardStyle, height: '250px' }}>
            <h2 style={{ margin: '0 0 10px 0' }}>Pitch Data</h2>
            <div style={chartContainerStyle}>
              <Line data={pitchData} options={{ responsive: true, maintainAspectRatio: false }} />
            </div>
          </div>
          <div style={{ ...cardStyle, height: '250px', marginTop: '20px' }}>
            <h2 style={{ margin: '0 0 10px 0' }}>Velocity Data</h2>
            <div style={chartContainerStyle}>
              <Line data={velocityData} options={{ responsive: true, maintainAspectRatio: false }} />
            </div>
          </div>
        </div>
        <div style={{ flex: '1 1 40%', marginBottom: '20px' }}>
          <div style={cardStyle}>
            <h2>Control Variables</h2>
            <select
              value={selectedVariable}
              onChange={(e) => setSelectedVariable(e.target.value)}
              style={{ marginBottom: '10px', padding: '10px', width: '100%', borderRadius: '5px', border: '1px solid #ccc' }}
            >
              <option value="" disabled>Select a variable</option>
              {Object.keys(variables).map(key => (
                <option key={key} value={key}>{key}</option>
              ))}
            </select>
            {selectedVariable && (
              <div style={{ marginBottom: '10px' }}>
                <label style={labelStyle}>{selectedVariable}: </label>
                <input
                  type="number"
                  value={variables[selectedVariable]}
                  onChange={(e) => handleVariableChange(selectedVariable, e.target.value)}
                  style={inputStyle}
                />
                <button onClick={() => setVariable(selectedVariable, variables[selectedVariable])} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>
                  Set {selectedVariable}
                </button>
              </div>
            )}
            <div style={{ display: 'flex', flexDirection: 'row', justifyContent: 'space-between' }}>
              <button onClick={setAllVariables} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Set All</button>
              <button onClick={clearAllVariables} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Clear All</button>
              <button onClick={fetchVariables} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Fetch Variables</button>
            </div>
          </div>
          <div style={cardStyle}>
            <h2>Commands</h2>
            <div style={commandContainerStyle}>
              <div style={commandRowStyle}>
                <button onClick={() => sendCommand('switch_to_auto')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Automatic Mode</button>
                <button onClick={() => sendCommand('switch_to_manual')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Manual Mode</button>
              </div>
              <div style={commandRowStyle}>
                <button onClick={() => sendCommand('stop')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Stop</button>
                <button onClick={() => sendCommand('forward')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Forward</button>
                <button onClick={() => sendCommand('backward')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Backward</button>
                <button onClick={() => sendCommand('left')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Turn Left</button>
                <button onClick={() => sendCommand('right')} style={buttonStyle} onMouseEnter={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor} onMouseLeave={e => e.target.style.backgroundColor = buttonStyle.backgroundColor} onMouseDown={e => e.target.style.backgroundColor = buttonActiveStyle.backgroundColor} onMouseUp={e => e.target.style.backgroundColor = buttonHoverStyle.backgroundColor}>Turn Right</button>
              </div>
            </div>
            <h2>Door Color</h2>
            <div style={commandContainerStyle}>
              <div style={commandRowStyle}>
                <button onClick={() => sendColor('red')} style={{ ...doorColorButtonStyle, backgroundColor: '#FF6347' }} onMouseEnter={e => e.target.style.backgroundColor = '#FF4500'} onMouseLeave={e => e.target.style.backgroundColor = '#FF6347'} onMouseDown={e => e.target.style.backgroundColor = '#FF6347'} onMouseUp={e => e.target.style.backgroundColor = '#FF4500'}>Red</button>
                <button onClick={() => sendColor('yellow')} style={{ ...doorColorButtonStyle, backgroundColor: '#FFD700' }} onMouseEnter={e => e.target.style.backgroundColor = '#FFC700'} onMouseLeave={e => e.target.style.backgroundColor = '#FFD700'} onMouseDown={e => e.target.style.backgroundColor = '#FFD700'} onMouseUp={e => e.target.style.backgroundColor = '#FFC700'}>Yellow</button>
                <button onClick={() => sendColor('blue')} style={{ ...doorColorButtonStyle, backgroundColor: '#1E90FF' }} onMouseEnter={e => e.target.style.backgroundColor = '#1C86EE'} onMouseLeave={e => e.target.style.backgroundColor = '#1E90FF'} onMouseDown={e => e.target.style.backgroundColor = '#1E90FF'} onMouseUp={e => e.target.style.backgroundColor = '#1C86EE'}>Blue</button>
                <button onClick={() => sendColor('green')} style={{ ...doorColorButtonStyle, backgroundColor: '#32CD32' }} onMouseEnter={e => e.target.style.backgroundColor = '#2E8B57'} onMouseLeave={e => e.target.style.backgroundColor = '#32CD32'} onMouseDown={e => e.target.style.backgroundColor = '#32CD32'} onMouseUp={e => e.target.style.backgroundColor = '#2E8B57'}>Green</button>
                <button onClick={() => sendColor('purple')} style={{ ...doorColorButtonStyle, backgroundColor: '#9370DB' }} onMouseEnter={e => e.target.style.backgroundColor = '#8A2BE2'} onMouseLeave={e => e.target.style.backgroundColor = '#9370DB'} onMouseDown={e => e.target.style.backgroundColor = '#9370DB'} onMouseUp={e => e.target.style.backgroundColor = '#8A2BE2'}>Purple</button>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;



