#!/usr/bin/env bash
# ST-Link VCP in WSL (after usbipd attach). Default: /dev/ttyACM0 @ 115200
DEV="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"
exec python3 -m serial.tools.miniterm "$DEV" "$BAUD"
