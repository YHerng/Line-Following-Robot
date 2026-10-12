# Line Following Robot

A 5-channel IR line follower built around the **Arduino Uno**, an **L298N**
dual H-bridge motor driver, and a hobby **servo** that actuates a mechanism when
the robot reaches an all-black stop marker.

The robot follows a black line on a white surface, steering proportionally to how
far off-centre the line is, and stops at the end of the track.

---

## Table of contents

- [Repository layout](#repository-layout)
- [Features](#features)
- [Hardware](#hardware)
- [Pin map](#pin-map)
- [Wiring](#wiring)
- [Power notes](#power-notes)
- [Building and uploading](#building-and-uploading)
- [How it works](#how-it-works)
- [Tuning](#tuning)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Repository layout

```
.
├── MAIN_CODE.ino              Single-file Arduino IDE sketch (self-contained)
├── Line Following Robot/      PlatformIO project (recommended)
│   ├── platformio.ini         Board + library configuration
│   ├── include/config.h       Pin map and all tunable constants
│   ├── src/main.cpp           Firmware (modular)
│   ├── lib/                   Project-private libraries (empty)
│   └── test/                  PlatformIO unit tests (empty)
├── LICENSE
└── README.md
```

`MAIN_CODE.ino` and `Line Following Robot/src/main.cpp` implement **identical
behaviour**; pick whichever build system you prefer. The PlatformIO version is
the one to extend, because the hardware configuration is separated into
`include/config.h`.

---

## Features

- **5-channel IR sensing** – two outer sensors for sharp pivots, two inner
  sensors for gentle corrections, one centre sensor for straight running.
- **Non-blocking control loop** – the servo sweep is driven from a state
  machine, so sensing continues while the servo moves (no `delay()` stalls).
- **Software motor polarity** – flip a single flag if a wheel spins the wrong
  way, instead of re-soldering motor leads.
- **Optional PWM speed control** – one `#define` away, for pins 3 and 5.
- **Stop-marker handling** – an all-black reading halts the robot and fires the
  servo exactly once per arrival.
- **Serial telemetry** – every decision is printed, with an optional raw
  sensor-pattern dump for debugging.

---

## Hardware

### Bill of materials

| Qty | Part | Notes |
|----:|------|-------|
| 1 | Arduino Uno | Target board (or a compatible clone) |
| 1 | L298N dual H-bridge module | Motor driver |
| 2 | DC gear motors | With wheels |
| 1 | 5-channel IR line sensor array | Digital outputs |
| 1 | Hobby servo (e.g. SG90) | Stop-marker actuator |
| 1 | Battery pack | 7–12 V for the motors (e.g. 4×AA or 2S Li-ion) |
| 1 | Chassis | Robot body |
| — | Jumper wires | — |

### Pin map

| Function | Arduino pin | Notes |
|----------|-------------|-------|
| Left motor enable (ENA) | `D3` | PWM capable |
| Left motor direction (IN1) | `D13` | |
| Left motor direction (IN2) | `D10` | |
| Right motor enable (ENB) | `D5` | PWM capable |
| Right motor direction (IN3) | `D4` | |
| Right motor direction (IN4) | `D2` | |
| Sensor – far left | `D12` | |
| Sensor – left | `D11` | |
| Sensor – centre | `D8` | |
| Sensor – right | `D7` | |
| Sensor – far right | `D6` | |
| Servo signal | `D9` | PWM capable |

All pin assignments live in [`Line Following Robot/include/config.h`](Line%20Following%20Robot/include/config.h).

---

## Wiring

```
                     Arduino Uno
                 ┌───────────────┐
   L298N ENA ────┤ D3            │
   L298N IN1 ────┤ D13           │
   L298N IN2 ────┤ D10           │
   L298N ENB ────┤ D5            │
   L298N IN3 ────┤ D4            │
   L298N IN4 ────┤ D2            │
                  │               │
  Sensor FL ─────┤ D12           │
  Sensor L  ─────┤ D11           │
  Sensor C  ─────┤ D8            │
  Sensor R  ─────┤ D7            │
  Sensor FR ─────┤ D6            │
                  │               │
  Servo sig ─────┤ D9            │
                  │               │
   Battery + ────┤ VIN  (7–12 V)  │
   Battery − ────┤ GND           │
                 └───────────────┘
```

**Connections**

- Motors and the L298N are powered from the battery pack; feed the battery into
  the Uno's `VIN` (7–12 V recommended) or power the Uno separately over USB.
- Tie every ground together: battery −, L298N `GND`, sensor `GND`, servo `GND`,
  and the Uno `GND`.
- L298N outputs `OUT1/OUT2` drive the left motor, `OUT3/OUT4` the right motor.
- If you feed 12 V into the L298N, remove its onboard 5 V jumper and power the
  driver's logic side from the Uno's 5 V pin instead.

---

## Power notes

The Uno runs on **5 V logic**, so the L298N and typical 5 V IR modules are a
natural match — no level shifting needed.

A few things that will save you a debugging session:

- **Never power the motors from the Uno's 5 V pin.** They draw far more current
  than the onboard regulator can supply. Use the battery pack through the L298N.
- **Power the servo from a separate 5 V supply**, not from the board. A stalling
  servo can draw well over 1 A and will brown out the MCU, causing random
  resets. Share the ground.
- **Common ground is mandatory.** If the Uno, L298N and sensors do not share a
  ground, the sensor readings will be nonsense.
- If you are using a clone Uno with a CH340 USB chip, you may need to install a
  driver before the board appears as a serial port.

---

## Building and uploading

### PlatformIO (recommended)

```bash
cd "Line Following Robot"
pio run -t upload        # build and flash
pio device monitor       # watch the serial output
```

The environment is named `uno` and is the default, so no `-e` flag is needed.
PlatformIO fetches the AVR toolchain and the Servo library on first run.

If you have more than one board attached, specify the port:

```bash
pio run -t upload --upload-port COM5            # Windows
pio run -t upload --upload-port /dev/ttyUSB0    # Linux
```

### Arduino IDE

Open `MAIN_CODE.ino` directly, select **Arduino Uno** as the board and the
correct serial port, then upload. The sketch is fully self-contained.

---

## How it works

### Sensor logic

Each of the five sensors reports whether it currently sits over the black line.
The firmware turns that 5-bit pattern into a motor command, checking the
outermost sensors first so sharp corrections win over gentle ones.

### Control table

| Condition | Left motor | Right motor | Result |
|-----------|-----------|-------------|--------|
| Far left sees black | Reverse | Forward | Sharp left pivot |
| Left sees black | Stop | Forward | Gentle left turn |
| Centre sees black | Forward | Forward | Straight |
| Right sees black | Forward | Stop | Gentle right turn |
| Far right sees black | Forward | Reverse | Sharp right pivot |
| All white | Stop | Stop | Line lost – stop |
| All black | Stop | Stop | Stop marker – fire servo once |

### Stop marker and servo

When all five sensors read black, the robot halts and the servo sweeps to
`SERVO_ACTUATE_DEG`, holds, and returns to `SERVO_NEUTRAL_DEG`. The sweep runs
non-blocking. A `servoArmed` latch ensures it fires **once per arrival**: it
re-arms only after the robot leaves the black marker.

---

## Tuning

Every constant lives in [`Line Following Robot/include/config.h`](Line%20Following%20Robot/include/config.h).

| Constant | Default | What it does |
|----------|---------|--------------|
| `SENSOR_ACTIVE_LEVEL` | `HIGH` | Level a sensor reports over the black line. Set to `LOW` if your sensors read inverted. |
| `INVERT_LEFT_MOTOR` | `0` | Set to `1` if the left wheel turns the wrong way. |
| `INVERT_RIGHT_MOTOR` | `0` | Set to `1` if the right wheel turns the wrong way. |
| `USE_PWM_SPEED` | `0` | `1` enables `analogWrite` speed control (needs PWM pins 3 and 5). |
| `MOTOR_SPEED` | `255` | 0–255 duty cycle, used only when `USE_PWM_SPEED` is `1`. |
| `LOOP_INTERVAL_MS` | `5` | Control-loop period. Lower = more responsive. |
| `SERVO_HOLD_MS` | `500` | Time the servo spends at each position. |
| `SERVO_ACTUATE_DEG` | `45` | Servo angle for the stop-line action. |
| `SERIAL_BAUD` | `115200` | Serial speed. |
| `DEBUG_SENSORS` | `0` | `1` prints the raw 5-bit sensor pattern every loop. |

**Bring-up order**

1. Flash the firmware with the robot lifted off the ground.
2. Open the serial monitor and confirm the printed decisions match what the
   sensors see (`DEBUG_SENSORS = 1` helps).
3. Check each wheel turns the right way; set the `INVERT_*` flags if not.
4. Place the robot on the line and confirm it tracks, then tune
   `LOOP_INTERVAL_MS` / `MOTOR_SPEED` for your track.

---

## Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| Robot drives backwards | Swap `INVERT_LEFT_MOTOR` / `INVERT_RIGHT_MOTOR`, or swap the two motor leads on that channel. |
| Robot spins in place | One motor polarity is inverted. Set the matching `INVERT_*` flag. |
| Robot steers away from the line | Sensors read inverted – change `SENSOR_ACTIVE_LEVEL`. |
| Servo twitches / board resets when the servo moves | Servo is drawing too much current – give it its own 5 V supply and a common ground. |
| Sensors give random readings | Missing common ground, or the sensors sit too far from the surface – adjust their height or the sensitivity trimmers. |
| Nothing on the serial monitor | Baud-rate mismatch – the sketch uses 115200. |
| Upload fails / no port listed | Wrong port selected, or a missing CH340 driver on clone boards. |

---

## License

Released under the [MIT License](LICENSE).
