# Telemetry IoT Engine

Edge firmware for an Arduino that watches its environment, classifies the situation as **NORMAL**, **WARNING** or **CRITICAL**, reacts on its own, and streams every reading over serial for logging.

Built as a model of condition monitoring on heavy equipment such as a mining drill: the thresholds simulate the limits a real machine would have.

## What it does

Every 500 ms, without blocking the main loop (`millis()` timing, no `delay()`):

1. **Samples three sensors:** temperature and humidity (DHT22), displacement in mm (HC-SR04 style ultrasonic sensor), and light level (LDR on A0).
2. **Checks for sensor faults:** a failed DHT read shows `SENSOR FAULT!` and skips the cycle instead of acting on bad data.
3. **Classifies the state** with threshold rules:

   | State | Any of |
   |---|---|
   | CRITICAL | temperature > 60 °C, displacement > 150 mm, light > 800 |
   | WARNING | temperature > 40 °C, displacement > 80 mm, light > 500 |
   | NORMAL | otherwise |

4. **Reacts locally:** green, yellow or red LED, a buzzer tone that escalates with severity, and a status message on a 16x2 I2C LCD.
5. **Streams telemetry** as CSV over serial at 115200 baud:

   ```
   DATA,<temperature_C>,<humidity_pct>,<displacement_mm>,<light_raw>,<STATE>
   ```

## Hardware

| Part | Pin |
|---|---|
| DHT22 | D2 |
| Ultrasonic trigger / echo | D3 / D4 |
| Green, yellow, red LEDs | D5, D6, D7 |
| Buzzer | D8 |
| LDR (voltage divider) | A0 |
| 16x2 LCD, I2C backpack (0x27) | SDA / SCL |

## Run it

1. Install the `DHT sensor library` and `LiquidCrystal I2C` libraries in the Arduino IDE.
2. Open `firmware/firmware.ino`, select your board and port, and upload.
3. Open the Serial Monitor at 115200 baud to see the `DATA,...` stream.

## Log the telemetry to CSV

`scripts/logger.py` reads the serial stream, keeps only valid `DATA` packets, timestamps each one in UTC and appends it to a CSV file. CSV files are git-ignored, so logs stay on your machine.

```bash
pip install -r scripts/requirements.txt
python scripts/logger.py --port COM3              # Windows
python scripts/logger.py --port /dev/ttyACM0      # Linux
```

Options: `--baud` (default 115200, matching the firmware) and `--out` (default `telemetry.csv`).

Close the Arduino Serial Monitor first: only one program can hold the port.

## Design choices

- **State machine over ad hoc `if` blocks**, so each state has one place that defines its outputs.
- **All outputs reset before each state is applied**, so a change of state never leaves a stale LED or tone on.
- **Fail safe on sensor faults**: no classification from invalid readings.
