#include <Servo.h>        // Library to control servo motors
#include <NewPing.h>      // Library for ultrasonic sensor (HC-SR04)
#include <SPI.h>          // SPI communication library
#include <nRF24L01.h>     // NRF24L01 wireless module library
#include <RF24.h>         // RF24 wireless communication library

// Motor control pins
#define ENA 2            
#define IN1 3            
#define IN2 4            
#define IN3 5            
#define IN4 6            
#define ENB 7            

// Movement directions
#define FORWARD 1
#define BACKWARD 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

// Servo and Ultrasonic sensor pins
#define SERVO_PIN A2       
#define TRIG_PIN A4        
#define ECHO_PIN A5        
#define DISTANCE_CHECK 20  // Default obstacle detection distance (20 cm)

// Default motor speed and control mode
int basePace = 170;           // Motor speed (0-255)
int mode = 5;             // 5: Web Control (default), 6: Autonomous, 7: Hand Gesture

// Initialize ultrasonic sensor and servo motor
NewPing sonar(TRIG_PIN, ECHO_PIN, 400);
/*float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = duration * 0.03432 / 2;
  return distance;
}*/

Servo myServo;

// NRF24L01 wireless module setup
RF24 radio(9, 10);       // CE on pin 9, CSN on pin 10
const byte address[4] = {0b10100100, 0b11000011, 0b11100010, 0b11110010};  // NRF24 address
char handCommand;        // Variable to store hand gesture commands

// PID variables
float kp = 3.0, ki = 0.1, kd = 0.8;      // PID constants
float preError = 0, integral = 0;
int setpoint = 20;

// ====== SETUP FUNCTION ======
void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud rate

  // Configure motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Set initial motor speed
  analogWrite(ENA, basePace);
  analogWrite(ENB, basePace);

  // Initialize servo motor position to 90 degrees (center)
  myServo.write(90);
  myServo.attach(SERVO_PIN);

  // Initialize NRF24L01 radio communication
  radio.begin();
  radio.openReadingPipe(0, address);  // Set communication address
  radio.setPALevel(RF24_PA_HIGH);     // Set power level to high
  radio.setChannel(25);
  radio.startListening();             // Start listening for incoming signals
}

// ====== MAIN LOOP ======
void loop() {
  // Check if there is data from ESP32-CAM through serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read incoming command
    Serial.println("Received: " + command);         // Print received command for debugging

    // Update mode based on received command
    if (command == "Web") {
      mode = 5;
    } else if (command == "Autonomous") {
      mode = 6;
    } else if (command == "Hand") {
      mode = 7;
    } else if (command == "Follow") {
      mode = 8;
    }
  }

  // Execute different control modes based on 'mode' variable
  if (mode == 5) {
    handleWebControl();
  } 
  else if (mode == 6) {
    handleAutonomous();
  } 
  else if (mode == 7) {
    handleHandGesture();
  }
  else if (mode == 8) {
    handleFollow();
  }
}

// ===== WEB CONTROL MODE =====
void handleWebControl() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');  // Read incoming command
    int commaIndex = command.indexOf(',');          // Find comma position

    if (commaIndex > 0) {
      String key = command.substring(0, commaIndex);  // Extract the command type (MoveCar, Speed)
      String value = command.substring(commaIndex + 1);  // Extract the value (e.g., 1, 255)

      if (key == "MoveCar") {
        int moveValue = value.toInt();
        moveCar(moveValue);  // Move the car based on received value
      } 
      else if (key == "Speed") {
        int speed = value.toInt();
        setPace(speed);  // Adjust motor speed
      }
    }
  }
}

// ===== AUTONOMOUS MODE =====
void handleAutonomous() {
  int distance = sonar.ping_cm();  // Measure distance using ultrasonic sensor

  // Obstacle detected within DISTANCE_CHECK
  if (distance > 0 && distance < 20) {
    moveCar(STOP);  // Stop the car
    delay(500);

    moveCar(BACKWARD);  // Reverse
    delay(100);
    moveCar(STOP);

    // Scan left
    myServo.write(180);
    delay(500);
    int distanceLeft = sonar.ping_cm();

    // Scan right
    myServo.write(0);
    delay(500);
    int distanceRight = sonar.ping_cm();

    // Return servo to center
    myServo.write(90);
    delay(500);

    // Decide movement based on obstacle distances
    if (distanceLeft == 0 && distanceRight == 0) {
      moveCar(BACKWARD);
    } 
    else if (distanceLeft == 0) {
      moveCar(LEFT);
      delay(200);
    } 
    else if (distanceRight == 0) {
      moveCar(RIGHT);
      delay(200);
    } 
    else if (distanceLeft >= distanceRight) {
      moveCar(LEFT);
      delay(200);
    } 
    else {
      moveCar(RIGHT);
      delay(200);
    }
  } 
  else {
    moveCar(FORWARD);  // No obstacle, continue moving forward
  }
}

// ===== HAND GESTURE MODE =====
void handleHandGesture() {
  if (radio.available()) {
    radio.read(&handCommand, sizeof(handCommand));  // Read hand gesture command
    Serial.print("Command: ");
    Serial.println(handCommand);  // Debugging

    // Execute car movement based on hand gesture
    switch (handCommand) {
      case 'F': moveCar(FORWARD); break;
      case 'B': moveCar(BACKWARD); break;
      case 'L': moveCar(LEFT); break;
      case 'R': moveCar(RIGHT); break;
      case 'S': moveCar(STOP); break;
      default:  moveCar(STOP); break;
    }
  }
}

// ===== FOLLOW MODE =====
void handleFollow() {
  int distance = sonar.ping_cm();      // Measure distance using Ultrasonic

  if (distance > 0) {
    // Calculate error
    float error = setpoint - distance;

    // PID calculations
    integral += error;      // Accumulate error
    integral = constrain(integral, -100, 100);
    float derivative = error - preError;
    float output = kp * abs(error) + ki * integral + kd * derivative;

    // Determine pace
    int pace;
    if (distance > setpoint) {
      pace = constrain(basePace + output, basePace, 255);
    } else if (distance <= setpoint && distance > 10) {
      pace = constrain(basePace - output, 130, basePace);
    } else {
      pace = 0;
    }

    // Adjust movement based on error sign
    if (distance <= 10) {
      // Stop the car if distance less than 8cm
      moveCar(STOP);
    }
    else if (distance > setpoint) {
      // Distance greater than setpoint => Speed up
      moveCar(FORWARD);
      analogWrite(ENA, pace);
      analogWrite(ENB, pace);
    }
    else {
      // Distance less than setpoint => Slow down
      moveCar(FORWARD);
      analogWrite(ENA, pace - 20);
      analogWrite(ENB, pace - 20);
    }

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm, Error: ");
    Serial.print(error);
    Serial.print(", PID Output: ");
    Serial.print(output);
    Serial.print(", Speed: ");
    Serial.println(pace);

    // Update previous error
    preError = error;
  } else {
    moveCar(STOP);      // Stop if no valid distance
    Serial.println("No valid distance measured, stopping...");
  }

  delay(50);
}

// ===== MOTOR CONTROL FUNCTION =====
void moveCar(int direction) {
  switch (direction) {
    case FORWARD:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case BACKWARD:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    case LEFT:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    case RIGHT:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case STOP:
    default:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
  }
}

// ===== SET MOTOR SPEED =====
void setPace(int speed) {
  basePace = speed;
  analogWrite(ENA, basePace);
  analogWrite(ENB, basePace);
}