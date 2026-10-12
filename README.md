# Line Following Robot

A 5-channel IR line follower built around the **Arduino UNO Q**, an **L298N**
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
- [3.3 V logic warning](#33-v-logic-warning)
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
| 1 | Arduino UNO Q | Target board (2 GB/16 GB or 4 GB/32 GB variant) |
| 1 | L298N dual H-bridge module | Motor driver |
| 2 | DC gear motors | With wheels |
| 1 | 5-channel IR line sensor array | Digital outputs |
| 1 | Hobby servo (e.g. SG90) | Stop-marker actuator |
| 1 | Battery pack | 7–12 V for the motors (e.g. 4×AA or 2S Li-ion) |
| 1 | Chassis | Robot body |
| — | Jumper wires | — |

### Pin map

| Function | UNO Q pin | Notes |
|----------|-----------|-------|
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
                    Arduino UNO Q
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
   Battery + ────┤ VIN  (7–24 V)  │
   Battery − ────┤ GND           │
                 └───────────────┘
```

**Power and ground**

- Motors and the L298N are powered from the battery pack; the UNO Q's `VIN`
  accepts **7–24 V**.
- Tie every ground together: battery −, L298N `GND`, sensor `GND`, servo `GND`,
  and the UNO Q `GND`.
- Power the **servo from a separate 5 V supply**, not from the board's 3.3 V
  rail — a stalling servo can draw well over 1 A and will brown out the MCU.
- L298N outputs `OUT1/OUT2` drive the left motor, `OUT3/OUT4` the right motor.

---

## 3.3 V logic warning

> **Important.** The UNO Q's header pins are **3.3 V logic and are NOT 5 V
> tolerant**. This is
the single biggest difference from a classic Uno, and it matters here:

- Many 5-channel IR modules and most L298N boards are designed for 5 V logic.
  Driving a 5 V signal into a UNO Q GPIO can damage the STM32U585.
- Use 3.3 V-compatible sensors, or level-shift the sensor outputs down to 3.3 V.
- The L298N's `5V` output is a 5 V rail — do **not** feed it into any UNO Q
  input pin.
- `A0`/`A1` are direct ADC inputs with an absolute maximum of about 3.6 V.

If you are unsure, measure the sensor outputs with a multimeter (with the
sensors powered) before connecting them to the board.

---

## Building and uploading

### PlatformIO – UNO Q (recommended)

The UNO Q has **no official PlatformIO support**. This project uses the
community platform [`lee-lab-skku/arduinoq`](https://github.com/lee-lab-skku/platform-arduinoq),
which `platformio.ini` already points at, so PlatformIO fetches it on first use.

> **The UNO Q firmware can only be *built* on Linux.** The platform's toolchain
> and framework packages are Linux-only. On Windows or macOS you must build on
> the board itself (see [remote builds](#building-from-windows-or-macos)).

On the board (over SSH, or in an editor session attached to the board):

```bash
cd "Line Following Robot"
pio run                 # build
pio run -t upload       # build and flash
pio device monitor -e uno_q   # watch the serial output
```

### Building from Windows or macOS

Use PlatformIO's remote mode with `--force-remote` so the whole build runs on
the board:

```bash
pio remote run -r -t upload
pio remote device monitor -e uno_q
```

### Classic Arduino Uno (bench testing)

A second environment builds the same source for a normal 5 V Uno/Nano — useful
for testing the logic without an UNO Q. This one builds locally on any OS:

```bash
cd "Line Following Robot"
pio run -e uno -t upload
```

### Arduino IDE

Open `MAIN_CODE.ino` directly. For the UNO Q, install the Arduino UNO Q board
package in the Boards Manager and select **Arduino UNO Q**; for a classic Uno,
just select **Arduino Uno**. The sketch is fully self-contained.

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
| No serial output on the UNO Q | `Serial` needs the RouterBridge library (already in `platformio.ini`) and `arduino-router.service` running on the MPU. |
| `pio run` fails immediately on Windows/macOS | Expected – the UNO Q platform builds only on Linux. Use `pio remote run -r -t upload`. |
| Build fails on `#include <Servo.h>` | Ensure `arduino-libraries/Servo` is at **1.3.0 or newer** – that is the first release with UNO Q / Zephyr support. |

---

## License

Released under the [MIT License](LICENSE).
