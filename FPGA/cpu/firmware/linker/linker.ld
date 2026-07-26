ENTRY(_start)
MEMORY {
    BOOT_ROM (rx)   : ORIGIN = 0x00010000, LENGTH = 8K
    SHARED_RAM (rwx) : ORIGIN = 0x10000000, LENGTH = 8K

   /* CPU0_PSRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
    CPU0_DSRAM (rwwx) : ORIGIN = 0x21000000, LENGTH = 32K */
}

STACK_SIZE = 512 * 4;
HEAP_SIZE  = 512 * 1;

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

    } > SHARED_RAM

    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data .data.* .sdata .sdata.*)
        . = ALIGN(4);
        _data_end = .;
    } > SHARED_RAM AT> BOOT_ROM

    _copy_data = LOADADDR(.data);

    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;
        . = . + HEAP_SIZE;
        _heap_end = .;
    } > SHARED_RAM

    .stack :
    {
        _stack_top = ORIGIN(SHARED_RAM) + LENGTH(SHARED_RAM);
        _stack_bottom = _stack_top - STACK_SIZE;
        . = _stack_bottom;
        . = . + STACK_SIZE;
    } > SHARED_RAM
}