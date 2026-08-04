# Bootloader Working Doc

Bootloader Protocol Use Cases:

General:
- Read bootloader version
- Read application version && validity check
- Read hardware information


Application:
- Download app into internal MCU flash
- Download app into external flash (FPGA)
- Download app into SRAM (MCU or FPGA)
- Boot app from internal MCU flash
- Boot app from external flash (FPGA)
  (running in SRAM)


Bootloader Upgrade:
- TBD


Data File
- Upload data file to host computer (any memory region)

*/


/*  Steps to downloading an image:
     1. Detect compatible hardware / bootloader
     2. Check app version / validity to see if it needs updating
     3. (Flash) Erase target flash sector(s)
     4. Configure transfer: start_address, size_bytes
     5. Transfer all segments (with ACK/retry?)
     6. Verify image

    Questions:
     - Is segment size fixed per PHY type?