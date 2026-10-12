/**
 * Line Following Robot - firmware
 * ===========================================================================
 *
 * 5-channel IR array + L298N dual H-bridge + one servo used to actuate a
 * mechanism when the robot reaches an all-black stop marker.
 *
 * Behaviour (see the control table in the README):
 *
 *   FAR LEFT  black -> sharp left  pivot  (left reverse, right forward)
 *   LEFT      black -> gentle left turn   (left stop,    right forward)
 *   CENTER    black -> drive straight     (both forward)
 *   RIGHT     black -> gentle right turn  (left forward, right stop)
 *   FAR RIGHT black -> sharp right pivot  (left forward, right reverse)
 *   ALL WHITE       -> stop (line lost)
 *   ALL BLACK       -> stop + fire the servo once
 *
 * The loop is non-blocking: the servo sweep is driven from a small state
 * machine, so the sensor logic keeps running while the servo moves.
 *
 * Hardware configuration lives in include/config.h.
 */

#include <Arduino.h>
#include <Servo.h>

#include "config.h"

// ===========================================================================
// Motor abstraction
// ===========================================================================

enum MotorCmd : uint8_t {
  MOTOR_STOP,
  MOTOR_FORWARD,
  MOTOR_REVERSE
};

/**
 * Enable or disable one motor channel.
 *
 * With USE_PWM_SPEED == 0 this is a plain digitalWrite, which works on every
 * board. With USE_PWM_SPEED == 1 it becomes an analogWrite so MOTOR_SPEED can
 * be used to slow the robot down.
 */
static void setEnable(uint8_t pin, bool on) {
#if USE_PWM_SPEED
  analogWrite(pin, on ? MOTOR_SPEED : 0);
#else
  digitalWrite(pin, on ? HIGH : LOW);
#endif
}

/**
 * Drive a single H-bridge channel.
 *
 * @param enPin   enable pin
 * @param inA     direction pin A
 * @param inB     direction pin B
 * @param cmd     STOP / FORWARD / REVERSE
 * @param invert  when true, FORWARD and REVERSE are swapped
 */
static void applyMotor(uint8_t enPin, uint8_t inA, uint8_t inB,
                       MotorCmd cmd, bool invert) {
  if (cmd == MOTOR_STOP) {
    setEnable(enPin, false);
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    return;
  }

  setEnable(enPin, true);

  const bool forward = (cmd == MOTOR_FORWARD) ^ invert;
  digitalWrite(inA, forward ? HIGH : LOW);
  digitalWrite(inB, forward ? LOW  : HIGH);
}

/** Command both motors at once. */
static void drive(MotorCmd left, MotorCmd right) {
  applyMotor(PIN_ENA, PIN_IN1, PIN_IN2, left,  INVERT_LEFT_MOTOR);
  applyMotor(PIN_ENB, PIN_IN3, PIN_IN4, right, INVERT_RIGHT_MOTOR);
}

static void stopMotors() {
  drive(MOTOR_STOP, MOTOR_STOP);
}

// ===========================================================================
// Sensor abstraction
// ===========================================================================

struct Sensors {
  bool farLeft;
  bool left;
  bool center;
  bool right;
  bool farRight;

  bool anyBlack() const {
    return farLeft || left || center || right || farRight;
  }
  bool allBlack() const {
    return farLeft && left && center && right && farRight;
  }
};

/** True when the given sensor currently sits over the line. */
static bool lineSeen(uint8_t pin) {
  return digitalRead(pin) == SENSOR_ACTIVE_LEVEL;
}

static Sensors readSensors() {
  Sensors s;
  s.farLeft  = lineSeen(PIN_SENSOR_FAR_LEFT);
  s.left     = lineSeen(PIN_SENSOR_LEFT);
  s.center   = lineSeen(PIN_SENSOR_CENTER);
  s.right    = lineSeen(PIN_SENSOR_RIGHT);
  s.farRight = lineSeen(PIN_SENSOR_FAR_RIGHT);
  return s;
}

// ===========================================================================
// Non-blocking servo sweep
// ===========================================================================

static Servo g_servo;

enum ServoPhase : uint8_t {
  SERVO_IDLE,
  SERVO_AT_TARGET,
  SERVO_RETURNING
};

static ServoPhase g_servoPhase = SERVO_IDLE;
static uint32_t   g_servoPhaseStart = 0;
static bool       g_servoArmed = true;   // re-armed whenever we leave the marker

static void servoInit() {
  g_servo.attach(PIN_SERVO, SERVO_MIN_US, SERVO_MAX_US);
  g_servo.write(SERVO_NEUTRAL_DEG);
  g_servoPhase = SERVO_IDLE;
}

