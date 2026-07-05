// Sim-only wrapper: adds configurable read/write response latency on top of
// block_sram's fixed 1-cycle access time.
//
// latency = 1 (default): request accepted on cycle T, rd_valid/wr_ack on T+1.
// latency = N:           request accepted on cycle T, rd_valid/wr_ack on T+N.

module slow_memory_adapter #(
    parameter int ADDR_MSB = 10,
    parameter int MAX_LATENCY = 256
) (
    input  logic                clk,
    input  logic                rst_n,
    input  int unsigned         latency,

    input  logic                cpu_valid,
    input  logic [3:0]          cpu_wr_strobe,
    input  logic [31:0]         cpu_wr_data,
    input  logic [ADDR_MSB:2]   cpu_addr,
    output logic                cpu_rd_valid,
    output logic [31:0]         cpu_rd_data,
    output logic                cpu_wr_ack,

    bus_slave_interface.master  bus
);

    localparam int WAIT_BITS = $clog2(MAX_LATENCY);

    logic                     busy;
    logic [WAIT_BITS-1:0]     resp_wait;
    logic [ADDR_MSB:2]        latched_addr;
    logic [3:0]               latched_strobe;
    logic [31:0]              latched_wdata;
    logic                     latched_is_write;
    logic                     sram_outstanding;
    logic [31:0]              latched_rdata;

    wire is_write = |cpu_wr_strobe;
    wire accept_req = cpu_valid && !busy;

    assign bus.valid = accept_req;
    assign bus.addr = cpu_addr;
    assign bus.wr_strobe = is_write ? cpu_wr_strobe : 4'b0000;
    assign bus.wr_data = cpu_wr_data;

    function automatic logic [WAIT_BITS-1:0] response_delay(input int unsigned lat);
        if (lat <= 1) begin
            response_delay = WAIT_BITS'(0);
        end else begin
            response_delay = WAIT_BITS'(lat - 2);
        end
    endfunction

    always_ff @(posedge clk) begin
        cpu_rd_valid <= 1'b0;
        cpu_wr_ack <= 1'b0;

        if (!rst_n) begin
            busy <= 1'b0;
            resp_wait <= '0;
            sram_outstanding <= 1'b0;
            latched_addr <= '0;
            latched_strobe <= '0;
            latched_wdata <= '0;
            latched_is_write <= 1'b0;
            latched_rdata <= '0;
        end else begin
            if (accept_req) begin
                busy <= 1'b1;
                sram_outstanding <= 1'b1;
                latched_addr <= cpu_addr;
                latched_strobe <= cpu_wr_strobe;
                latched_wdata <= cpu_wr_data;
                latched_is_write <= is_write;
                resp_wait <= response_delay(latency);
            end

            if (sram_outstanding) begin
                if (latched_is_write && bus.wr_ack) begin
                    sram_outstanding <= 1'b0;
                    if (resp_wait == '0) begin
                        cpu_wr_ack <= 1'b1;
                        busy <= 1'b0;
                    end
                end else if (!latched_is_write && bus.rd_valid) begin
                    sram_outstanding <= 1'b0;
                    latched_rdata <= bus.rd_data;
                    if (resp_wait == '0) begin
                        cpu_rd_valid <= 1'b1;
                        cpu_rd_data <= bus.rd_data;
                        busy <= 1'b0;
                    end
                end
            end else if (busy && resp_wait != '0) begin
                resp_wait <= resp_wait - 1'b1;
                if (resp_wait == 1) begin
                    if (latched_is_write) begin
                        cpu_wr_ack <= 1'b1;
                    end else begin
                        cpu_rd_valid <= 1'b1;
                        cpu_rd_data <= latched_rdata;
                    end
                    busy <= 1'b0;
                end
            end
        end
    end

endmodule
