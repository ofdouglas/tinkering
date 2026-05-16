#!/usr/bin/env python3
"""Brief read from ST-Link VCP to check if board is sending."""
import sys
import serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

try:
    ser = serial.Serial(port, baud, timeout=0.5)
    ser.reset_input_buffer()
    print(f"Opened {port} @ {baud}")
    data = b""
    for _ in range(8):
        chunk = ser.read(512)
        if chunk:
            data += chunk
            print(f"  +{len(chunk)} bytes")
    ser.close()
    if data:
        print("--- received ---")
        print(data.decode("utf-8", errors="replace"), end="")
        print("---")
        print(f"Total: {len(data)} bytes — board appears alive on serial")
        sys.exit(0)
    print("No data in ~4s")
    print("Note: firmware uses USART1; ST-Link VCP on F746-Disco is usually USART3")
    sys.exit(1)
except serial.SerialException as e:
    print(f"Serial error: {e}")
    sys.exit(2)