/** Advance the servo state machine. Call this on every loop iteration. */
static void servoUpdate(uint32_t now) {
  switch (g_servoPhase) {
    case SERVO_AT_TARGET:
      if (now - g_servoPhaseStart >= SERVO_HOLD_MS) {
        g_servo.write(SERVO_NEUTRAL_DEG);
        g_servoPhase = SERVO_RETURNING;
        g_servoPhaseStart = now;
      }
      break;

    case SERVO_RETURNING:
      if (now - g_servoPhaseStart >= SERVO_HOLD_MS) {
        g_servoPhase = SERVO_IDLE;
      }
      break;

    case SERVO_IDLE:
    default:
      break;
  }
}

/** Kick off one sweep, but only if the servo is idle and armed. */
static void servoTrigger(uint32_t now) {
  if (g_servoPhase != SERVO_IDLE) {
    return;
  }
  g_servo.write(SERVO_ACTUATE_DEG);
  g_servoPhase = SERVO_AT_TARGET;
  g_servoPhaseStart = now;
}

// ===========================================================================
// Logging helpers
// ===========================================================================

#if ENABLE_SERIAL
static void logAction(const char *msg) {
  Serial.println(msg);
}

#if DEBUG_SENSORS
static void logSensors(const Sensors &s) {
  char buf[6];
  buf[0] = s.farLeft  ? '1' : '0';
  buf[1] = s.left     ? '1' : '0';
  buf[2] = s.center   ? '1' : '0';
  buf[3] = s.right    ? '1' : '0';
  buf[4] = s.farRight ? '1' : '0';
  buf[5] = '\0';
  Serial.print("sensors=");
  Serial.println(buf);
}
#endif  // DEBUG_SENSORS
#else
static void logAction(const char *) {}
#endif  // ENABLE_SERIAL

// ===========================================================================
// Setup / loop
// ===========================================================================

static uint32_t g_lastLoop = 0;

void setup() {
  // Motor driver outputs
  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  stopMotors();

  // IR sensor inputs
  pinMode(PIN_SENSOR_FAR_LEFT,  INPUT);
  pinMode(PIN_SENSOR_LEFT,      INPUT);
  pinMode(PIN_SENSOR_CENTER,    INPUT);
  pinMode(PIN_SENSOR_RIGHT,     INPUT);
  pinMode(PIN_SENSOR_FAR_RIGHT, INPUT);

  servoInit();

#if ENABLE_SERIAL
  Serial.begin(SERIAL_BAUD);
  Serial.println("==================================");
  Serial.println("  Line Following Robot - ready");
  Serial.println("==================================");
#endif

  g_lastLoop = millis();
}

void loop() {
  const uint32_t now = millis();

  // Keep the servo moving regardless of the control-loop rate.
  servoUpdate(now);

  // Fixed-rate control loop.
  if (now - g_lastLoop < LOOP_INTERVAL_MS) {
    return;
  }
  g_lastLoop = now;

  const Sensors s = readSensors();

#if ENABLE_SERIAL && DEBUG_SENSORS
  logSensors(s);
#endif

  if (s.allBlack()) {
    // Stop marker: halt and fire the servo exactly once per arrival.
    stopMotors();
    if (g_servoArmed) {
      servoTrigger(now);
      g_servoArmed = false;
      logAction("ALL BLACK  -> stop line, servo actuated");
    }
    return;
  }

  // Any other pattern means we are off the marker, so re-arm the servo.
  g_servoArmed = true;

  if (!s.anyBlack()) {
    stopMotors();
    logAction("ALL WHITE  -> line lost, stopped");
  } else if (s.farLeft) {
    drive(MOTOR_REVERSE, MOTOR_FORWARD);
    logAction("FAR LEFT   -> sharp left pivot");
  } else if (s.farRight) {
    drive(MOTOR_FORWARD, MOTOR_REVERSE);
    logAction("FAR RIGHT  -> sharp right pivot");
  } else if (s.left) {
    drive(MOTOR_STOP, MOTOR_FORWARD);
    logAction("LEFT       -> gentle left turn");
  } else if (s.right) {
    drive(MOTOR_FORWARD, MOTOR_STOP);
    logAction("RIGHT      -> gentle right turn");
  } else if (s.center) {
    drive(MOTOR_FORWARD, MOTOR_FORWARD);
    logAction("CENTER     -> straight");
  } else {
    stopMotors();
    logAction("FALLBACK   -> stopped");
  }
}
