ENTRY(_start)
MEMORY {
    SRAM_DATA  (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
    SRAM_TEXT  (rx)  : ORIGIN = 0x10008000, LENGTH = 32K
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
     * Text sections 
     *************************************************************************/
    .text_section : 
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
    } > SRAM_TEXT


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
    } > SRAM_DATA AT> SRAM_TEXT

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