// Simple bus slave with 1-cycle read/write latency
interface bus_slave_interface #(
    parameter int ADDR_MSB = 23
);
    // System
    logic                clk;
    logic                rst_n;

    // Bus Master Request
    logic                valid;
    logic [ 3 : 0]       wr_strobe;
    logic [31 : 0]       wr_data;
    logic [ADDR_MSB : 2] addr;

    // Bus Slave Response
    logic                ready;
    logic                rd_valid;
    logic [31 : 0]       rd_data;
    logic                wr_ack;
    logic                error;

    modport master (
        output valid, wr_strobe, wr_data, addr,
        input ready, rd_data, rd_valid, error
    );

    modport slave (
        output ready, rd_data, rd_valid, wr_ack, error,
        input clk, rst_n, valid, wr_strobe, wr_data, addr
    );
endinterface
