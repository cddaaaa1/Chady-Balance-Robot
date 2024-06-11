import spidev
import time
import RPi.GPIO as GPIO
import os

# MCP3208 pin connections
cs_pin = 8  # GPIO8 (Pin 24 on the Raspberry Pi) - Chip Select (SS)
miso_pin = 9  # GPIO9 (Pin 21 on the Raspberry Pi) - Master In Slave Out (MISO)
mosi_pin = 10  # GPIO10 (Pin 19 on the Raspberry Pi) - Master Out Slave In (MOSI)
clk_pin = 11  # GPIO11 (Pin 23 on the Raspberry Pi) - Serial Clock (SCK)

# Define MCP3208 channels for each terminal
VOM_CHANNEL = 0
VOL_CHANNEL = 1
VB_CHANNEL = 2
VL_CHANNEL = 4

# Constants
MAX_ADC = 5.0
MAX_12bits = 4095
R = 0.01  # 10mΩ resistor
VtoI_motor = 1  # voltage to current ratio
VtoI_logic = 1  # voltage to current ratio
PD_motor = 5.0  # Potential divider of Vm
PD_logic = 5.0  # Potential divider of Vl
PD_Vb = 5.0  # Potential divider of Vb

numPoints = 29
capacity_discharged = [0, 2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 82.5, 85, 87.5, 90, 92.5, 95, 97.5, 100];
voltage = [16.8, 16.325, 15.85, 15.6725, 15.53, 15.425, 15.33, 15.2675, 15.22, 15.05, 14.93, 14.835, 14.74, 14.645, 14.575, 14.5275, 14.48, 14.4325, 14.385, 14.3375, 14.29, 14.25, 14.23, 14.1, 14, 13.8, 13.5, 12.9975, 12];


soc = 0  # State of Charge (%)
lastUpdateTime = 0  # To track time intervals
accumulatedCharge = 0.0  # mAh
BATTERY_CAPACITY = 2000  # Battery capacity in mAh
CAPACITY_FILE = 'battery_capacity.txt'  # File to store battery capacity

# Initialize SPI
spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 1000000

# Initialize GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(cs_pin, GPIO.OUT)
GPIO.output(cs_pin, GPIO.HIGH)

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
    if v_battery >= voltageTable[0]:
        return socTable[0]
    if v_battery <= voltageTable[numPoints - 1]:
        return socTable[numPoints - 1]
    
    for i in range(numPoints - 1):
        if v_battery <= voltageTable[i] and v_battery > voltageTable[i + 1]:
            soc = socTable[i] + (socTable[i + 1] - socTable[i]) * (v_battery - voltageTable[i]) / (voltageTable[i + 1] - voltageTable[i])
            return soc
    return 0  # Should never reach here

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
    accumulatedCharge = 0
    soc = 100
    store_capacity(accumulatedCharge)
    print("Battery capacity reset to 100%")

def reset_based_on_voltage():
    global accumulatedCharge, soc
    Vb = read_voltage(VB_CHANNEL) * PD_Vb
    soc = voltage_SOC(Vb)
    accumulatedCharge = (1 - (soc / 100)) * BATTERY_CAPACITY
    store_capacity(accumulatedCharge)
    print(f"Battery capacity reset based on voltage to {soc}%")

# Measure initial value
initialVb = 0
while initialVb <= 12.8:
    initialVb = read_voltage(VB_CHANNEL) * PD_Vb
    if initialVb <= 12.8:
        print("Voltage not detected or low battery, retrying....")
        time.sleep(1)

# Initialize SOC and accumulated charge
stored_capacity = read_stored_capacity()
accumulatedCharge = (1 - (soc / 100)) * stored_capacity  # Initial accumulated charge

lastUpdateTime = time.time()

try:
    while True:
        # Simulate a UI reset action (for example, pressing a button)
        # Replace this with actual UI interaction code
        user_input = input("Press 'r' to reset battery capacity to 100%, 'v' to reset based on voltage, or any other key to continue: ")
        if user_input.lower() == 'r':
            reset_to_full_capacity()
            continue
        elif user_input.lower() == 'v':
            reset_based_on_voltage()
            continue

        # Read values from MCP3208
        VOM = read_voltage(VOM_CHANNEL)
        VOL = read_voltage(VOL_CHANNEL)
        Vb = read_voltage(VB_CHANNEL) * PD_Vb
        VM = Vb  # Reuse the value read for VB_CHANNEL
        VL = read_voltage(VL_CHANNEL) * PD_logic

        # Calculate currents and power
        IM = VOM * VtoI_motor  # Motor current
        IL = VOL * VtoI_logic  # Logic current
        PM = IM * VM  # Motor power
        PL = IL * VL  # Logic power

        # Print motor measurements
        print(f"Motor Current: {IM} A, Motor Voltage: {VM} V, Motor Power: {PM} W")
        
        # Print logic measurements
        print(f"Logic Current: {IL} A, Logic Voltage: {VL} V, Logic Power: {PL} W")

        # Battery measurements
        print(f"Battery Voltage: {Vb} V")

        # Calculate battery parameters
        currentTime = time.time()
        elapsedTime = currentTime - lastUpdateTime
        lastUpdateTime = currentTime

        I_bat = IM + (IL * (VL / Vb)) * 0.93 
        Ibat_mA = I_bat * 1000.0  # Convert to mA
        accumulatedCharge += (Ibat_mA * (elapsedTime / 3600.0))  # Charge in mAh

        chargeSoc = 100 - ((accumulatedCharge / BATTERY_CAPACITY) * 100.0)

        print(f"Charge SOC: {chargeSoc}")

        time.sleep(1)  # Delay for 1 second before next measurement

except KeyboardInterrupt:
    pass

finally:
    store_capacity(accumulatedCharge)
    spi.close()
    GPIO.cleanup()
