#!/usr/bin/env python3
"""Send a single line to a Duck's USB serial port and print whatever comes
back for a few seconds afterward.

Useful when the PlatformIO serial monitor's interactive typing isn't
working (e.g. focus issues, or a board that keeps resetting), or when you
just want a quick scripted way to fire off a CDK: command line.

Usage:
    python3 tools/send_serial_cmd.py /dev/ttyACM0 "CDK:RADIOREGION,VALUE:MY"
    python3 tools/send_serial_cmd.py /dev/ttyACM0 "CDK:RADIOREGION" --baud 115200 --listen 5

Requires pyserial: pip install pyserial
"""
import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("pyserial is required: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="Serial port, e.g. /dev/ttyACM0")
    parser.add_argument("line", help="Line to send (a trailing \\n is added automatically)")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--listen", type=float, default=8.0,
                         help="Seconds to keep reading output after sending (default: 8)")
    parser.add_argument("--boot-wait", type=float, default=6.0,
                         help="Seconds to wait after opening the port (letting an "
                              "ESP32/nRF52 finish rebooting from the DTR/RTS toggle "
                              "that opening the port usually causes) before sending "
                              "the command. Boot output is printed as it arrives. "
                              "(default: 6)")
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser:
        # Opening the port commonly asserts DTR/RTS, which on ESP32/nRF52 boards
        # triggers a hardware reset. Sending immediately (as this script used to)
        # can land the command mid-boot, before Serial/BLE/setup() are ready, and
        # it gets silently dropped with no error. Wait here and echo boot output
        # so it's obvious once the board is idle and ready.
        print(f"--> waiting {args.boot_wait}s for board to finish (re)booting...", file=sys.stderr)
        deadline = time.time() + args.boot_wait
        while time.time() < deadline:
            if ser.in_waiting:
                sys.stdout.write(ser.read(ser.in_waiting).decode(errors="replace"))
            else:
                time.sleep(0.05)

        payload = (args.line + "\n").encode()
        ser.write(payload)
        ser.flush()
        print(f"--> sent: {args.line!r}", file=sys.stderr)

        deadline = time.time() + args.listen
        while time.time() < deadline:
            if ser.in_waiting:
                sys.stdout.write(ser.read(ser.in_waiting).decode(errors="replace"))
            else:
                time.sleep(0.05)


if __name__ == "__main__":
    main()
