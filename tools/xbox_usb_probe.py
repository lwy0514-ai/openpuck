#!/usr/bin/env python3
"""Summarize USB control SETUP requests captured as CSV.

Expected columns: bmRequestType,bRequest,wIndex,wLength
Optional columns: timestamp,wValue. Numbers may be decimal or 0x-prefixed.
This is a diagnostic tool; it does not implement or bypass Xbox authentication.

Usage:
  python tools/xbox_usb_probe.py capture.csv
"""
import argparse
import csv
from collections import Counter
from pathlib import Path

XSM3_COMMANDS = {
    0x81: "identify",
    0x82: "init challenge",
    0x83: "challenge response",
    0x84: "keepalive",
    0x86: "status",
    0x87: "verify challenge",
}


def number(value):
    value = str(value).strip()
    return int(value, 0) if value.lower().startswith("0x") else int(value, 10)


def analyze(path):
    counts = Counter()
    unexpected = Counter()
    with Path(path).open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        required = {"bmRequestType", "bRequest", "wIndex", "wLength"}
        missing = required - set(reader.fieldnames or [])
        if missing:
            raise ValueError("Missing CSV columns: " + ", ".join(sorted(missing)))
        for row_number, row in enumerate(reader, 2):
            try:
                direction = number(row["bmRequestType"])
                command = number(row["bRequest"])
                index = number(row["wIndex"])
                length = number(row["wLength"])
            except (ValueError, TypeError) as error:
                raise ValueError(f"Invalid numeric field at CSV row {row_number}") from error
            if min(direction, command, index, length) < 0:
                raise ValueError(f"Negative field at CSV row {row_number}")
            key = (direction, command, index, length)
            counts[key] += 1
            recipient = direction & 0x7f
            known_recipient = (recipient == 0x41 and index in (3, 0x103)) or (
                recipient == 0x40 and index == 0
            )
            if not known_recipient or command not in XSM3_COMMANDS:
                unexpected[key] += 1
    return counts, unexpected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", help="CSV file containing USB SETUP packets")
    args = parser.parse_args()
    counts, unexpected = analyze(args.capture)
    print(f"Total SETUP requests: {sum(counts.values())}")
    print("Count  bmRequestType  bRequest  wIndex  wLength  XSM3 interpretation")
    for (direction, command, index, length), count in sorted(counts.items()):
        name = XSM3_COMMANDS.get(command, "unknown")
        print(f"{count:5d}  0x{direction:02X}           0x{command:02X}      "
              f"0x{index:04X}  {length:7d}  {name}")
    print(f"Outside known Xbox 360 XSM3 request shape: {sum(unexpected.values())}")
    print("NOTE: Request shapes alone cannot prove Xbox Series X authentication.")


if __name__ == "__main__":
    main()
