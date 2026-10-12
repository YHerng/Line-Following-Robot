# Line Following Robot (PlatformIO project)

PlatformIO build for the 5-channel IR line follower. Full documentation,
including the wiring diagram and power notes, lives in the
[repository README](../README.md).

## Structure

| Path | Contents |
|------|----------|
| `platformio.ini` | Board and library configuration |
| `include/config.h` | Pin map and all tunable constants |
| `src/main.cpp` | Firmware |
| `lib/` | Project-private libraries (unused) |
| `test/` | PlatformIO unit tests (unused) |

## Board

Arduino Uno (`atmelavr` platform), environment `uno`.

## Commands

```bash
pio run -t upload          # build and flash
pio device monitor         # watch the serial output

# pick a specific board if several are attached
pio run -t upload --upload-port COM5
```

## Configuration

All hardware settings are in [`include/config.h`](include/config.h):

- Pin assignments for the motors, sensors and servo
- `SENSOR_ACTIVE_LEVEL` – level a sensor reports over the black line
- `INVERT_LEFT_MOTOR` / `INVERT_RIGHT_MOTOR` – fix a wheel that spins the wrong way
- `USE_PWM_SPEED` / `MOTOR_SPEED` – optional PWM speed control
- `LOOP_INTERVAL_MS`, servo timing, and serial options
