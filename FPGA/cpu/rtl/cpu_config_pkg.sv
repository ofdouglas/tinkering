package cpu_config_pkg;

    // Bus geometry
    localparam int DATA_BYTES       = 4;
    // localparam int MEMORY_ADDR_MSB  = 15;   // [15:2] ->  64 KiB 
    localparam int MEMORY_ADDR_MSB  = 11;   // [11:2] ->  4 KiB 
    localparam int PERIPH_ADDR_MSB  = 7;    // [7:2]  -> 256 B

    // Memory regions (64 KiB each)
    localparam logic [31:0] ROM_BASE  = 32'h0000_0000;
    localparam logic [31:0] RAM_BASE  = 32'h0001_0000;
    localparam logic [31:0] LED_BASE  = 32'h0002_0000;
    localparam logic [31:0] UART_BASE = 32'h0003_0000;

    // Region decode (address bits [19:16])
    localparam logic [3:0] REGION_ROM  = 4'h0;
    localparam logic [3:0] REGION_RAM  = 4'h1;
    localparam logic [3:0] REGION_LED  = 4'h2;
    localparam logic [3:0] REGION_UART = 4'h3;

endpackage