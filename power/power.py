import spidev
import time
import RPi.GPIO as GPIO

# MCP3208 pin connections
cs_pin = 8  # GPIO8 (Pin 24 on the Raspberry Pi) - Chip Select (SS)
miso_pin = 9  # GPIO9 (Pin 21 on the Raspberry Pi) - Master In Slave Out (MISO)
mosi_pin = 10  # GPIO10 (Pin 19 on the Raspberry Pi) - Master Out Slave In (MOSI)
clk_pin = 11  # GPIO11 (Pin 23 on the Raspberry Pi) - Serial Clock (SCK)

# Define MCP3208 channels for each terminal
VOM_CHANNEL = 0
VOL_CHANNEL = 1
VB_CHANNEL = 2
VM_CHANNEL = 3
VL_CHANNEL = 4

# Constants
MAX_ADC = 3.3
MAX_12bits = 4095
R = 0.01  # 10mΩ resistor
VtoI_motor = 1  # voltage to current ratio
VtoI_logic = 1  # voltage to current ratio
PD_motor = 5.0  # Potential divider of Vm
PD_logic = 5.0  # Potential divider of Vl
PD_Vb = 5.0  # Potential divider of Vb

numPoints = 11
voltageTable = [16.8, 16.4, 16.0, 15.6, 15.2, 14.8, 14.4, 14.0, 13.6, 13.2, 12.8]
socTable = [100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0]

soc = 0  # State of Charge (%)
lastUpdateTime = 0  # To track time intervals
accumulatedCharge = 0.0  # mAh
BATTERY_CAPACITY = 2000  # Battery capacity in mAh

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

# Measure initial value 
initialVb = read_voltage(VB_CHANNEL) * PD_Vb
soc = voltage_SOC(initialVb)
lastUpdateTime = time.time()

try:
    while True:
        # Read values from MCP3208
        VOM = read_voltage(VOM_CHANNEL)
        VOL = read_voltage(VOL_CHANNEL)
        Vb = read_voltage(VB_CHANNEL) * PD_Vb
        VM = read_voltage(VM_CHANNEL) * PD_motor
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

        I_bat = IM + (IL * (5.0 / Vb)) * 0.93
        Ibat_mA = I_bat * 1000.0  # Convert to mA
        accumulatedCharge += (Ibat_mA * (elapsedTime / 3600.0))  # Charge in mAh
        chargeSoc = 100.0 - ((accumulatedCharge / BATTERY_CAPACITY) * 100.0)
        voltageSoc = voltage_SOC(Vb)

        print(f"Voltage SOC: {voltageSoc}")
        print(f"Charge SOC: {chargeSoc}")

        time.sleep(1)  # Delay for 1 second before next measurement

except KeyboardInterrupt:
    pass

finally:
    spi.close()
    GPIO.cleanup()
