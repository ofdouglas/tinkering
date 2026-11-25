
///////////////////////////////////////////////////////////////////////////////
// External Interrupt Assignments
///////////////////////////////////////////////////////////////////////////////

typedef enum logic [4:0] {
    EXT_IRQ_UART0 = 5'd16,
    EXT_IRQ_RESERVED1 = 5'd17,
    EXT_IRQ_RESERVED2 = 5'd18,
    EXT_IRQ_RESERVED3 = 5'd19,
    EXT_IRQ_RESERVED4 = 5'd20,
    EXT_IRQ_RESERVED5 = 5'd21,
    EXT_IRQ_RESERVED6 = 5'd22,
    EXT_IRQ_RESERVED7 = 5'd23,
    EXT_IRQ_RESERVED8 = 5'd24,
    EXT_IRQ_RESERVED9 = 5'd25,
    EXT_IRQ_RESERVED10 = 5'd26,
    EXT_IRQ_RESERVED11 = 5'd27,
    EXT_IRQ_RESERVED12 = 5'd28,
    EXT_IRQ_RESERVED13 = 5'd29,
    EXT_IRQ_RESERVED14 = 5'd30,
    EXT_IRQ_RESERVED15 = 5'd31,
} external_irq_e;