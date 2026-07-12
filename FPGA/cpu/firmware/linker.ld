ENTRY(_start)
MEMORY {
    BOOT_ROM (rx)   : ORIGIN = 0x00010000, LENGTH = 4K
    SHARED_RAM (rx) : ORIGIN = 0x10000000, LENGTH = 4K

    CPU0_PSRAM (rw) : ORIGIN = 0x20000000, LENGTH = 64K
    CPU0_DSRAM (rw) : ORIGIN = 0x21000000, LENGTH = 32K
}
SECTIONS {
    .text : 
    { 
        . = ALIGN(4);
        *(.text*)
        . = ALIGN(4);
        *(.rodata*)
        *(.srodata*)
        . = ALIGN(4);

    } > BOOT_ROM
    
    .bss  : 
    { 
        . = ALIGN(4);
        _bss_start = .;
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
        *(.data*)
        *(.sdata*)
        . = ALIGN(4);
        _data_end = .;

    } > SHARED_RAM AT> BOOT_ROM

    _copy_data = LOADADDR(.data);

    _sram_start = ORIGIN(SHARED_RAM);
    _stack_top = ORIGIN(SHARED_RAM) + LENGTH(SHARED_RAM);
}