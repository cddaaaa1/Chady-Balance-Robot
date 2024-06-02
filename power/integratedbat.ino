#define VDM_PIN 25 // Arduino A2 for motor differential
#define Vmotor_PIN 24 // Arduino A3 for motor voltage 
#define VDL_PIN  26 // Arduino A4 for logic differential 
#define Vlogic_PIN 27 // Arduino A5 for logic voltage 
#define Vbat_PIN 23 // 
#define MAX_ADC 3.3 
#define MAX_12bits 4095

const float R = 0.01; // 10mΩ resistor 
const float G_motor = 10.0; // Gain of motor
const float G_logic = 10.0; // Gain of logic 
const float PD_motor = 6.0; // Potential divider of Vm
const float PD_Vb = 6.0; // Potential divider#include <SPI.h>
#include <MCP_ADC.h>

// MCP3208 pins
#define CS_PIN 5

// Create an MCP3208 object
MCP_ADC adc(MCP_ADC::MCP3208);  // This line is essential

// Define the pins used for the MCP3208 inputs (example GPIO pins on ESP32)
const int inputPins[] = {32, 33, 34, 35}; 

void setup() {
  Serial.begin(115200);

  // Initialize the MCP3208
  if (!adc.begin(CS_PIN)) {
    Serial.println("Failed to initialize MCP3208.");
    while (1);
  }

  Serial.println("MCP3208 initialized successfully.");
}

void loop() {
  // Read the first four channels of the MCP3208
  for (int i = 0; i < 4; i++) {
    int value = adc.analogRead(i);
    Serial.print("Channel ");
    Serial.print(i);
    Serial.print(" (Pin GPIO");
    Serial.print(inputPins[i]);
    Serial.print("): ");
    Serial.println(value);
  }

  delay(1000);
}
 of Vb
const float PD_logic = 2.0; // Potential divider of Vlogic 


const int numPoints = 11;
const float voltageTable[numPoints] = {16.8, 16.4, 16.0, 15.6, 15.2, 14.8, 14.4, 14.0, 13.6, 13.2, 12.8};
const float socTable[numPoints] = {100, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0};

float soc; // State of Charge (%)
unsigned long lastUpdateTime = 0; // To track time intervals
float accumulatedCharge = 0.0; // mAh
float BATTERY_CAPACITY = 2000;


float voltage_SOC(float v_battery) {
  // If voltage is out of the bounds of the table, return the max and min 
  if (v_battery >= voltageTable[0]) return socTable[0];
  if (v_battery <= voltageTable[numPoints - 1]) return socTable[numPoints - 1];


 
  for (int i = 0; i < numPoints - 1; i++) {
    if (v_battery <= voltageTable[i] &&v_battery > voltageTable[i + 1]) {
     
      float soc = socTable[i] + (socTable[i + 1] - socTable[i]) * (v_battery - voltageTable[i]) / (voltageTable[i + 1] - voltageTable[i]);
      return soc;
    }
  }
  
  return 0; // Should never reach here
}
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  // measure initial value 
  int initialADC_Vb = analogRead(Vbat_PIN);
  float initialVb = (initialADC_Vb / MAX_12bits) * MAX_ADC * PD_Vb;
  soc = voltage_SOC(initialVb);
  lastUpdateTime = millis();
}

void loop() {
  // Read differential voltage with gain, and convert to unity differential voltage
  int ADC_VDM = analogRead(VDM_PIN);
  float VDM = (ADC_VDM / (float)MAX_12bits) * (MAX_ADC / G_motor);

  int ADC_VM = analogRead(Vmotor_PIN); 
  float VM = (ADC_VM / (float)MAX_12bits) * (MAX_ADC * PD_motor);
  float IM = VDM / R; // Motor current

  float PM = IM * VM; // Motor power

  // Read logic differential voltage with gain, and convert to unity differential voltage
  int ADC_VDL = analogRead(VDL_PIN);
  float VDL = (ADC_VDL / (float)MAX_12bits) * (MAX_ADC / G_logic);

  int ADC_VL = analogRead(Vlogic_PIN);
  float VL = (ADC_VL / (float)MAX_12bits) * (MAX_ADC * PD_logic);
  float IL = VDL / R; // Logic current

  float PL = IL * VL; // Logic power

  // Print motor measurements
  Serial.print("Motor Current: ");
  Serial.print(IM);
  Serial.print(" A, Motor Voltage: ");
  Serial.print(VM);
  Serial.print(" V, Motor Power: ");
  Serial.print(PM);
  Serial.println(" W");

  // Print logic measurements
  Serial.print("Logic Current: ");
  Serial.print(IL);
  Serial.print(" A, Logic Voltage: ");
  Serial.print(VL);
  Serial.print(" V, Logic Power: ");
  Serial.print(PL);
  Serial.println(" W");
//
unsigned long currentTime = millis();
unsigned long elapsedTime = currentTime - lastUpdateTime;

int ADC_Vb = analogRead(Vbat_PIN);
float Vb = (ADC_Vb / MAX_12bits) * MAX_ADC * PD_Vb;
float I_bat = IM + (IL * (5.0 /Vb))*0.93;
float Ibat_mA = I_bat * 1000.0; // Convert to mA
  accumulatedCharge += (Ibat_mA * (elapsedTime / 3600000.0)); // Charge in mAh
float chargeSoc = 100.0 - ((accumulatedCharge / BATTERY_CAPACITY) * 100.0);
float voltageSoc = voltage_SOC(Vb);

Serial.print("voltage soc");
Serial.println(voltageSoc);
Serial.print("charge soc");
Serial.println(chargeSoc);
  delay(1000); // Delay for 1 second before next measurement
}
