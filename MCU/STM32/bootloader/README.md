# STM32F7 UART Bootloader

Target HW: STM32F746G-DISCO dev kit (STM32F746NGHx).

## Prerequisites

- `make`
- `arm-none-eabi-gcc` and `arm-none-eabi-g++` on PATH (WSL: `sudo apt install make gcc-arm-none-eabi`)
- Shared makefile fragments in `../mk/`
- Shared firmware sources in `../../../Firmware/` (logging, crc, hdlc, data_structures)

## Build

From WSL or Linux:

```bash
cd /mnt/c/Design/MCU/STM32/bootloader
make clean
make
arm-none-eabi-size build/bootloader.elf
```

Outputs land in `build/bootloader.elf` and `build/bootloader.map`.

Optional targets from the shared `stm32f7.mk` rules:

```bash
make flash   # OpenOCD + ST-Link
make reset
```

## Memory map

| Region | Origin     | Size  | Use                          |
|--------|------------|-------|------------------------------|
| FLASH  | 0x08000000 | 64 KB | Bootloader code and constants |
| RAM    | 0x20000000 | 320 KB| Data, heap (512 B), stack (2 KB) |

The application image is expected to live above the bootloader region (starting at 0x08010000). Image metadata is defined in `Firmware/bootloader/image_header.h`.

## Runtime behavior

On reset the bootloader:

1. Initializes HAL, clocks, the on-board LED (GPIOI3), and USART1 (ST-Link VCP, 115200 baud).
2. Logs a boot message referencing `Bootloader::ImageHeader::kMagic`.
3. Enters a superloop that toggles the LED and echoes UART bytes (1-byte receive with 10 ms timeout).

## Related projects

- `../motor-controller/` — full application firmware (FreeRTOS, larger flash layout)
- `../../../Firmware/` — shared C++ libraries used by bootloader and application
