import serial
import hdlc
import time

ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
data = b'\xDE\xAD\xBE\xEF'
msg = hdlc.hdlc_encode(data, True)

print(msg)

while True:
    for b in msg:
        ser.write(b)
        time.sleep(0.01)

    line_bytes = ser.readline()

    # Convert raw binary bytes into a readable Python text string
    line_text = line_bytes.decode('utf-8').strip()
    print(f"Received Text: {line_text}")

ser.close()