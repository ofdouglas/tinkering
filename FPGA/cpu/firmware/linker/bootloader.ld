ENTRY(_start)
MEMORY {
    BOOT_ROM   (rx)  : ORIGIN = 0x00010000, LENGTH = 8K
    SRAM_DATA  (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
    SRAM_TEXT  (rx)  : ORIGIN = 0x10008000, LENGTH = 32K
}

STACK_SIZE = 512 * 4;
HEAP_SIZE  = 512 * 1;

_SRAM_TEXT_START = ORIGIN(SRAM_TEXT);
_SRAM_TEXT_END = ORIGIN(SRAM_TEXT) + LENGTH(SRAM_TEXT);
_SRAM_TEXT_LENGTH = LENGTH(SRAM_TEXT);

_SRAM_DATA_START = ORIGIN(SRAM_DATA);
_SRAM_DATA_END = ORIGIN(SRAM_DATA) + LENGTH(SRAM_DATA);
_SRAM_DATA_LENGTH = LENGTH(SRAM_DATA);

_BOOT_ROM_START = ORIGIN(BOOT_ROM);
_BOOT_ROM_END = ORIGIN(BOOT_ROM) + LENGTH(BOOT_ROM);
_BOOT_ROM_LENGTH = LENGTH(BOOT_ROM);

SECTIONS {
    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.eh_frame_hdr*)
    }

    /**************************************************************************
     * ROM sections 
     *************************************************************************/
    .boot_rom_section : 
    { 
        . = ALIGN(4);
        *(.startup_text*)

        . = ALIGN(4);
        *(.text*)

        . = ALIGN(4);
        *(.rodata*)
        *(.srodata*)

        . = ALIGN(4);
        _init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array .init_array.*))
        _init_array_end = .;
    } > BOOT_ROM


    /**************************************************************************
     * RAM sections 
     *************************************************************************/
    .bss  (NOLOAD) :
    { 
        . = ALIGN(4);
        _bss_start = .;
        *(.log_buffer)
        *(.bss*)
        *(.sbss*)
        *(.scommon)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;

    } > SRAM_DATA

    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(4);
        _data_end = .;
    } > SRAM_DATA AT> BOOT_ROM

    _copy_data = LOADADDR(.data);

    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
        . = . + HEAP_SIZE;
        _heap_end = .;
    } > SRAM_DATA

    .stack :
    {
        _stack_top = ORIGIN(SRAM_DATA) + LENGTH(SRAM_DATA);
        _stack_bottom = _stack_top - STACK_SIZE;
        . = _stack_bottom;
        . = . + STACK_SIZE;
    } > SRAM_DATA
}