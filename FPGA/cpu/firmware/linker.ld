ENTRY(_start)
MEMORY {
    ROM (rx) : ORIGIN = 0x00000000, LENGTH = 4K
    RAM (rw) : ORIGIN = 0x00010000, LENGTH = 4K
    SYS (rw) : ORIGIN = 0x00020000, LENGTH = 4K
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

    } > ROM
    
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

    } > RAM

    .data : 
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data*)
        *(.sdata*)
        . = ALIGN(4);
        _data_end = .;

    } > RAM AT> ROM

    _copy_data = LOADADDR(.data);

    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}