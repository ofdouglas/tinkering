#!/usr/bin/env python3
"""UART image downloader for the ROM bootloader.

Board:
    python3 downloader.py app.bin /dev/ttyUSB0

Host loopback (spawns the host-built bootloader on stdin/stdout):
    python3 downloader.py --host-test app.bin --bootloader build/host/bootloader
"""

from __future__ import annotations

import argparse
import os
import select
import subprocess
import sys
import time
import zlib
from enum import Enum
from typing import BinaryIO, Optional, Protocol


BOARD_BAUD = 115200
BOOT_IMG_RESEND_S = 0.25


class DownloadState(Enum):
    WAIT_FOR_BOOTLOADER = 1
    IMAGE_TRANSFER = 2
    WAIT_FOR_STATUS = 3
    SUCCESS = 4
    ERROR = 5


class LineSerial(Protocol):
    def readline(self) -> bytes:
        ...

    def write(self, data: bytes) -> int:
        ...

    def flush(self) -> None:
        ...


def encode_image_hex(image_bytes: bytes) -> bytes:
    return image_bytes.hex(" ").upper().encode("ascii")


def encode_crc_hex(crc: int) -> bytes:
    crc_bytes = (crc & 0xFFFFFFFF).to_bytes(4, "big")
    return crc_bytes.hex(" ").upper().encode("ascii")


def encode_payload(image_bytes: bytes, crc: int) -> bytes:
    return encode_image_hex(image_bytes) + b" $ " + encode_crc_hex(crc) + b" ;"


class PipeSerial:
    """Line-oriented serial stand-in over a pair of pipes."""

    def __init__(self, stdin: BinaryIO, stdout: BinaryIO, timeout: float = 1.0) -> None:
        self._stdin = stdin
        self._stdout = stdout
        self.timeout = timeout
        self._buf = bytearray()
        os.set_blocking(stdout.fileno(), False)

    def readline(self) -> bytes:
        deadline = time.monotonic() + self.timeout
        while True:
            newline_at = self._buf.find(b"\n")
            if newline_at >= 0:
                line = bytes(self._buf[: newline_at + 1])
                del self._buf[: newline_at + 1]
                return line

            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return b""

            ready, _, _ = select.select([self._stdout], [], [], remaining)
            if not ready:
                return b""
            try:
                chunk = os.read(self._stdout.fileno(), 1024)
            except BlockingIOError:
                continue
            if not chunk:
                return b""
            self._buf.extend(chunk)

    def write(self, data: bytes) -> int:
        if isinstance(data, str):
            data = data.encode("ascii")
        self._stdin.write(data)
        self._stdin.flush()
        return len(data)

    def flush(self) -> None:
        self._stdin.flush()


def runDownloader(
    image_bytes: bytes,
    crc: int,
    ser: LineSerial,
    timeout_s: float = 10.0,
) -> DownloadState:
    current_state = DownloadState.WAIT_FOR_BOOTLOADER
    deadline = time.monotonic() + timeout_s
    last_boot_img_s = 0.0

    while current_state not in [DownloadState.SUCCESS, DownloadState.ERROR]:
        if time.monotonic() > deadline:
            print("Timed out waiting for bootloader")
            return DownloadState.ERROR

        raw_line = ser.readline()
        if not raw_line:
            continue
        line_str = raw_line.decode("utf-8", errors="replace").strip()
        if not line_str:
            continue

        print(line_str)

        if current_state in (
            DownloadState.WAIT_FOR_BOOTLOADER,
            DownloadState.IMAGE_TRANSFER,
        ):
            if "BL_READY" in line_str:
                now = time.monotonic()
                if (last_boot_img_s == 0.0) or ((now - last_boot_img_s) >= BOOT_IMG_RESEND_S):
                    print("Sending BOOT_IMG")
                    ser.write(b"BOOT_IMG")
                    ser.flush()
                    last_boot_img_s = now
                current_state = DownloadState.IMAGE_TRANSFER

            if current_state is DownloadState.IMAGE_TRANSFER and "DL_START" in line_str:
                print("DL_START reported")
                current_state = DownloadState.WAIT_FOR_STATUS
                deadline = time.monotonic() + timeout_s
                ser.write(encode_payload(image_bytes, crc))
                ser.flush()

        elif current_state is DownloadState.WAIT_FOR_STATUS:
            if "DL_ERROR" in line_str:
                print("DL_ERROR reported")
                current_state = DownloadState.ERROR
            elif "CRC_ERROR" in line_str:
                print("CRC_ERROR reported")
                current_state = DownloadState.ERROR
            elif "BOOT_APP" in line_str:
                print("BOOT_APP reported")
                current_state = DownloadState.SUCCESS
            elif "BL_READY" in line_str:
                print("Bootloader returned to idle after payload")
                current_state = DownloadState.ERROR
            else:
                print(f"Unknown status: {line_str}")

    return current_state


