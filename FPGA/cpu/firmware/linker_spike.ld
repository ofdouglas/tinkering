/* Spike places its debug module at 0x0; keep firmware ROM above that page. */
ENTRY(_start)
MEMORY {
    ROM (rx) : ORIGIN = 0x00001000, LENGTH = 4K
    RAM (rw) : ORIGIN = 0x00010000, LENGTH = 4K
    SYS (rw) : ORIGIN = 0x00020000, LENGTH = 4K
}
SECTIONS {
    .text : { *(.text*) } > ROM
    .data : { *(.data*) } > RAM
    .bss  : { *(.bss*) *(COMMON) } > RAM
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
