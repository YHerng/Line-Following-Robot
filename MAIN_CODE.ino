/**
 * Line Following Robot - single-file Arduino IDE sketch
 * ===========================================================================
 *
 * This is the standalone version of the firmware, for pasting straight into
 * the Arduino IDE. The equivalent, modular PlatformIO build lives in
 * "Line Following Robot/src/main.cpp" + "Line Following Robot/include/config.h"
 * and behaves identically.
 *
 * Hardware:
 *   - 5-channel IR line sensor array (digital outputs)
 *   - L298N dual H-bridge motor driver
 *   - 1 hobby servo, actuated once when the all-black stop marker is reached
 *
 * Note for the Arduino UNO Q: the header pins are 3.3 V logic and are NOT
 * 5 V tolerant. See the repository README before wiring 5 V modules.
 */

#include <Servo.h>

// ===========================================================================
// PIN DEFINITIONS
// ===========================================================================

// L298N Left Motor Pins
const int ENA = 3;    // Left  motor enable
const int IN1 = 13;   // Left  motor direction A
const int IN2 = 10;   // Left  motor direction B

// L298N Right Motor Pins
const int ENB = 5;    // Right motor enable
const int IN3 = 4;    // Right motor direction A
const int IN4 = 2;    // Right motor direction B

// 5-Channel IR Sensor Pins
const int SENSOR_FAR_LEFT  = 12;
const int SENSOR_LEFT      = 11;
const int SENSOR_CENTER    = 8;
const int SENSOR_RIGHT     = 7;
const int SENSOR_FAR_RIGHT = 6;

// Servo Pin
const int SERVO_PIN = 9;

// ===========================================================================
// CONSTANTS & OBJECTS
// ===========================================================================

Servo myServo;

// Electrical level reported by a sensor sitting over the BLACK line.
// Most IR arrays pull LOW on black; change to HIGH if yours read inverted.
const int SENSOR_ACTIVE_LEVEL = HIGH;

// Servo geometry and timing
const int SERVO_MIN_US     = 544;
const int SERVO_MAX_US     = 2400;
const int SERVO_NEUTRAL    = 0;
const int SERVO_ACTUATE    = 45;
const unsigned long SERVO_HOLD_MS = 500;

const unsigned long LOOP_INTERVAL_MS = 5;

// Motor polarity: flip to true if a wheel turns the wrong way.
const bool INVERT_LEFT_MOTOR  = false;
const bool INVERT_RIGHT_MOTOR = false;

// ===========================================================================
// STATE
// ===========================================================================

enum MotorCmd { MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE };

enum ServoPhase { SERVO_IDLE, SERVO_AT_TARGET, SERVO_RETURNING };

ServoPhase servoPhase = SERVO_IDLE;
unsigned long servoPhaseStart = 0;
bool servoArmed = true;             // re-armed whenever we leave the marker

unsigned long lastLoop = 0;

// ===========================================================================
// MOTOR HELPERS
// ===========================================================================

void applyMotor(int enPin, int inA, int inB, MotorCmd cmd, bool invert) {
  if (cmd == MOTOR_STOP) {
    digitalWrite(enPin, LOW);
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    return;
  }

  digitalWrite(enPin, HIGH);
  bool forward = (cmd == MOTOR_FORWARD) ^ invert;
  digitalWrite(inA, forward ? HIGH : LOW);
  digitalWrite(inB, forward ? LOW  : HIGH);
}

void drive(MotorCmd left, MotorCmd right) {
  applyMotor(ENA, IN1, IN2, left,  INVERT_LEFT_MOTOR);
  applyMotor(ENB, IN3, IN4, right, INVERT_RIGHT_MOTOR);
}

void stopMotors() {
  drive(MOTOR_STOP, MOTOR_STOP);
}

// ===========================================================================
// SERVO (non-blocking sweep)
// ===========================================================================

void servoUpdate(unsigned long now) {
  if (servoPhase == SERVO_AT_TARGET && now - servoPhaseStart >= SERVO_HOLD_MS) {
    myServo.write(SERVO_NEUTRAL);
    servoPhase = SERVO_RETURNING;
    servoPhaseStart = now;
  } else if (servoPhase == SERVO_RETURNING && now - servoPhaseStart >= SERVO_HOLD_MS) {
    servoPhase = SERVO_IDLE;
  }
}

void servoTrigger(unsigned long now) {
  if (servoPhase != SERVO_IDLE) {
    return;
  }
  myServo.write(SERVO_ACTUATE);
  servoPhase = SERVO_AT_TARGET;
  servoPhaseStart = now;
}

// ===========================================================================
// SETUP
// ===========================================================================

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  pinMode(SENSOR_FAR_LEFT,  INPUT);
  pinMode(SENSOR_LEFT,      INPUT);
  pinMode(SENSOR_CENTER,    INPUT);
  pinMode(SENSOR_RIGHT,     INPUT);
  pinMode(SENSOR_FAR_RIGHT, INPUT);

  myServo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  myServo.write(SERVO_NEUTRAL);

  Serial.println("==================================");
  Serial.println("  Line Following Robot - ready");
  Serial.println("==================================");

  lastLoop = millis();
}

// ===========================================================================
// MAIN LOOP
// ===========================================================================

void loop() {
  unsigned long now = millis();

  servoUpdate(now);

  if (now - lastLoop < LOOP_INTERVAL_MS) {
    return;
  }
  lastLoop = now;

  bool farLeft  = (digitalRead(SENSOR_FAR_LEFT)  == SENSOR_ACTIVE_LEVEL);
  bool left     = (digitalRead(SENSOR_LEFT)      == SENSOR_ACTIVE_LEVEL);
  bool center   = (digitalRead(SENSOR_CENTER)    == SENSOR_ACTIVE_LEVEL);
  bool right    = (digitalRead(SENSOR_RIGHT)     == SENSOR_ACTIVE_LEVEL);
  bool farRight = (digitalRead(SENSOR_FAR_RIGHT) == SENSOR_ACTIVE_LEVEL);

  bool allBlack = farLeft && left && center && right && farRight;
  bool anyBlack = farLeft || left || center || right || farRight;

  // --- Stop marker: halt and actuate the servo exactly once -------------
  if (allBlack) {
    stopMotors();
    if (servoArmed) {
      servoTrigger(now);
      servoArmed = false;
      Serial.println("ALL BLACK  -> stop line, servo actuated");
    }
    return;
  }

  // Off the marker -> re-arm the servo for the next stop line.
  servoArmed = true;

  // --- Steering ---------------------------------------------------------
  if (!anyBlack) {
    stopMotors();
    Serial.println("ALL WHITE  -> line lost, stopped");
  } else if (farLeft) {
    drive(MOTOR_REVERSE, MOTOR_FORWARD);   // sharp left pivot
    Serial.println("FAR LEFT   -> sharp left pivot");
  } else if (farRight) {
    drive(MOTOR_FORWARD, MOTOR_REVERSE);   // sharp right pivot
    Serial.println("FAR RIGHT  -> sharp right pivot");
  } else if (left) {
    drive(MOTOR_STOP, MOTOR_FORWARD);      // gentle left turn
    Serial.println("LEFT       -> gentle left turn");
  } else if (right) {
    drive(MOTOR_FORWARD, MOTOR_STOP);      // gentle right turn
    Serial.println("RIGHT      -> gentle right turn");
  } else if (center) {
    drive(MOTOR_FORWARD, MOTOR_FORWARD);   // straight
    Serial.println("CENTER     -> straight");
  } else {
    stopMotors();
    Serial.println("FALLBACK   -> stopped");
  }
}
