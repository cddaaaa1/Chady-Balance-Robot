#include <SPI.h>

// MCP3208 pin connections
const int csPin = 53; // Chip Select pin

void setup() {
  Serial.begin(9600);

  // Initialize SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Adjust the SPI clock speed if necessary
  SPI.setDataMode(SPI_MODE0); // Set SPI mode to 0
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH); // Disable the MCP3208
}

void loop() {
  int channel = 1; // Testing only channel 1 for now
  int value = readMCP3208(channel);
  Serial.print("Channel ");
  Serial.print(channel);
  Serial.print(": ");
  Serial.println(value);

  delay(1000); // Delay for readability
}

int readMCP3208(int channel) {
  if (channel < 0 || channel > 7) {
    return -1; // Invalid channel
  }

  digitalWrite(csPin, LOW); // Enable MCP3208

  // Send start bit, single/differential bit, and channel bits
  byte command1 = 0x06 | ((channel & 0x04) >> 2); // Start bit + Single-ended
  byte command2 = (channel & 0x03) << 6; // Channel selection

  SPI.transfer(command1);
  byte highByte = SPI.transfer(command2);
  highByte = highByte & 0x0F; // Only the last 4 bits are valid
  byte lowByte = SPI.transfer(0x00); // Get the lower 8 bits

  digitalWrite(csPin, HIGH); // Disable MCP3208

  // Combine the two bytes to get the 12-bit result
  int result = (highByte << 8) | lowByte;

  // Debug output
  Serial.print("Command1: ");
  Serial.println(command1, BIN);
  Serial.print("Command2: ");
  Serial.println(command2, BIN);
  Serial.print("High Byte: ");
  Serial.println(highByte, BIN);
  Serial.print("Low Byte: ");
  Serial.println(lowByte, BIN);
  Serial.print("Result: ");
  Serial.println(result);

  return result;
}

