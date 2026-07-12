package cpu_config_pkg;

    // Bus geometry
    localparam int DATA_BYTES       = 4;
    localparam int REGION_BITS_MSB  = 31;
    localparam int REGION_BITS_LSB  = 28;

    // localparam int MEMORY_ADDR_MSB  = 17;   // [17:2] ->  256 KiB 
    localparam int MEMORY_ADDR_MSB  = 11;   // [11:2] ->  4 KiB 
    localparam int PERIPH_ADDR_MSB  = 7;    // [7:2]  -> 256 B

    // Memory regions (64 KiB each)
    localparam logic [31:0] BOOT_ROM_BASE   = 32'h0000_0000;
    localparam logic [31:0] SHARED_RAM_BASE = 32'h1000_0000;
    localparam logic [31:0] CPU0_PSR_BASE   = 32'h2000_0000;
    localparam logic [31:0] CPU0_DSR_BASE   = 32'h2100_0000;
    localparam logic [31:0] RESERVED_3      = 32'h3000_0000;
    localparam logic [31:0] RESERVED_4      = 32'h4000_0000;
    localparam logic [31:0] RESERVED_5      = 32'h5000_0000;
    localparam logic [31:0] RESERVED_6      = 32'h6000_0000;
    localparam logic [31:0] RESERVED_7      = 32'h7000_0000;
    localparam logic [31:0] PERIPH_BASE     = 32'h8000_0000;

    // Region decode (address bits [31:28])
    localparam logic [3:0] REGION_BOOT_ROM    = 4'h0;
    localparam logic [3:0] REGION_SHARED_RAM  = 4'h1;
    localparam logic [3:0] REGION_CPU0_DSRAM  = 4'h2;
    localparam logic [3:0] REGION_RESERVED_3  = 4'h3;
    localparam logic [3:0] REGION_RESERVED_4  = 4'h4;
    localparam logic [3:0] REGION_RESERVED_5  = 4'h5;
    localparam logic [3:0] REGION_RESERVED_6  = 4'h6;
    localparam logic [3:0] REGION_RESERVED_7  = 4'h7;
    localparam logic [3:0] REGION_PERIPH      = 4'h8;

endpackage