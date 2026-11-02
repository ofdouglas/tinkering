# Primitive text-based bootloader

ROM bootloader. The host sends an app image over UART; the bootloader writes it
to SRAM_TEXT (`0x10008000`) and jumps there.

## Image format

Send the app **load image**, not an ELF:

    riscv64-unknown-elf-objcopy -O binary app.elf app.bin

`app.ld` places `.text` / `.rodata` in SRAM_TEXT and keeps `.data`'s load
address there (`AT> SRAM_TEXT`). The binary must include those load bytes so
the app's `_start` can copy `.data` into SRAM_DATA. `.bss` / stack / heap are
not part of the download.

CRC-32 is IEEE (poly `0xEDB88320`, init `0xFFFFFFFF`, final XOR
`0xFFFFFFFF`). The four CRC bytes on the wire are big-endian.

## Bootloader -> Host

Each status is one line terminated by `\n`.

| Line      | When |
|-----------|------|
| `BL_READY`  | Idle, about every 100 ms, until `BOOT_IMG` is seen. Wait for this before sending `BOOT_IMG`. |
| `DL_START`  | `BOOT_IMG` matched. **Wait for this before the payload** (UART RX is one byte). |
| `DL_ERROR`  | Image or CRC framing failed (bad hex, overflow, empty image, incomplete byte). Returns to idle. |
| `CRC_ERROR` | Image CRC mismatch. Returns to idle. |
| `BOOT_APP`  | CRC matched; jumping to SRAM_TEXT. |

## Host -> Bootloader

```
BOOT_IMG <hex-bytes> $ <crc0> <crc1> <crc2> <crc3> ;
```

Hex bytes are ASCII `0-9A-Fa-f`. Spaces, tabs, CR, and LF are allowed between
complete bytes, not between the two nibbles of a byte.

Example:

```
BOOT_IMG 7F FF FF FF 00 00 00 01 DE AD BE EF $ 0A FE 17 75 ;
```
