/**
 * config.h - Hardware pin map and tunable constants for the line following robot.
 *
 * Everything you are likely to change while bringing the robot up lives here,
 * so that src/main.cpp can stay focused on the control logic.
 */
#pragma once

#include <Arduino.h>

// ===========================================================================
// 1. MOTOR DRIVER  (L298N dual H-bridge)
// ===========================================================================
#define PIN_ENA  3    // Left  motor enable  (PWM capable)
#define PIN_IN1 13    // Left  motor direction A
#define PIN_IN2 10    // Left  motor direction B

#define PIN_ENB  5    // Right motor enable  (PWM capable)
#define PIN_IN3  4    // Right motor direction A
#define PIN_IN4  2    // Right motor direction B

// ===========================================================================
// 2. IR LINE SENSOR ARRAY  (5 channel, digital outputs)
// ===========================================================================
#define PIN_SENSOR_FAR_LEFT  12
#define PIN_SENSOR_LEFT      11
#define PIN_SENSOR_CENTER     8
#define PIN_SENSOR_RIGHT      7
#define PIN_SENSOR_FAR_RIGHT  6

// ===========================================================================
// 3. SERVO  (actuated once when the robot reaches the all-black stop marker)
// ===========================================================================
#define PIN_SERVO          9
#define SERVO_MIN_US     544    // pulse width for 0 degrees
#define SERVO_MAX_US    2400    // pulse width for the servo's maximum angle
#define SERVO_NEUTRAL_DEG  0    // resting position
#define SERVO_ACTUATE_DEG 45    // position used for the stop-line action
#define SERVO_HOLD_MS    500    // time spent at each position (non-blocking)

// ===========================================================================
// 4. SENSOR LOGIC
// ===========================================================================
// Electrical level the sensor reports while it is over the BLACK line.
// Most IR arrays pull their output LOW on black -> set this to LOW.
// The original sketch used HIGH; keep HIGH only if your modules read inverted.
#define SENSOR_ACTIVE_LEVEL HIGH

// ===========================================================================
// 5. MOTOR POLARITY
// ===========================================================================
// Set a flag to 1 if that wheel turns the wrong way. This corrects mirrored
// wiring or gearing in software instead of forcing you to swap motor leads.
#define INVERT_LEFT_MOTOR   0
#define INVERT_RIGHT_MOTOR  0

// ===========================================================================
// 6. SPEED CONTROL
// ===========================================================================
// 0 = simple on/off drive (digitalWrite)  -> maximum hardware compatibility
// 1 = PWM speed control via analogWrite   -> requires PWM pins 3 and 5
#define USE_PWM_SPEED 0
#define MOTOR_SPEED   255   // 0..255, only used when USE_PWM_SPEED == 1

// ===========================================================================
// 7. TIMING AND DIAGNOSTICS
// ===========================================================================
#define LOOP_INTERVAL_MS  5      // control loop period in milliseconds
#define SERIAL_BAUD       115200 // UNO Q RouterBridge default; native UART on AVR
#define ENABLE_SERIAL     1      // set to 0 to remove all Serial traffic
#define DEBUG_SENSORS     0      // set to 1 to print the 5-bit sensor pattern
