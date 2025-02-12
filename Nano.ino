#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <MPU6050.h>

// MPU-6050 declaration
MPU6050 mpu;

// NRF24L01 declaration
RF24 radio(9, 10); // CE = 9, CSN = 10
// Receiver communication address
const byte address[4] = {0b10100100, 0b11000011, 0b11100010, 0b11110010};

char command;

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // Start MPU-6050
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connected!");
  } else {
    Serial.println("MPU6050 not connected!");
    while (1);  // Stop if no connection to MPU-6050
  }

  // Start NRF24L01
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_HIGH); // Can try RF24_PA_LOW if need closer range
  radio.setChannel(25);
}

void loop() {
  // Read data from MPU-6050
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  int threshold = 7000;
  
  // Signal processing from MPU-6050
  if (ax < -threshold) {
    command = 'F'; // Forward
  } else if (ax > threshold) {
    command = 'B'; // Backward
  } else if (ay > threshold) {
    command = 'R'; // Turn right
  } else if (ay < -threshold) {
    command = 'L'; // Turn left
  } else {
    command = 'S'; // Stop
  }

  // Send command via NRF24L01
  radio.write(&command, sizeof(command));
  Serial.print("Command: ");
  Serial.println(command);

  delay(100);
}