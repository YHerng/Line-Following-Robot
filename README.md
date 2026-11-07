# Line Following Robot

Arduino Uno line follower using a five-channel digital IR sensor array, an
L298N motor driver, two DC motors, and a servo that operates at an all-black
stop marker.

## Sketches

These sketches currently contain the same firmware:

- `MAIN_CODE.ino` — main Arduino IDE sketch
- `testing_without_servo/testing_without_servo.ino` — compatibility copy

If you edit one sketch, copy the complete updated file to the other to keep
their behavior aligned. The second filename is retained for compatibility;
it now includes the servo behavior too. A matching copy also remains in the
workspace's sibling `testing_without_servo` folder.

## Arduino Uno pin map

| Function | Pin |
|---|---:|
| Left motor enable (ENA) | D3 |
| Left motor direction (IN1, IN2) | D13, D10 |
| Right motor enable (ENB) | D5 |
| Right motor direction (IN3, IN4) | D4, D2 |
| IR sensors (far left, left, center, right, far right) | D12, D11, D8, D7, D6 |
| Servo signal | D9 |

Connect the motor driver, sensor array, servo, and Arduino grounds together.
Power the motors through the L298N with a suitable motor supply. Power the
servo from a suitable regulated 5 V supply and share ground with the Arduino;
avoid powering a loaded servo from the Uno's 5 V pin.

## Behavior

| Sensor condition | Motor action |
|---|---|
| Far-left sensor sees black | Sharp left pivot: left motor reverse, right forward |
| Far-right sensor sees black | Sharp right pivot: left motor forward, right reverse |
| Left sensor sees black | Gentle left correction: left motor stopped, right forward |
| Right sensor sees black | Gentle right correction: left motor forward, right stopped |
| Center sensor sees black | Both motors forward |
| All sensors see white | Stop; line lost |
| All sensors see black | Stop and actuate the servo once for that marker |

The servo moves to 45 degrees, holds for 500 ms, returns to 0 degrees, and is
re-armed after the robot leaves the all-black marker.

## Settings to check for your hardware

At the top of the sketch, set `SENSOR_ACTIVE_LEVEL` to the electrical level
your sensor outputs over black (`LOW` or `HIGH`). The default is `LOW`; modules
vary, so check the sensor outputs before running the robot. Set
`INVERT_LEFT_MOTOR` or `INVERT_RIGHT_MOTOR` to `true` if that motor's forward
direction is reversed by your wiring.

The enable pins are switched fully on or off. The code does not currently
provide variable motor speed control. The servo angles may need adjustment for
your mechanism.

Open either `.ino` file in Arduino IDE, select Arduino Uno and the correct
serial port, then upload. Serial status runs at 115200 baud.
