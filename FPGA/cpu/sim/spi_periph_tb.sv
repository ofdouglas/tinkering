`timescale 1ns / 1ps

module spi_periph_tb;

    localparam time CLK_PERIOD = 10ns;

    logic clk;
    logic rst_n;
    logic sck;
    logic cs_n;
    logic mosi;
    logic miso;

    bus_slave_interface #(.ADDR_MSB(7)) bus();

    assign bus.clk = clk;
    assign bus.rst_n = rst_n;
    assign miso = mosi;

    spi_periph dut (
        .bus(bus.slave),
        .sck(sck),
        .cs_n(cs_n),
        .mosi(mosi),
        .miso(miso)
    );

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    task automatic bus_write(
        input logic [7:2] word_addr,
        input logic [31:0] data,
        input logic [3:0] strobe
    );
        @(negedge clk);
        bus.addr = word_addr;
        bus.wr_data = data;
        bus.wr_strobe = strobe;
        bus.valid = 1'b1;

        while (bus.wr_ack !== 1'b1 && bus.error !== 1'b1) begin
            @(negedge clk);
        end
        if (bus.error) begin
            $fatal(1, "spi_periph_tb: write error at word address %0d", word_addr);
        end

        bus.valid = 1'b0;
        bus.wr_strobe = '0;
        @(posedge clk);
        @(negedge clk);
    endtask

    task automatic bus_read(
        input logic [7:2] word_addr,
        output logic [31:0] data
    );
        @(negedge clk);
        bus.addr = word_addr;
        bus.wr_strobe = '0;
        bus.valid = 1'b1;

        while (bus.rd_valid !== 1'b1 && bus.error !== 1'b1) begin
            @(negedge clk);
        end
        if (bus.error) begin
            $fatal(1, "spi_periph_tb: read error at word address %0d", word_addr);
        end
        data = bus.rd_data;

        bus.valid = 1'b0;
        @(posedge clk);
        @(negedge clk);
    endtask

    task automatic transfer_byte(
        input logic [7:0] tx_data,
        output logic [7:0] rx_data
    );
        logic [31:0] status;
        logic [31:0] received;

        do begin
            bus_read(6'd2, status);
        end while (!status[0]);

        bus_write(6'd4, {24'b0, tx_data}, 4'b0001);

        do begin
            bus_read(6'd2, status);
        end while (!status[1]);

        bus_read(6'd3, received);
        rx_data = received[7:0];
    endtask

    initial begin
        logic [7:0] received;
        logic [31:0] status;

        rst_n = 1'b0;
        bus.valid = 1'b0;
        bus.wr_strobe = '0;
        bus.wr_data = '0;
        bus.addr = '0;
        repeat (4) @(posedge clk);
        rst_n = 1'b1;
        repeat (2) @(posedge clk);

        if (cs_n !== 1'b1) begin
            $fatal(1, "spi_periph_tb: chip select active after reset");
        end

        bus_write(6'd0, 32'h0000_0001, 4'b0001);

        transfer_byte(8'ha5, received);
        if (received !== 8'ha5 || cs_n !== 1'b0) begin
            $fatal(1, "spi_periph_tb: first transfer got 0x%02x cs_n=%b", received, cs_n);
        end

        bus_write(6'd2, 32'h0000_0000, 4'b0001);
        if (cs_n !== 1'b0) begin
            $fatal(1, "spi_periph_tb: writing zero cleared cs_active");
        end

        transfer_byte(8'h3c, received);
        if (received !== 8'h3c || cs_n !== 1'b0) begin
            $fatal(1, "spi_periph_tb: second transfer got 0x%02x cs_n=%b", received, cs_n);
        end

        bus_read(6'd2, status);
        if (status[2] !== 1'b1) begin
            $fatal(1, "spi_periph_tb: status did not report cs_active");
        end

        bus_write(6'd2, 32'h0000_0004, 4'b0001);
        if (cs_n !== 1'b1) begin
            $fatal(1, "spi_periph_tb: W1C did not release chip select");
        end

        $display("spi_periph_tb: PASS");
        $finish;
    end

    initial begin
        #1ms;
        $fatal(1, "spi_periph_tb: timed out");
    end

endmodule
