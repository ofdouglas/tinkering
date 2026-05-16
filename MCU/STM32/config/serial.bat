REM Windows ST-Link VCP (when USB is NOT forwarded to WSL). Find port in Device Manager.
python -m serial.tools.miniterm COM6 115200
REM WSL (usbipd):  python3 -m serial.tools.miniterm /dev/ttyACM0 115200
REM Quick check:   python3 read_serial.py /dev/ttyACM0