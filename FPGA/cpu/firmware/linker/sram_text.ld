ENTRY(_start)
MEMORY {
    SRAM_TEXT (rx) : ORIGIN = 0x10008000, LENGTH = 32K
}

SECTIONS {
    /DISCARD/ :
    {
        *(.eh_frame*)
        *(.eh_frame_hdr*)
        *(.comment)
    }

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.text.*)
        . = ALIGN(4);
    } > SRAM_TEXT
}
