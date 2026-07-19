`timescale 1ns / 1ps
import cpu_config_pkg::*;
import test_data_pkg::*;

module system_tb;

    localparam time CLK_PERIOD = 10ns;  // 100 MHz, matches Nexys Video sysclk
    localparam time UART_BIT_PERIOD = (CLK_PERIOD * 2) * 27 * 16; // x2 for 100->50 MHz conversion
    localparam time TEST_TIMEOUT = 2ms;
    localparam int SYSTEM_SRAM_WORDS = 2 ** (MEMORY_ADDR_MSB - 1);
    localparam realtime LED0_PULSE_MIN_NS = 95_000.0;
    localparam realtime LED0_PULSE_MAX_NS = 115_000.0;

    logic clk;
    logic [7:0] led;
    logic uart_tx_out;
    logic uart_rx_in;
    logic [7:0] uart_rx_queue [$]; // Queue of received UART bytes
    logic led0_seen, led0_done, led0_pulse_ok;
    logic led1_seen;
    logic uart_seen;
    logic test_failed;
    logic [31:0] expected_sram [0:SYSTEM_SRAM_WORDS-1];
    bit system_sram_expected_loaded;
    int unsigned system_sram_check_words;
    realtime led0_on_time;
    realtime led0_off_time;
    realtime led0_pulse_duration;

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

    task automatic dump_cpu_context;
        $display("system_tb: --- CPU context @ %s ---", format_time($realtime));
        $display("system_tb:   reset rst_n=%b system_rst_n=%b clk_locked=%b",
            dut.rst_n, dut.system_rst_n, dut.clk_locked);
        $display("system_tb:   fetch  valid=%b current_pc=0x%08x fetch_pc=0x%08x insn=0x%08x",
            dut.cpu.fetch_regs.valid,
            dut.cpu.fetch_regs.current_pc,
            dut.cpu.fetch_regs.fetch_pc,
            dut.cpu.fetch_regs.instruction);
        $display("system_tb:   ifetch addr=0x%08x valid=%b response_match=%b data=0x%08x",
            {dut.cpu.fetch_addr, 2'b00},
            dut.cpu.fetch_valid,
            dut.rom_fetch_response_matches,
            dut.cpu.instruction_fetch);
        $display("system_tb:   decode valid=%b pc=0x%08x rs1=x%0d rs2=x%0d mem=%b wr=%b",
            dut.cpu.decode_regs.valid,
            dut.cpu.decode_regs.current_pc,
            dut.cpu.decode_regs.rs1_index,
            dut.cpu.decode_regs.rs2_index,
            dut.cpu.decode_regs.mem_ctrl.memory_request,
            dut.cpu.decode_regs.mem_ctrl.memory_write);
        $display("system_tb:   exec   valid=%b pc=0x%08x result=0x%08x mem=%b wr=%b rd=x%0d",
            dut.cpu.execute_regs.valid,
            dut.cpu.execute_regs.current_pc,
            dut.cpu.execute_regs.exec_result,
            dut.cpu.execute_regs.mem_ctrl.memory_request,
            dut.cpu.execute_regs.mem_ctrl.memory_write,
            dut.cpu.execute_regs.wb_ctrl.rd_reg_select);
        $display("system_tb:   memory valid=%b wb_data=0x%08x rd=x%0d wb_en=%b mem_stall=%b response_wait=%b",
            dut.cpu.memory_regs.valid,
            dut.cpu.memory_regs.writeback_data,
            dut.cpu.memory_regs.wb_ctrl.rd_reg_select,
            dut.cpu.memory_regs.wb_ctrl.writeback_en,
            dut.cpu.cpu_ctrl.mem_stall,
            dut.cpu.mem_response_wait);
        $display("system_tb:   bus    request=%b addr=0x%08x wr_strobe=0x%x wr_data=0x%08x",
            dut.cpu_request, dut.address_bus, dut.byte_enables, dut.data_write_bus);
        $display("system_tb:   bus    rd_valid=%b rd_data=0x%08x wr_ack=%b error=%b region=0x%x",
            dut.memory_rd_valid,
            dut.data_read_bus,
            dut.memory_wr_ack,
            dut.memory_access_error,
            dut.region);
        $display("system_tb:   faults memory=%b rom=%b periph=%b unaligned=%b debug_leds=0x%x",
            dut.memory_access_error,
            dut.rom_trap,
            dut.periph_trap,
            dut.unaligned_write_trap,
            dut.debug_leds);
        $display("system_tb:   irq    mti=%b mei=%b pending=%b",
            dut.mti_irq, dut.mei_irq, dut.cpu.external_interrupt);
        $display("system_tb:   csr    mtvec=0x%08x mepc=0x%08x mcause=0x%08x mtval=0x%08x",
            {dut.cpu.machine_special_regs.mtvec.base, 2'b00},
            dut.cpu.machine_special_regs.mepc,
            dut.cpu.machine_special_regs.mcause,
            dut.cpu.machine_special_regs.mtval);
        $display("system_tb:   csr    mstatus.mie=%b mpie=%b mie.mti=%b mie.mei=%b",
            dut.cpu.machine_special_regs.mstatus.mie,
            dut.cpu.machine_special_regs.mstatus.mpie,
            dut.cpu.machine_special_regs.mie.mti_enable,
            dut.cpu.machine_special_regs.mie.mei_enable);
        $display("system_tb:   csr   mtime = 0x%016x",  dut.mtime);
        $display("system_tb:   csr   mtime_compare = 0x%016x", dut.peripherals.rv32_machine_timer.comp_value);
        $display("system_tb:   --- integer registers ---");
        for (int i = 0; i < 32; i += 4) begin
            $display("system_tb:   %4s=0x%08x %4s=0x%08x %4s=0x%08x %4s=0x%08x",
                abi_name(i),     dut.cpu.x_register_file[i],
                abi_name(i + 1), dut.cpu.x_register_file[i + 1],
                abi_name(i + 2), dut.cpu.x_register_file[i + 2],
                abi_name(i + 3), dut.cpu.x_register_file[i + 3]);
        end
        $display("SRAM contents:");
        for (int address = 0; address < 64; address += 4) begin
            $display("system_tb:   %0d: 0x%08x 0x%08x 0x%08x 0x%08x", address * 4, 
                dut.cpu_sram.sram_word_array[address], dut.cpu_sram.sram_word_array[address + 1],
                dut.cpu_sram.sram_word_array[address + 2], dut.cpu_sram.sram_word_array[address + 3]);
        end


        $display("system_tb: --- end CPU context ---");
    endtask

    task automatic fail(input string message);
        if (test_failed) begin
            return;
        end
        test_failed = 1'b1;
        $display("[%0t] FAIL: %s", $time, message);
        dump_cpu_context();
        dump_uart_buffer();
        $error("[%0t] system_tb failed", $time);
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
            7: expected_uart_byte = "H";
            8: expected_uart_byte = "i";
            9: expected_uart_byte = ".";
            default: expected_uart_byte = 8'h00;
        endcase
    endfunction

    function automatic string abi_name(input int reg_idx);
        unique case (reg_idx)
            0:  abi_name = "zero";
            1:  abi_name = "ra";
            2:  abi_name = "sp";
            3:  abi_name = "gp";
            4:  abi_name = "tp";
            5:  abi_name = "t0";
            6:  abi_name = "t1";
            7:  abi_name = "t2";
            8:  abi_name = "s0";
            9:  abi_name = "s1";
            10: abi_name = "a0";
            11: abi_name = "a1";
            12: abi_name = "a2";
            13: abi_name = "a3";
            14: abi_name = "a4";
            15: abi_name = "a5";
            16: abi_name = "a6";
            17: abi_name = "a7";
            18: abi_name = "s2";
            19: abi_name = "s3";
            20: abi_name = "s4";
            21: abi_name = "s5";
            22: abi_name = "s6";
            23: abi_name = "s7";
            24: abi_name = "s8";
            25: abi_name = "s9";
            26: abi_name = "s10";
            27: abi_name = "s11";
            28: abi_name = "t3";
            29: abi_name = "t4";
            30: abi_name = "t5";
            31: abi_name = "t6";
            default: abi_name = "?";
        endcase
    endfunction

    function automatic string format_time(input realtime time_ns);
        if (time_ns < 1.0) begin
            return $sformatf("%0.3f ps", time_ns * 1000.0);
        end
        if (time_ns < 1000.0) begin
            return $sformatf("%0.3f ns", time_ns);
        end
        if (time_ns < 1_000_000.0) begin
            return $sformatf("%0.3f us", time_ns / 1000.0);
        end
        if (time_ns < 1_000_000_000.0) begin
            return $sformatf("%0.3f ms", time_ns / 1_000_000.0);
        end
        return $sformatf("%0.3f s", time_ns / 1_000_000_000.0);
    endfunction

    task automatic dump_uart_buffer;
        $display("[%0t] UART buffer: %s", $time, format_uart_queue(uart_rx_queue));
    endtask

    task automatic check_debug_leds;
        if ((led[7] !== 1'b0) || (led[6] !== 1'b0) || (led[5] !== 1'b0) || (led[4] !== 1'b0)) begin
            fail($sformatf("debug LEDs asserted: led[7]=%b led[6]=%b led[5]=%b led[4]=%b",
                led[7], led[6], led[5], led[4]));
        end
    endtask

    function automatic string format_uart_queue(input logic [7:0] bytes [$]);
        format_uart_queue = $sformatf("%0d bytes:", bytes.size());
        for (int i = 0; i < bytes.size(); i++) begin
            if ((bytes[i] >= 8'h20) && (bytes[i] <= 8'h7e)) begin
                format_uart_queue = {format_uart_queue, $sformatf(" '%c'", bytes[i])};
            end else begin
                format_uart_queue = {format_uart_queue, $sformatf(" 0x%02x", bytes[i])};
            end
        end
    endfunction

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

    task automatic write_uart_byte(input logic [7:0] value);
        uart_rx_in = 1'b0;
        #(UART_BIT_PERIOD);
        for (int i = 0; i < 8; i++) begin
            uart_rx_in = value[i];
            #(UART_BIT_PERIOD);
        end
        uart_rx_in = 1'b1;
        #(UART_BIT_PERIOD);
    endtask

    task automatic receive_uart_bytes;
        logic [7:0] rx_byte;
        for (int i = 0; i < 10; i++) begin
            read_uart_byte(rx_byte);
            uart_rx_queue.push_back(rx_byte);
            if ((rx_byte !== expected_uart_byte(i))) begin
                fail($sformatf("UART byte %0d mismatch: got 0x%02x expected 0x%02x",
                    i, rx_byte, expected_uart_byte(i)));
            end
        end
        uart_seen = 1'b1;

        for (int i = 0; i < 30; i++) begin
            read_uart_byte(rx_byte);
            uart_rx_queue.push_back(rx_byte);
        end
    endtask

    task automatic expect_led_on;
        forever begin
            @(posedge clk);
            check_debug_leds();
            if ((led[0] === 1'b1) && (!led0_seen)) begin
                led0_seen = 1'b1;
                led0_on_time = $realtime;
                $display("[%0t] LED0 turned on at %s", $time, format_time(led0_on_time));
            end
            if ((led[0] === 1'b0) && (led0_seen) && (!led0_done)) begin
                led0_off_time = $realtime;
                led0_done = 1'b1;
                led0_pulse_duration = led0_off_time - led0_on_time;

                if ((led0_pulse_duration >= LED0_PULSE_MIN_NS) &&
                    (led0_pulse_duration <= LED0_PULSE_MAX_NS)) begin
                    led0_pulse_ok = 1'b1;
                end else begin
                    fail($sformatf("LED0 pulse duration %s out of range %s-%s",
                        format_time(led0_pulse_duration),
                        format_time(LED0_PULSE_MIN_NS),
                        format_time(LED0_PULSE_MAX_NS)));
                end
                $display("[%0t] LED0 turned off; pulse duration %s",
                    $time, format_time(led0_pulse_duration));
            end
            if ((led[1] === 1'b1) && (!led1_seen)) begin
                led1_seen = 1'b1;
                $display("[%0t] LED1 turned on", $time);
            end

            if ((led0_pulse_ok && led1_seen) || test_failed) begin
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
        led0_seen = 1'b0;
        led0_done = 1'b0;
        led0_pulse_ok = 1'b0;
        led1_seen = 1'b0;
        uart_seen = 1'b0;
        test_failed = 1'b0;
        load_optional_system_sram_expected();
    end

    initial begin
        while (dut.rst_n !== 1'b1) begin
            @(posedge clk);
        end
        repeat (100) @(posedge clk);
        check_debug_leds();
        write_uart_byte("H");
        write_uart_byte("i");
        write_uart_byte(".");
    end

    initial begin
        while (dut.rst_n !== 1'b1) begin
            @(posedge clk);
        end
        expect_led_on();
    end

    initial begin
        while (dut.rst_n !== 1'b1) begin
            @(posedge clk);
        end
        receive_uart_bytes();
    end

    initial begin
        while (dut.rst_n !== 1'b1) begin
            @(posedge clk);
        end
        #TEST_TIMEOUT;
        fail($sformatf("timed out waiting for LED behavior and UART Hello message: led0_seen=%b, led0_done=%b, led0_pulse_ok=%b, led1_seen=%b, uart_seen=%b",
            led0_seen, led0_done, led0_pulse_ok, led1_seen, uart_seen));
    end

    initial begin
        while (dut.rst_n !== 1'b1) begin
            @(posedge clk);
        end
        while (!(led0_pulse_ok && led1_seen && uart_seen) && !test_failed) begin
            @(posedge clk);
        end
        check_debug_leds();
        check_optional_system_sram_expected();
        if (!test_failed) begin
            $display("[%0t] PASS: system basic behavior verified", $time);
        end
        dump_uart_buffer();

        $finish;
    end

endmodule
