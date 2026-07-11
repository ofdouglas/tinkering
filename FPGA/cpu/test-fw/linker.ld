ENTRY(_start)
MEMORY {
    TEST_ROM (rx)  : ORIGIN = 0x00010000, LENGTH = 4K
    TEST_RAM (rw)  : ORIGIN = 0x10000000, LENGTH = 4K
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
    } > TEST_ROM

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(.scommon)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > TEST_RAM

    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data*)
        *(.sdata*)
        . = ALIGN(4);
        _data_end = .;
    } > TEST_RAM AT> TEST_ROM

    _copy_data = LOADADDR(.data);

    _test_rom_start = ORIGIN(TEST_ROM);
    _test_ram_start = ORIGIN(TEST_RAM);
    _sram_start = _test_ram_start;
    _stack_top = ORIGIN(TEST_RAM) + LENGTH(TEST_RAM);
}