def open_board_serial(tty: str) -> LineSerial:
    try:
        import serial
    except ImportError as exc:
        raise SystemExit(
            "pyserial is required for board downloads (pip install pyserial)"
        ) from exc

    ser = serial.Serial(
        port=tty,
        baudrate=BOARD_BAUD,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1,
        write_timeout=2,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False,
    )
    ser.dtr = True
    ser.rts = True
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    return ser


def run_host_test(image_bytes: bytes, crc: int, bootloader: str, image_out: Optional[str]) -> DownloadState:
    if not os.path.isfile(bootloader):
        raise SystemExit(f"host bootloader not found: {bootloader}")

    env = os.environ.copy()
    if image_out:
        env["HOST_BL_IMAGE_OUT"] = image_out

    proc = subprocess.Popen(
        [bootloader],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env,
        bufsize=0,
    )
    assert proc.stdin is not None
    assert proc.stdout is not None
    ser = PipeSerial(proc.stdin, proc.stdout, timeout=1.0)

    try:
        result = runDownloader(image_bytes, crc, ser)
        if result is DownloadState.SUCCESS:
            proc.wait(timeout=2)
        return result
    finally:
        if proc.poll() is None:
            proc.kill()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
        stderr = b""
        if proc.stderr is not None:
            stderr = proc.stderr.read()
            proc.stderr.close()
        if stderr:
            print("--- bootloader stderr ---")
            print(stderr.decode("utf-8", errors="replace"))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Download an image to the UART bootloader")
    parser.add_argument("image", help="Binary load image (objcopy -O binary)")
    parser.add_argument(
        "tty",
        nargs="?",
        help="Serial device for the target board, e.g. /dev/ttyUSB0",
    )
    parser.add_argument(
        "--host-test",
        action="store_true",
        help="Spawn the host-built bootloader and download over stdin/stdout",
    )
    parser.add_argument(
        "--bootloader",
        default=os.path.join(os.path.dirname(__file__), "build/host/bootloader"),
        help="Path to the host bootloader binary (host-test only)",
    )
    parser.add_argument(
        "--image-out",
        default=None,
        help="Host bootloader writes the received image here (host-test only)",
    )
    return parser.parse_args()


def board_timeout_s(image_bytes: bytes, crc: int) -> float:
    n_bytes = len(b"BOOT_IMG") + len(encode_payload(image_bytes, crc))
    wire_s = n_bytes * (10.0 / BOARD_BAUD)
    return max(30.0, wire_s + 10.0)


def main() -> None:
    args = parse_args()
    if (not args.host_test) and (args.tty is None):
        print("Usage: python3 downloader.py <image.bin> <tty>")
        print("   or: python3 downloader.py --host-test <image.bin>")
        sys.exit(2)

    with open(args.image, "rb") as image_file:
        firmware_data = image_file.read()

    if len(firmware_data) == 0:
        print("Image is empty")
        sys.exit(1)

    firmware_crc = zlib.crc32(firmware_data) & 0xFFFFFFFF
    print(f"Firmware length: {len(firmware_data)}")
    print(f"Firmware CRC: {firmware_crc:#x}")

    if args.host_test:
        image_out = args.image_out
        result = run_host_test(firmware_data, firmware_crc, args.bootloader, image_out)
        if result is DownloadState.SUCCESS and image_out:
            with open(image_out, "rb") as dumped:
                received = dumped.read()
            if received != firmware_data:
                print("Host bootloader image dump did not match the sent bytes")
                sys.exit(1)
            print(f"Host image dump matched ({len(received)} bytes)")
    else:
        with open_board_serial(args.tty) as ser:
            result = runDownloader(
                firmware_data,
                firmware_crc,
                ser,
                timeout_s=board_timeout_s(firmware_data, firmware_crc),
            )

    if result is DownloadState.SUCCESS:
        print("Download successful")
        sys.exit(0)
    if result is DownloadState.ERROR:
        print("Download failed")
        sys.exit(1)
    print(f"Unknown result: {result}")
    sys.exit(1)


if __name__ == "__main__":
    main()
