`timescale 1ns / 1ps
import cpu_config_pkg::*;
import test_data_pkg::*;

module system_tb;

    localparam time CLK_PERIOD = 10ns;  // 100 MHz, matches Nexys Video sysclk
    localparam time UART_BIT_PERIOD = (CLK_PERIOD * 2) * 27 * 16; // x2 for 100->50 MHz conversion
    localparam time TEST_TIMEOUT = 2ms;
    localparam int SYSTEM_SRAM_WORDS = 2 ** (MEMORY_ADDR_MSB - 1);

    logic clk;
    logic [7:0] led;
    logic uart_tx_out;
    logic uart_rx_in;
    logic led_seen;
    logic uart_seen;
    logic test_failed;
    logic [31:0] expected_sram [0:SYSTEM_SRAM_WORDS-1];
    bit system_sram_expected_loaded;
    int unsigned system_sram_check_words;

    system dut (
        .clk_in      (clk),
        .led         (led),
        .uart_tx_out (uart_tx_out),
        .uart_rx_in  (uart_rx_in)
    );

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    task automatic fail(input string message);
        $error("[%0t] FAIL: %s", $time, message);
        test_failed = 1'b1;
        $finish;
    endtask

    function automatic logic [7:0] expected_uart_byte(input int index);
        unique case (index)
            0: expected_uart_byte = "H";
            1: expected_uart_byte = "e";
            2: expected_uart_byte = "l";
            3: expected_uart_byte = "l";
            4: expected_uart_byte = "o";
            5: expected_uart_byte = "!";
            6: expected_uart_byte = "\n";
            default: expected_uart_byte = 8'h00;
        endcase
    endfunction

    task automatic check_debug_leds;
        if ((led[7] !== 1'b0) || (led[6] !== 1'b0) || (led[5] !== 1'b0) || (led[4] !== 1'b0)) begin
            $error("[%0t] FAIL: debug LEDs asserted: led[7]=%b led[6]=%b led[5]=%b led[4]=%b",
                $time, led[7], led[6], led[5], led[4]);
            test_failed = 1'b1;
            $finish;
        end
    endtask

    task automatic read_uart_byte(output logic [7:0] value);
        do begin
            wait (uart_tx_out === 1'b1);
            @(negedge uart_tx_out);
            #(UART_BIT_PERIOD / 2);
        end while (uart_tx_out !== 1'b0);

        for (int i = 0; i < 8; i++) begin
            #(UART_BIT_PERIOD);
            value[i] = uart_tx_out;
        end
        #(UART_BIT_PERIOD);
        if (uart_tx_out !== 1'b1) begin
            fail("UART stop bit was not high at sample point");
        end
    endtask

    task automatic expect_uart_hello;
        logic [7:0] rx_byte;
        for (int i = 0; i < 7; i++) begin
            read_uart_byte(rx_byte);
            if (rx_byte !== expected_uart_byte(i)) begin
                fail($sformatf("UART byte %0d mismatch: got 0x%02x expected 0x%02x",
                    i, rx_byte, expected_uart_byte(i)));
            end
        end
        uart_seen = 1'b1;
        $display("[%0t] UART sent expected Hello message", $time);
    endtask

    task automatic expect_led_on;
        forever begin
            @(posedge clk);
            check_debug_leds();
            if (led[0] === 1'b1) begin
                led_seen = 1'b1;
                $display("[%0t] LED0 turned on", $time);
                return;
            end
        end
    endtask

    task automatic load_optional_system_sram_expected;
        string expected_path;

        system_sram_expected_loaded = 1'b0;
        system_sram_check_words = SYSTEM_SRAM_WORDS;
        for (int i = 0; i < SYSTEM_SRAM_WORDS; i++) begin
            expected_sram[i] = 'x;
        end
        if ($value$plusargs("SYSTEM_SRAM_WORDS=%d", system_sram_check_words)) begin
            // plusarg consumed above
        end else if ($value$plusargs("TEST_SRAM_WORDS=%d", system_sram_check_words)) begin
            // plusarg consumed above
        end
        if ($value$plusargs("SYSTEM_SRAM_EXPECTED=%s", expected_path) ||
            $value$plusargs("TEST_SRAM_EXPECTED=%s", expected_path)) begin
            system_sram_expected_loaded = file_readable(expected_path);
            if (system_sram_expected_loaded) begin
                $readmemh(expected_path, expected_sram);
                $display("test_data: loaded %s from %s", test_data_label(TEST_SRAM), expected_path);
            end
        end
        if (system_sram_check_words > SYSTEM_SRAM_WORDS) begin
            system_sram_check_words = SYSTEM_SRAM_WORDS;
        end
    endtask

    task automatic check_optional_system_sram_expected;
        int unsigned mismatch_count;

        if (!system_sram_expected_loaded) begin
            return;
        end
        mismatch_count = 0;
        for (int i = 0; i < system_sram_check_words; i++) begin
            if (expected_word_mismatch(dut.cpu_sram.sram_word_array[i], expected_sram[i],
                                       i, "system SRAM", 1'b1, "system_tb: FAIL ")) begin
                mismatch_count++;
            end
        end
        if (mismatch_count > 0) begin
            fail($sformatf("system SRAM expected contents had %0d mismatch(es)", mismatch_count));
        end
        $display("[%0t] system SRAM contents matched expected file", $time);
    endtask

    initial begin
        logic [7:0] led_prev = '0;
        forever begin
            @(posedge clk);
            if (led !== led_prev) begin
                $display("[%0t] led -> %0b", $time, led);
                led_prev = led;
            end
        end
    end

    initial begin
        uart_rx_in = 1'b1;
        led_seen = 1'b0;
        uart_seen = 1'b0;
        test_failed = 1'b0;
        load_optional_system_sram_expected();

        wait (dut.rst_n === 1'b1);
        repeat (4) @(posedge clk);
        check_debug_leds();

        fork
            expect_led_on();
            expect_uart_hello();
            begin
                #TEST_TIMEOUT;
                fail($sformatf("timed out waiting for LED0 and UART Hello message: led_seen=%b uart_seen=%b",
                    led_seen, uart_seen));
            end
        join_none

        wait (led_seen && uart_seen);
        disable fork;
        check_debug_leds();
        check_optional_system_sram_expected();
        if (!test_failed) begin
            $display("[%0t] PASS: system basic behavior verified", $time);
        end
        $finish;
    end

endmodule
