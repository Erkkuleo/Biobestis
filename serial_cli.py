#!/usr/bin/env python3
import serial
import serial.tools.list_ports
import threading
import sys

BAUD = 115200


def find_port():
    if len(sys.argv) > 1:
        return sys.argv[1]
    # Auto-detect: pick the first port that looks like a USB serial device
    candidates = [
        p.device for p in serial.tools.list_ports.comports()
        if any(kw in (p.description or "").lower() for kw in ("usb", "uart", "acm", "xiao"))
           or any(kw in (p.device or "").lower() for kw in ("acm", "usbmodem", "com"))
    ]
    if candidates:
        return candidates[0]
    # Fallback
    import platform
    if platform.system() == "Windows":
        return "COM3"
    elif platform.system() == "Darwin":
        return "/dev/tty.usbmodem101"
    return "/dev/ttyACM0"


PORT = find_port()
COMMANDS = [
    "h <0-100>",
    "f <0-100>",
    "d",
    "d <number>",
    "s",
]

def read_serial(ser):
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if line:
                print(f"\r{line}\n> ", end="", flush=True)
        except Exception:
            break

def main():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=1)
    except serial.SerialException as e:
        print(f"Error opening {PORT}: {e}")
        sys.exit(1)

    print(f"Connected to {PORT} at {BAUD} baud")
    print("Commands:", ", ".join(COMMANDS))
    print("Type 'quit' to exit\n")

    t = threading.Thread(target=read_serial, args=(ser,), daemon=True)
    t.start()

    while True:
        try:
            cmd = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting.")
            break

        if cmd == "quit":
            break
        if not cmd:
            continue

        lower = cmd.lower()
        parts = lower.split()
        valid = False

        if lower in ("s", "status"):
            cmd = "status"
            valid = True
        elif len(parts) == 2 and parts[0] in ("h", "humidity"):
            try:
                val = int(parts[1])
                if 0 <= val <= 100:
                    cmd = f"humidity {val}"
                    valid = True
            except ValueError:
                pass
        elif len(parts) == 2 and parts[0] in ("f", "fullness"):
            try:
                val = int(parts[1])
                if 0 <= val <= 100:
                    cmd = f"fullness {val}"
                    valid = True
            except ValueError:
                pass
        elif lower in ("d", "day"):
            cmd = "day"
            valid = True
        elif len(parts) == 2 and parts[0] in ("d", "day"):
            try:
                int(parts[1])
                cmd = f"day {parts[1]}"
                valid = True
            except ValueError:
                pass
        if valid:
            ser.write((cmd + "\n").encode())
        else:
            print(f"Error: unknown command '{cmd}'")
            print("Valid commands:", ", ".join(COMMANDS))

    ser.close()

if __name__ == "__main__":
    main()
