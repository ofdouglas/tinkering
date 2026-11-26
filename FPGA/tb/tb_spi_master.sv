`timescale 1ns / 1ps
`default_nettype none

module tb_spi_master();

    localparam int kNumDataBits = 8;
    localparam time kClkPeriod = 10ns;

    logic                    clk;
    logic                    rst_n;
    logic [kNumDataBits-1:0] data_in;
    logic                    valid_in;
    logic                    in_ready;
    logic [kNumDataBits-1:0] data_out;
    logic                    valid_out;
    logic                    sck;
    logic                    mosi;
    logic                    miso;

    int unsigned tests_passed = 0;
    int unsigned tests_failed = 0;

    spi_master dut (
        .clk(clk),
        .rst_n(rst_n),
        .data_in(data_in),
        .valid_in(valid_in),
        .in_ready(in_ready),
        .data_out(data_out),
        .valid_out(valid_out),
        .sck(sck),
        .mosi(mosi),
        .miso(miso)
    );

    initial begin
        clk = 1'b0;
        forever #(kClkPeriod / 2) clk = ~clk;
    end

    initial begin
        string dump_path;
        if ($test$plusargs("trace")) begin
            if (!$value$plusargs("dumpfile=%s", dump_path))
                dump_path = "tb_spi_master.fst";
            $dumpfile(dump_path);
            $dumpvars(0, tb_spi_master);
        end
    end

    task automatic apply_reset;
        rst_n = 1'b0;
        data_in = '0;
        valid_in = 1'b0;
        miso = 1'b0;
        repeat (4) @(posedge clk);
        rst_n = 1'b1;
        repeat (2) @(posedge clk);
    endtask

    task automatic run_test_case(
        input string name,
        input logic [kNumDataBits-1:0] master_tx_bits,
        input logic [kNumDataBits-1:0] slave_tx_bits
    );
        logic [kNumDataBits-1:0] slave_rx_bits;
        logic [kNumDataBits-1:0] master_rx_bits;
        logic [kNumDataBits-1:0] slave_shift_bits;
        bit                      case_ok;

        slave_rx_bits = '0;
        master_rx_bits = '0;
        slave_shift_bits = slave_tx_bits;
        case_ok = 1'b1;

        wait (in_ready === 1'b1);
        @(negedge clk);
        data_in = master_tx_bits;
        valid_in = 1'b1;
        miso = slave_shift_bits[kNumDataBits-1];
        @(posedge clk);
        @(negedge clk);
        valid_in = 1'b0;

        if (in_ready !== 1'b0) begin
            $error("[%0t] %s: in_ready stayed high during transfer", $time, name);
            case_ok = 1'b0;
        end

        for (int bit_index = 0; bit_index < kNumDataBits; bit_index++) begin
            @(posedge sck);
            slave_rx_bits = {slave_rx_bits[kNumDataBits-2:0], mosi};

            if (bit_index != kNumDataBits - 1) begin
                @(negedge sck);
                slave_shift_bits = {slave_shift_bits[kNumDataBits-2:0], 1'b0};
                miso = slave_shift_bits[kNumDataBits-1];
            end
        end

        @(negedge sck);
        wait (valid_out === 1'b1);
        master_rx_bits = data_out;

        if (slave_rx_bits !== master_tx_bits) begin
            $error("[%0t] %s: MOSI got 0x%02x, expected 0x%02x",
                   $time, name, slave_rx_bits, master_tx_bits);
            case_ok = 1'b0;
        end
        if (master_rx_bits !== slave_tx_bits) begin
            $error("[%0t] %s: MISO got 0x%02x, expected 0x%02x",
                   $time, name, master_rx_bits, slave_tx_bits);
            case_ok = 1'b0;
        end
        if (sck !== 1'b0 || in_ready !== 1'b1) begin
            $error("[%0t] %s: bad completed state sck=%b in_ready=%b",
                   $time, name, sck, in_ready);
            case_ok = 1'b0;
        end

        @(posedge clk);
        #1ps;
        if (valid_out !== 1'b0) begin
            $error("[%0t] %s: valid_out was not a one-cycle pulse", $time, name);
            case_ok = 1'b0;
        end

        if (case_ok) begin
            tests_passed++;
            $display("[%0t] PASS: %s (master 0x%02x, slave 0x%02x)",
                     $time, name, master_tx_bits, slave_tx_bits);
        end else begin
            tests_failed++;
        end
    endtask

    initial begin
        apply_reset();

        run_test_case("all zeroes",        8'h00, 8'h00);
        run_test_case("all ones",          8'hff, 8'hff);
        run_test_case("alternating bits",  8'haa, 8'h55);
        run_test_case("walking endpoints", 8'h80, 8'h01);
        run_test_case("mixed patterns",     8'ha5, 8'h3c);

        $display("");
        if (tests_failed != 0) begin
            $fatal(1, "tb_spi_master: FAIL %0d / %0d",
                   tests_failed, tests_passed + tests_failed);
        end
        $display("tb_spi_master: PASS %0d / %0d", tests_passed, tests_passed);
        $finish;
    end

    initial begin
        #1ms;
        $fatal(1, "tb_spi_master: timed out");
    end

endmodule

`default_nettype wire
