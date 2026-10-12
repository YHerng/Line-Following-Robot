# Line Following Robot (PlatformIO project)

PlatformIO build for the 5-channel IR line follower. Full documentation,
including the wiring diagram and the 3.3 V safety warning, lives in the
[repository README](../README.md).

## Structure

| Path | Contents |
|------|----------|
| `platformio.ini` | Board and library configuration |
| `include/config.h` | Pin map and all tunable constants |
| `src/main.cpp` | Firmware |
| `lib/` | Project-private libraries (unused) |
| `test/` | PlatformIO unit tests (unused) |

## Environments

| Environment | Board | Notes |
|-------------|-------|-------|
| `uno_q` (default) | Arduino UNO Q | Builds **only on Linux**; use remote builds from Windows/macOS |
| `uno` | Classic Arduino Uno | Builds anywhere; useful for bench testing |

## Commands

```bash
# UNO Q - run on the board / a Linux host
pio run
pio run -t upload
pio device monitor -e uno_q

# From Windows or macOS - build on the board
pio remote run -r -t upload

# Classic Uno - builds locally on any OS
pio run -e uno -t upload
```

## Configuration

All hardware settings are in [`include/config.h`](include/config.h):

- Pin assignments for the motors, sensors and servo
- `SENSOR_ACTIVE_LEVEL` – level a sensor reports over the black line
- `INVERT_LEFT_MOTOR` / `INVERT_RIGHT_MOTOR` – fix a wheel that spins the wrong way
- `USE_PWM_SPEED` / `MOTOR_SPEED` – optional PWM speed control
- `LOOP_INTERVAL_MS`, servo timing, and serial options
