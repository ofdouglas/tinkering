# Primitive text-based bootloader

## Bootloader -> Host Format

BL_READY  -- Periodically sent until a download start is detected
DL_START  -- Sent when the download start is detected
DL_ERROR  -- Download failed
CRC_ERROR -- Image failed verification
BOOT_APP  -- Bootloader is jumping to app

## Host -> Bootloader Format

BOOT_IMG img[0] img[1] img[2] ... img[N-1] $ crc[0] crc[1] crc[2] crc[3] ;

Example: "BOOT_IMG 7F FF FF FF 00 00 00 01 DE AD BE EF $ 0A FE 17 75 ;"
