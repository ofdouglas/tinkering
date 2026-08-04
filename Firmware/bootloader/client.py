import serial
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "hdlc"))
import hdlc as hdlc
# import protocol as protocol

ser = serial.Serial(port='/dev/ttyACM0', baudrate=115200, timeout=1)

def send_command(command: int, argument: int = 0) -> bool:
    payload = b'\xC3' + bytes([command, argument])
    frame = hdlc.hdlc_encode(payload, trailing=True)
    ser.write(frame)


if __name__ == "__main__":
    send_command(0x01, 0)
    response = ser.read(1024)
    print(response)