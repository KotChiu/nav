#!/usr/bin/env python3
"""MotoNav 2.1 on-device QA log reader. Never flashes or resets the board.

pip install pyserial
python3 tools/hardware-qa/collect.py --port /dev/cu.usbmodemXXXX
python3 tools/hardware-qa/collect.py --port /dev/cu.usbmodemXXXX --run

The QA firmware automatically runs after power-up; QA? retrieves the most
recent report from NVS even if USB serial wasn't connected at that moment.
"""
import argparse
import json
import pathlib
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="Serial device, e.g. /dev/cu.usbmodemXXX")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--run", action="store_true",
                        help="Re-run diagnostics (refused if connected/navigating)")
    parser.add_argument("--timeout", type=float, default=30)
    parser.add_argument("--output", type=pathlib.Path,
                        help="Save report JSON (default: motonav-qa-report.json)")
    args = parser.parse_args()
    try:
        import serial
        import serial.tools.list_ports
    except ImportError:
        print("Install dependency: python3 -m pip install pyserial", file=sys.stderr)
        return 2

    if args.port:
        port = args.port
    else:
        ports = list(serial.tools.list_ports.comports())
        probable = [p for p in ports if "usb" in p.device.lower() or
                    "jtag" in p.description.lower() or "cp210" in p.description.lower()]
        if len(probable) != 1:
            print("Specify --port. Ports found:", file=sys.stderr)
            for p in ports:
                print(f"  {p.device}  {p.description}", file=sys.stderr)
            return 2
        port = probable[0].device

    outfile = args.output or pathlib.Path("motonav-qa-report.json")
    print(f"Opening {port} at {args.baud} baud. Do not disconnect during the test.")
    with serial.Serial(port, args.baud, timeout=0.3, write_timeout=2) as conn:
        # ESP32-S3 USB CDC may take a moment to enumerate after power-on.
        time.sleep(0.5)
        conn.reset_input_buffer()
        command = b"QARUN\n" if args.run else b"QAREPORT\n"
        conn.write(command)
        deadline = time.monotonic() + args.timeout
        report = None
        while time.monotonic() < deadline:
            raw = conn.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", "replace").strip()
            print(line)
            if line.startswith("QA_REJECT,"):
                print("Device is busy: disconnect navigation/phone and retry.",
                      file=sys.stderr)
                return 3
            if line.startswith("QA_REPORT,"):
                try:
                    report = json.loads(line[len("QA_REPORT,"):])
                except json.JSONDecodeError:
                    print("Invalid report JSON; check USB line quality.", file=sys.stderr)
                    continue
                if not args.run:
                    break
            if args.run and line.startswith("QA_END,") and report:
                break
        if report is None:
            print("Timed out waiting for QA_REPORT. Did you flash the *QA* image?",
                  file=sys.stderr)
            return 4

        outfile.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n",
                           encoding="utf-8")
        print(f"Saved {outfile}")
        print("Result: PASS" if report.get("fail") == 0 else
              f"Result: {report.get('fail', '?')} automatic checks failed")
        if not report.get("optical_verified"):
            print("Note: screen colors, rotation, and touch alignment still need visual review.")
        return 0 if report.get("fail") == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
