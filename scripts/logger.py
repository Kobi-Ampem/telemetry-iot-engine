"""Serial telemetry logger for the edge firmware.

Reads the `DATA,<temperature>,<humidity>,<displacement_mm>,<light>,<STATE>` lines
that firmware/firmware.ino prints every 500 ms, timestamps each one, and appends
it to a CSV file. Any other serial output, such as boot noise, is skipped.

Usage:
    python scripts/logger.py --port COM3
    python scripts/logger.py --port /dev/ttyACM0 --out telemetry.csv
"""

import argparse
import csv
import sys
from datetime import datetime, timezone
from pathlib import Path

import serial

FIELDS = ["timestamp_utc", "temperature_c", "humidity_pct", "displacement_mm", "light_raw", "state"]
STATES = {"NORMAL", "WARNING", "CRITICAL"}


def parse_line(line: str) -> dict | None:
    """Turn one serial line into a record, or None if it is not a valid DATA packet."""
    parts = line.strip().split(",")
    if len(parts) != 6 or parts[0] != "DATA" or parts[5] not in STATES:
        return None
    try:
        temperature, humidity, displacement = (float(p) for p in parts[1:4])
        light = int(parts[4])
    except ValueError:
        return None
    return {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(timespec="milliseconds"),
        "temperature_c": temperature,
        "humidity_pct": humidity,
        "displacement_mm": displacement,
        "light_raw": light,
        "state": parts[5],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Log edge telemetry from serial to CSV.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM3 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200, help="Must match Serial.begin() in the firmware")
    parser.add_argument("--out", type=Path, default=Path("telemetry.csv"), help="CSV file to append to")
    args = parser.parse_args()

    new_file = not args.out.exists()
    with serial.Serial(args.port, args.baud, timeout=2) as port, args.out.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDS)
        if new_file:
            writer.writeheader()
        print(f"Logging {args.port} at {args.baud} baud to {args.out}. Ctrl+C to stop.")
        try:
            while True:
                raw = port.readline().decode("utf-8", errors="replace")
                record = parse_line(raw)
                if record is None:
                    continue
                writer.writerow(record)
                f.flush()
                print(f"{record['timestamp_utc']}  {record['state']:<8}  "
                      f"T={record['temperature_c']}C  D={record['displacement_mm']}mm  L={record['light_raw']}")
        except KeyboardInterrupt:
            print("\nStopped.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
