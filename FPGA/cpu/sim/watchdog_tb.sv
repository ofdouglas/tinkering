`timescale 1ns / 1ps

module watchdog_tb;

    localparam time CLK_PERIOD = 10ns;
    localparam int ADDR_MSB = 7;

    localparam logic [ADDR_MSB:2] ADDR_CONTROL = 0;
    localparam logic [ADDR_MSB:2] ADDR_CONFIG  = 1;
    localparam logic [ADDR_MSB:2] ADDR_SERVICE = 2;
    localparam logic [ADDR_MSB:2] ADDR_INVALID = 3;

    localparam logic [15:0] kServiceKey1     = 16'hA703;
    localparam logic [15:0] kServiceKey2     = 16'hF8FC;
    localparam logic [15:0] kServiceStopKey1 = 16'h6AED;
    localparam logic [15:0] kServiceStopKey2 = 16'h9106;

    logic clk;
    logic rst_n;
    logic reset_request;

    int unsigned tests_passed = 0;
    int unsigned tests_failed = 0;

    bus_slave_interface #(.ADDR_MSB(ADDR_MSB)) bus();

    watchdog_periph dut (
        .bus           (bus.slave),
        .reset_request (reset_request)
    );

    assign bus.clk   = clk;
    assign bus.rst_n = rst_n;

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    initial begin
        string dump_path;
        if ($test$plusargs("trace")) begin
            if (!$value$plusargs("dumpfile=%s", dump_path))
                dump_path = "watchdog_tb.fst";
            $dumpfile(dump_path);
            $dumpvars(0, watchdog_tb);
        end
    end

    task automatic fail(input string message);
        tests_failed++;
        $error("[%0t] FAIL: %s", $time, message);
    endtask

    task automatic pass(input string message);
        tests_passed++;
        $display("[%0t] PASS: %s", $time, message);
    endtask

    task automatic check(input bit cond, input string message);
        if (cond) begin
            pass(message);
        end else begin
            fail(message);
        end
    endtask

    task automatic bus_idle;
        bus.valid     = 1'b0;
        bus.wr_strobe = 4'b0000;
        bus.wr_data   = '0;
        bus.addr      = '0;
    endtask

    task automatic wait_posedge(input int unsigned cycles = 1);
        repeat (cycles) @(posedge clk);
    endtask

    task automatic apply_reset;
        rst_n = 1'b0;
        bus_idle();
        wait_posedge(4);
        rst_n = 1'b1;
        wait_posedge(2);
    endtask

    task automatic bus_write(
        input logic [ADDR_MSB:2] addr,
        input logic [31:0] data,
        input logic [3:0] strobe = 4'b1111
    );
        @(negedge clk);
        bus.valid     = 1'b1;
        bus.wr_strobe = strobe;
        bus.wr_data   = data;
        bus.addr      = addr;
        @(posedge clk);
        @(negedge clk);
        if (bus.wr_ack !== 1'b1) begin
            fail($sformatf("write addr=%0d data=0x%08x missing wr_ack", addr, data));
        end
        if (bus.error !== 1'b0) begin
            fail($sformatf("write addr=%0d data=0x%08x unexpected bus.error", addr, data));
        end
        bus_idle();
    endtask

    task automatic bus_read(
        input  logic [ADDR_MSB:2] addr,
        output logic [31:0] data
    );
        @(negedge clk);
        bus.valid     = 1'b1;
        bus.wr_strobe = 4'b0000;
        bus.wr_data   = '0;
        bus.addr      = addr;
        @(posedge clk);
        @(negedge clk);
        if (bus.rd_valid !== 1'b1) begin
            fail($sformatf("read addr=%0d missing rd_valid", addr));
        end
        if (bus.error !== 1'b0) begin
            fail($sformatf("read addr=%0d unexpected bus.error", addr));
        end
        data = bus.rd_data;
        bus_idle();
    endtask

    task automatic expect_read(input logic [ADDR_MSB:2] addr, input logic [31:0] expected, input string message);
        logic [31:0] data;
        bus_read(addr, data);
        check(data === expected, $sformatf("%s (got 0x%08x expected 0x%08x)", message, data, expected));
    endtask

    task automatic write_service(input logic [15:0] key);
        bus_write(ADDR_SERVICE, {16'h0000, key});
    endtask

    task automatic service_watchdog;
        write_service(kServiceKey1);
        write_service(kServiceKey2);
    endtask

    task automatic stop_watchdog;
        write_service(kServiceStopKey1);
        write_service(kServiceStopKey2);
    endtask

    // One watchdog tick is (prescaler_max + 1) clocks while counting is enabled.
    function automatic int unsigned timeout_cycles(input int unsigned wdg_max, input int unsigned presc_max);
        return (wdg_max + 1) * (presc_max + 1);
    endfunction

    task automatic wait_and_expect_reset(
        input int unsigned cycles_until_reset,
        input string message
    );
        wait_posedge(cycles_until_reset - 1);
        @(negedge clk);
        if (reset_request !== 1'b0) begin
            fail($sformatf("%s: reset_request rose early", message));
            return;
        end
        @(posedge clk);
        @(negedge clk);
        check(reset_request === 1'b1, message);
    endtask

    task automatic wait_for_reset(input int unsigned max_cycles, input string message);
        int unsigned waited;
        waited = 0;
        while (reset_request !== 1'b1 && waited < max_cycles) begin
            @(posedge clk);
            waited++;
        end
        @(negedge clk);
        check(reset_request === 1'b1, message);
    endtask

    task automatic test_reset_state;
        logic [31:0] data;
        apply_reset();
        check(reset_request === 1'b0, "reset_request clear after rst_n");
        expect_read(ADDR_CONTROL, 32'h0, "control is 0 after reset");
        expect_read(ADDR_CONFIG, 32'h0, "config is 0 after reset");
        bus_read(ADDR_SERVICE, data);
        check(data === 32'h0, "service reads as 0");
    endtask

    task automatic test_config_rw_and_enable;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h0003_0001);
        expect_read(ADDR_CONFIG, 32'h0003_0001, "config readback");

        bus_write(ADDR_CONTROL, 32'h1);
        expect_read(ADDR_CONTROL, 32'h1, "enable_counting set");

        bus_write(ADDR_CONTROL, 32'h0);
        expect_read(ADDR_CONTROL, 32'h1, "enable_counting cannot be cleared by SW");

        bus_write(ADDR_CONFIG, 32'h00AA_00BB);
        expect_read(ADDR_CONFIG, 32'h0003_0001, "config frozen while counting");
    endtask

    task automatic test_timeout_prescaler_0;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h0003_0000); // 4 ticks, every clock
        bus_write(ADDR_CONTROL, 32'h1);
        wait_and_expect_reset(timeout_cycles(3, 0), "timeout with prescaler_max=0");
    endtask

    task automatic test_timeout_with_prescaler;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h0001_0002); // 2 ticks * 3 clocks
        bus_write(ADDR_CONTROL, 32'h1);
        wait_and_expect_reset(timeout_cycles(1, 2), "timeout with prescaler_max=2");
    endtask

    task automatic test_service_prevents_timeout;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h000F_0000); // 16 ticks, plenty of margin for bus cycles
        bus_write(ADDR_CONTROL, 32'h1);

        wait_posedge(4);
        @(negedge clk);
        check(reset_request === 1'b0, "no reset before service");
        service_watchdog();

        wait_posedge(4);
        @(negedge clk);
        check(reset_request === 1'b0, "service postponed timeout");
        service_watchdog();

        // Original 16-tick window from enable has elapsed; kicks should have restarted it.
        wait_posedge(timeout_cycles(15, 0) - 4);
        @(negedge clk);
        check(reset_request === 1'b0, "still running after repeated service");

        wait_for_reset(timeout_cycles(15, 0) + 8, "timeout after last service window");
    endtask

    task automatic test_stop_sequence;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h000F_0000);
        bus_write(ADDR_CONTROL, 32'h1);

        wait_posedge(2);
        stop_watchdog();
        expect_read(ADDR_CONTROL, 32'h0, "stop sequence clears enable_counting");

        wait_posedge(timeout_cycles(15, 0) + 4);
        @(negedge clk);
        check(reset_request === 1'b0, "stopped watchdog does not timeout");
    endtask

    task automatic test_wrong_sequence_does_not_act;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h000F_0000);
        bus_write(ADDR_CONTROL, 32'h1);

        write_service(kServiceKey1);
        write_service(kServiceStopKey2); // not the matching second key
        expect_read(ADDR_CONTROL, 32'h1, "mismatched keys do not stop counting");
        @(negedge clk);
        check(reset_request === 1'b0, "mismatched keys do not reset");

        wait_for_reset(timeout_cycles(15, 0) + 8, "timeout after ignored mismatched sequence");
    endtask

    task automatic test_undefined_key_resets;
        apply_reset();
        write_service(16'hDEAD);
        @(negedge clk);
        check(reset_request === 1'b1, "undefined service key requests reset");
    endtask

    task automatic test_module_reset;
        apply_reset();
        bus_write(ADDR_CONFIG, 32'h0010_0003);
        bus_write(ADDR_CONTROL, 32'h1);
        expect_read(ADDR_CONTROL, 32'h1, "counting enabled before module reset attempt");

        bus_write(ADDR_CONTROL, 32'h2); // reset_module while counting
        expect_read(ADDR_CONTROL, 32'h1, "reset_module ignored while counting");
        expect_read(ADDR_CONFIG, 32'h0010_0003, "config unchanged when reset_module ignored");

        stop_watchdog();
        bus_write(ADDR_CONTROL, 32'h2);
        wait_posedge(2);
        expect_read(ADDR_CONTROL, 32'h0, "module reset clears enable");
        expect_read(ADDR_CONFIG, 32'h0, "module reset clears config");
        @(negedge clk);
        check(reset_request === 1'b0, "module reset does not request system reset");
    endtask

    task automatic test_invalid_address;
        apply_reset();
        @(negedge clk);
        bus.valid     = 1'b1;
        bus.wr_strobe = 4'b1111;
        bus.wr_data   = 32'h1;
        bus.addr      = ADDR_INVALID;
        @(posedge clk);
        @(negedge clk);
        check(bus.error === 1'b1, "write to invalid addr sets error");
        check(bus.wr_ack === 1'b0, "write to invalid addr has no wr_ack");
        bus_idle();

        @(negedge clk);
        bus.valid     = 1'b1;
        bus.wr_strobe = 4'b0000;
        bus.addr      = ADDR_INVALID;
        @(posedge clk);
        @(negedge clk);
        check(bus.error === 1'b1, "read from invalid addr sets error");
        check(bus.rd_valid === 1'b0, "read from invalid addr has no rd_valid");
        bus_idle();
    endtask

    initial begin
        rst_n = 1'b0;
        bus_idle();

        test_reset_state();
        test_config_rw_and_enable();
        test_timeout_prescaler_0();
        test_timeout_with_prescaler();
        test_service_prevents_timeout();
        test_stop_sequence();
        test_wrong_sequence_does_not_act();
        test_undefined_key_resets();
        test_module_reset();
        test_invalid_address();

        $display("");
        if (tests_failed != 0) begin
            $fatal(1, "watchdog_tb: FAIL %0d / %0d", tests_failed, tests_passed + tests_failed);
        end
        $display("watchdog_tb: PASS %0d / %0d", tests_passed, tests_passed);
        $finish;
    end

    initial begin
        #1ms;
        $fatal(1, "watchdog_tb: timed out");
    end

endmodule
