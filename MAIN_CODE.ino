#include <Servo.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

// L298N Left Motor Pins
const int ENA = 3;    // Digital Pin for Enable A
const int IN1 = 13;   // Direction pin 1
const int IN2 = 10;   // Direction pin 2

// L298N Right Motor Pins
const int ENB = 5;    // Digital Pin for Enable B
const int IN3 = 4;    // Direction pin 1
const int IN4 = 2;    // Direction pin 2

// 5-Channel IR Sensor Pins
const int SENSOR_FAR_LEFT   = 12;
const int SENSOR_LEFT       = 11;
const int SENSOR_CENTER     = 8;
const int SENSOR_RIGHT      = 7;
const int SENSOR_FAR_RIGHT  = 6;

// Servo Pin
const int SERVO_PIN = 9;

// ==========================================
// CONSTANTS & OBJECTS
// ==========================================
Servo myServo;

// Sensor Logic Value for BLACK line (Active LOW for standard IR arrays)
const int BLACK_LINE = HIGH; 

// FLAG TO PREVENT REPEATED SERVO ACTUATION
bool servoExecuted = false; 

void setup() {
  Serial.begin(9600);

  // Initialize L298N Motor Pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize IR Sensor Pins
  pinMode(SENSOR_FAR_LEFT, INPUT);
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_CENTER, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);
  pinMode(SENSOR_FAR_RIGHT, INPUT);

  // Attach Servo
  myServo.attach(SERVO_PIN, 544, 2400);
  myServo.write(0); // Default neutral position

  Serial.println("==================================");
  Serial.println("  Line Follower Logic & Circuit Test");
  Serial.println("==================================");
}

void loop() {
  // Read digital state of all 5 IR sensors
  int sFarLeft  = digitalRead(SENSOR_FAR_LEFT);
  int sLeft     = digitalRead(SENSOR_LEFT);
  int sCenter   = digitalRead(SENSOR_CENTER);
  int sRight    = digitalRead(SENSOR_RIGHT);
  int sFarRight = digitalRead(SENSOR_FAR_RIGHT);

  // -------------------------------------------------------------
  // CONDITIONAL LOGIC
  // -------------------------------------------------------------

  // Condition 1: ALL sensors detect WHITE -> Stop motors completely
  if (sFarLeft != BLACK_LINE && sLeft != BLACK_LINE && sCenter != BLACK_LINE && sRight != BLACK_LINE && sFarRight != BLACK_LINE) {
    Serial.println("Action: ALL WHITE ground detected -> Motors STOPPED");
    servoExecuted = false; 
    stopMotors();
  }

  // Condition 2: ALL sensors detect BLACK -> Stop motors & sweep servo ONCE
  else if (sFarLeft == BLACK_LINE && sLeft == BLACK_LINE && sCenter == BLACK_LINE && sRight == BLACK_LINE && sFarRight == BLACK_LINE) {
    stopMotors();
    
    if (!servoExecuted) {
      Serial.println("Action: ALL BLACK marker detected -> Actuating Servo ONCE");
      
      myServo.write(45);
      delay(500);
      myServo.write(0);
      delay(500);
      
      servoExecuted = true; 
    } else {
      Serial.println("Action: ALL BLACK -> Waiting on stop line (Servo already actuated)");
    }
  }

  // Condition 3: FAR LEFT sensor detects black -> Sharp Left Pivot (Right forward, Left REVERSE)
  else if (sFarLeft == BLACK_LINE) {
    Serial.println("Action: FAR LEFT sensor black -> Sharp Left Pivot (Right forward, Left reverse)");
    servoExecuted = false;
    setMotorLeft(true, false);   // Reverse left motor
    setMotorRight(true, false);   // Forward right motor
    Serial.print("FarLeft: "); Serial.print(sFarLeft);
    Serial.print(" | Left: "); Serial.println(sLeft);
  }

  // Condition 4: FAR RIGHT sensor detects black -> Sharp Right Pivot (Left forward, Right REVERSE)
  else if (sFarRight == BLACK_LINE) {
    Serial.println("Action: FAR RIGHT sensor black -> Sharp Right Pivot (Left forward, Right reverse)");
    servoExecuted = false;
    setMotorLeft(true, true);    // Forward left motor
    setMotorRight(true, true);  // Reverse right motor
  }

  // Condition 5: LEFT sensor detects black -> Gentle Turn Right Motor Only
  else if (sLeft == BLACK_LINE && sFarLeft != BLACK_LINE) {
    Serial.println("Action: LEFT sensor black -> Turn Right motor forward, Left stopped");
    servoExecuted = false;
    setMotorLeft(false, true);   // Stop left motor
    setMotorRight(true, true);   // Forward right motor
    Serial.print("FarLeft: "); Serial.print(sFarLeft);
    Serial.print(" | Left: "); Serial.println(sLeft);
  }

  // Condition 6: RIGHT sensor detects black -> Gentle Turn Left Motor Only
  else if (sRight == BLACK_LINE && sFarRight != BLACK_LINE) {
    Serial.println("Action: RIGHT sensor black -> Turn Left motor forward, Right stopped");
    servoExecuted = false;
    setMotorLeft(true, true);    // Forward left motor
    setMotorRight(false, true);  // Stop right motor
  }

  // Condition 7: CENTER sensor detects black -> Move Straight
  else if (sCenter == BLACK_LINE) {
    Serial.println("Action: CENTER sensor black -> Move Straight");
    servoExecuted = false;
    moveForward();
  }

  // Fallback
  else {
    Serial.println("Action: Fallback -> Stop motors");
    servoExecuted = false;
    stopMotors();
  }

  delay(50); // Small loop delay for stability
}

// ==========================================
// MOTOR CONTROL HELPER FUNCTIONS (DIGITAL)
// ==========================================

void setMotorLeft(bool enable, bool forward) {
  digitalWrite(ENA, enable ? HIGH : LOW);
  if (forward) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
}

void setMotorRight(bool enable, bool forward) {
  digitalWrite(ENB, enable ? HIGH : LOW);
  if (forward) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
}

void moveForward() {
  setMotorLeft(true, false);
  setMotorRight(true, true);
}

void stopMotors() {
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}