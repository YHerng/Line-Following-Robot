#include <Arduino.h>
#include <Servo.h>

// Arduino Uno pin map. Keep this map the same in both sketches.
const uint8_t ENA = 3;  // Left motor enable (PWM-capable, used as on/off)
const uint8_t IN1 = 13;
const uint8_t IN2 = 10;
const uint8_t ENB = 5;  // Right motor enable (PWM-capable, used as on/off)
const uint8_t IN3 = 4;
const uint8_t IN4 = 2;

const uint8_t SENSOR_FAR_LEFT  = 12;
const uint8_t SENSOR_LEFT      = 11;
const uint8_t SENSOR_CENTER    = 8;
const uint8_t SENSOR_RIGHT     = 7;
const uint8_t SENSOR_FAR_RIGHT = 6;
const uint8_t SERVO_PIN        = 9;

// Change to HIGH if your sensor outputs HIGH over black.
const uint8_t SENSOR_ACTIVE_LEVEL = LOW;

// Set either flag true if that wheel's forward direction is reversed.
const bool INVERT_LEFT_MOTOR = true;
const bool INVERT_RIGHT_MOTOR = false;

const uint8_t SERVO_NEUTRAL_DEG = 0;
const uint8_t SERVO_ACTUATE_DEG = 45;
const unsigned long SERVO_HOLD_MS = 500;
const unsigned long LOOP_INTERVAL_MS = 5;

Servo markerServo;

enum MotorCommand : uint8_t { MOTOR_STOP, MOTOR_FORWARD, MOTOR_REVERSE };
enum ServoPhase : uint8_t { SERVO_IDLE, SERVO_AT_TARGET, SERVO_RETURNING };

ServoPhase servoPhase = SERVO_IDLE;
unsigned long servoPhaseStart = 0;
unsigned long lastLoop = 0;
bool servoArmed = true;

void applyMotor(uint8_t enablePin, uint8_t inA, uint8_t inB,
                MotorCommand command, bool inverted) {
  if (command == MOTOR_STOP) {
    digitalWrite(enablePin, LOW);
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    return;
  }

  const bool forward = (command == MOTOR_FORWARD) ^ inverted;
  digitalWrite(inA, forward ? HIGH : LOW);
  digitalWrite(inB, forward ? LOW : HIGH);
  digitalWrite(enablePin, HIGH);
}

void drive(MotorCommand left, MotorCommand right) {
  applyMotor(ENA, IN1, IN2, left, INVERT_LEFT_MOTOR);
  applyMotor(ENB, IN3, IN4, right, INVERT_RIGHT_MOTOR);
}

void updateServo(unsigned long now) {
  if (servoPhase == SERVO_AT_TARGET && now - servoPhaseStart >= SERVO_HOLD_MS) {
    markerServo.write(SERVO_NEUTRAL_DEG);
    servoPhase = SERVO_RETURNING;
    servoPhaseStart = now;
  } else if (servoPhase == SERVO_RETURNING && now - servoPhaseStart >= SERVO_HOLD_MS) {
    servoPhase = SERVO_IDLE;
  }
}

void triggerServo(unsigned long now) {
  if (servoPhase != SERVO_IDLE) return;
  markerServo.write(SERVO_ACTUATE_DEG);
  servoPhase = SERVO_AT_TARGET;
  servoPhaseStart = now;
}

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  drive(MOTOR_STOP, MOTOR_STOP);

  pinMode(SENSOR_FAR_LEFT, INPUT);
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_CENTER, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);
  pinMode(SENSOR_FAR_RIGHT, INPUT);

  markerServo.attach(SERVO_PIN, 544, 2400);
  markerServo.write(SERVO_NEUTRAL_DEG);
  Serial.println(F("Line follower ready"));
  lastLoop = millis();
}

void loop() {
  const unsigned long now = millis();
  updateServo(now);
  if (now - lastLoop < LOOP_INTERVAL_MS) return;
  lastLoop = now;

  // Read each input once so every decision uses one consistent sensor sample.
  const bool farLeft = digitalRead(SENSOR_FAR_LEFT) == SENSOR_ACTIVE_LEVEL;
  const bool left = digitalRead(SENSOR_LEFT) == SENSOR_ACTIVE_LEVEL;
  const bool center = digitalRead(SENSOR_CENTER) == SENSOR_ACTIVE_LEVEL;
  const bool right = digitalRead(SENSOR_RIGHT) == SENSOR_ACTIVE_LEVEL;
  const bool farRight = digitalRead(SENSOR_FAR_RIGHT) == SENSOR_ACTIVE_LEVEL;
  const bool allBlack = farLeft && left && center && right && farRight;
  const bool anyBlack = farLeft || left || center || right || farRight;

  if (allBlack) {
    drive(MOTOR_STOP, MOTOR_STOP);
    if (servoArmed) {
      triggerServo(now);
      servoArmed = false;
      Serial.println(F("All black: stopped; servo triggered"));
    }
    return;
  }

  // Leaving the marker arms the servo for the next marker.
  servoArmed = true;

  if (!anyBlack) {
    drive(MOTOR_STOP, MOTOR_STOP);
  } else if (farLeft) {
    drive(MOTOR_REVERSE, MOTOR_FORWARD);  // Sharp left pivot
  } else if (farRight) {
    drive(MOTOR_FORWARD, MOTOR_REVERSE);  // Sharp right pivot
  } else if (left) {
    drive(MOTOR_STOP, MOTOR_FORWARD);     // Gentle left correction
  } else if (right) {
    drive(MOTOR_FORWARD, MOTOR_STOP);     // Gentle right correction
  } else if (center) {
    drive(MOTOR_FORWARD, MOTOR_FORWARD);
  } else {
    drive(MOTOR_STOP, MOTOR_STOP);
  }
}
