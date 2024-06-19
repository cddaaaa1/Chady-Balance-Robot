import spidev
import time
import RPi.GPIO as GPIO
import os
import threading

# MCP3208 pin connections
cs_pin = 8  # Pin 24 on the Raspberry Pi - Chip Select (SS)
miso_pin = 9  # Pin 21 on the Raspberry Pi - Master In Slave Out (DIN)
mosi_pin = 10  # Pin 19 on the Raspberry Pi - Master Out Slave In (DOUT)
clk_pin = 11  # Pin 23 on the Raspberry Pi - Serial Clock (CLK)

# Define MCP3208 channels for each terminal
VB_CHANNEL = 0
VOM_CHANNEL = 1 
VOL_CHANNEL = 2
VL_CHANNEL = 3


# Constants
MAX_ADC = 5.0
MAX_12bits = 4095
R = 0.01  # 10mΩ resistor
VtoI_motor = 1  # voltage to current ratio
VtoI_logic = 1  # voltage to current ratio
PD_logic = 2.0  # Potential divider of Vl
PD_Vb = 4.0  # Potential divider of Vb

numPoints = 29

capacity_discharged = [0, 2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 82.5, 85, 87.5, 90, 92.5, 95, 97.5, 100]
voltage = [16.8, 16.525, 16.25, 16.0725, 15.93, 15.8, 15.63, 15.5275, 15.4, 15.25, 15.13, 15.035, 14.94, 14.845, 14.775, 14.7275, 14.68, 14.6325, 14.585, 14.5375, 14.4, 14.3, 14.2425, 14.15, 13.98, 13.8, 13.6, 12.9975, 10]

soc = 0  # State of Charge (%)
lastUpdateTime = 0  # To track time intervals
accumulatedCharge = 0.0  # mAh
BATTERY_CAPACITY = 2000  # Battery capacity in mAh
CAPACITY_FILE = 'battery_capacity.txt'  # File to store acculated charge

# Initialize SPI
spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 100000

# Initialize GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(cs_pin, GPIO.OUT)
GPIO.output(cs_pin, GPIO.HIGH)

lock = threading.Lock()

def read_voltage(channel):
    if channel < 0 or channel > 7:
        return -1  # Invalid channel
    
    # Perform SPI transaction and store the returned bits
    GPIO.output(cs_pin, GPIO.LOW)
    adc = spi.xfer2([0x06 | ((channel & 0x04) >> 2), (channel & 0x03) << 6, 0x00])
    GPIO.output(cs_pin, GPIO.HIGH)
    
    # Combine the three bytes to get the result
    adc_out = ((adc[1] & 0x0F) << 8) | adc[2]
    voltage = (adc_out / MAX_12bits) * MAX_ADC
    return voltage

def voltage_SOC(v_battery):
    
    if v_battery >= voltage[0]:
        return capacity_discharged[0]
    if v_battery <= voltage[numPoints - 1]:
        return capacity_discharged[numPoints - 1]
    
    for i in range(numPoints - 1):
        if v_battery <= voltage[i] and v_battery > voltage[i + 1]:
            soc = capacity_discharged[i] + (capacity_discharged[i + 1] - capacity_discharged[i]) * (v_battery - voltage[i]) / (voltage[i + 1] - voltage[i])
            return soc
    return 0 

def read_stored_capacity():
    if os.path.exists(CAPACITY_FILE):
        with open(CAPACITY_FILE, 'r') as file:
            return float(file.read())
    return BATTERY_CAPACITY

def store_capacity(capacity):
    with open(CAPACITY_FILE, 'w') as file:
        file.write(str(capacity))

def reset_to_full_capacity():
    global accumulatedCharge, soc
    with lock:
        accumulatedCharge = 0
        soc = 100
        store_capacity(accumulatedCharge)
    print("Battery capacity reset to 100%")

def reset_based_on_voltage():
    global accumulatedCharge, soc
    Vb = read_voltage(VB_CHANNEL) * PD_Vb
    soc = 100 - voltage_SOC(Vb)
    with lock:
        accumulatedCharge = (1 - (soc / 100)) * BATTERY_CAPACITY
        store_capacity(accumulatedCharge)
    print(f"Battery capacity reset based on voltage to {soc}%")

def user_input_thread():
    while True:
        user_input = input("Press 'r' to reset battery capacity to 100%, 'v' to reset based on voltage, or press Enter to continue: ")
        if user_input.lower() == 'r':
            reset_to_full_capacity()
        elif user_input.lower() == 'v':
            reset_based_on_voltage()
#using tread for resetting 
threading.Thread(target=user_input_thread, daemon=True).start()

# Measure initial value
initialVb = 0
while initialVb <= 13.0:
    initialVb = read_voltage(VB_CHANNEL) * PD_Vb
    if initialVb <= 13.0:
        print("Voltage not detected or low battery, retrying....")
        time.sleep(1)

# Initialize SOC and accumulated charge
stored_capacity = read_stored_capacity()
accumulatedCharge = stored_capacity  # Use stored capacity default

lastUpdateTime = time.time()

try:
    while True:
        # Read values from MCP3208
        VOM = read_voltage(VOM_CHANNEL)
        VOL = read_voltage(VOL_CHANNEL)
        Vb = read_voltage(VB_CHANNEL) * PD_Vb
        VL = read_voltage(VL_CHANNEL) * PD_logic

        # Calculate currents and power
        IM = VOM * VtoI_motor  # Motor current
        VM = Vb - R * IM # Motor Voltage
        IL = VOL * VtoI_logic  # Logic current
        PM = IM * VM  # Motor power
        PL = IL * VL  # Logic power
        
        print(f"Motor Current: {IM} A, Motor Voltage: {VM} V, Motor Power: {PM} W")
        
        print(f"Logic Current: {IL} A, Logic Voltage: {VL} V, Logic Power: {PL} W")

        print(f"Battery Voltage: {Vb} V")

       
        currentTime = time.time()
        elapsedTime = currentTime - lastUpdateTime
        lastUpdateTime = currentTime

        I_bat = IM + (IL * (VL / Vb)) * 0.93 
        Ibat_mA = I_bat * 1000.0  # Convert to mA
        with lock:
            accumulatedCharge += (Ibat_mA * (elapsedTime / 3600.0))  # Charge in mAh

        chargeSoc = 100 - ((accumulatedCharge / BATTERY_CAPACITY) * 100.0)

        print(f"Charge SOC: {chargeSoc}")
    

        time.sleep(1)  
        store_capacity(accumulatedCharge)

except KeyboardInterrupt:
    pass

finally:
    store_capacity(accumulatedCharge)
    spi.close()
    GPIO.cleanup()