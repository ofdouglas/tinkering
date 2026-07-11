`timescale 1ns / 1ps

module cpu_tb;

    localparam time CLK_PERIOD = 10ns;  // 100 MHz, matches Nexys Video sysclk
    localparam int ROM_ADDR_BITS = 10;  // 4 KiB instruction ROM, matches linker.ld
    localparam int ROM_WORDS = 2 ** ROM_ADDR_BITS;
    localparam int DATA_ADDR_BITS = 10; // 4 KiB data SRAM, matches cpu_config_pkg
    localparam int SRAM_WORDS = 2 ** DATA_ADDR_BITS;
    localparam int NUM_REGS = 32;
    localparam logic [3:0] REGION_ROM = 4'h0;
    localparam logic [3:0] REGION_RAM = 4'h1;
    localparam logic [31:0] NOP_INSTRUCTION = 32'h0000_0013;

    logic clk;
    logic rst_n;
    logic test_failed = 1'b0;

    logic [31:0] instruction_fetch;
    logic [31:2] fetch_addr;
    logic fetch_valid;
    logic valid;
    logic [3:0] wr_strobe;
    logic [31:0] wr_data;
    logic [31:2] addr;
    logic rd_valid;
    logic [31:0] rd_data;
    logic [31:0] sram_rd_data;
    logic wr_ack;
    logic invalid_addr;
    logic invalid_region;
    logic error;
    logic ext_irq;
    bit irq_test = 1'b0;
    bit irq_armed = 1'b0;
    bit irq_done = 1'b0;
    int unsigned irq_settle_cycles = 0;
    localparam logic [31:0] IRQ_WAIT_PC = 32'h0000_003c;

    logic [31:0] instruction_rom [0:ROM_WORDS-1];
    logic [31:0] expected_registers [0:NUM_REGS-1];
    logic [31:0] expected_sram [0:SRAM_WORDS-1];
    bit sram_expected_loaded = 1'b0;
    bit skip_register_check = 1'b0;
    bit verbose = 1'b0;
    bit dump_regs_on_fail = 1'b0;
    int unsigned sram_check_words = 0;
    int unsigned mem_latency = 1;
    int unsigned run_cycles = 20000;
    logic [ROM_ADDR_BITS-1:0] rom_word_addr;
    logic fetch_in_range;
    logic rom_busy;

    bus_slave_interface #(.ADDR_MSB(DATA_ADDR_BITS+1)) sram_bus();
    slow_memory_adapter #(.ADDR_MSB(DATA_ADDR_BITS+1)) mem_adapter (
        .clk         (clk),
        .rst_n       (rst_n),
        .latency     (mem_latency),
        .cpu_valid   (valid && !invalid_addr && !invalid_region),
        .cpu_wr_strobe (wr_strobe),
        .cpu_wr_data (wr_data),
        .cpu_addr    (addr[DATA_ADDR_BITS+1:2]),
        .cpu_rd_valid (rd_valid),
        .cpu_rd_data (sram_rd_data),
        .cpu_wr_ack  (wr_ack),
        .bus         (sram_bus)
    );
    assign sram_bus.clk = clk;
    assign sram_bus.rst_n = rst_n;
    block_sram #(.WORD_ADDR_BITS(DATA_ADDR_BITS)) data_sram (
        .bus(sram_bus.slave)
    );

    assign rom_word_addr = fetch_addr[ROM_ADDR_BITS+1:2];
    assign fetch_in_range = (fetch_addr[31:ROM_ADDR_BITS+2] == '0);
    assign instruction_fetch = fetch_in_range ? instruction_rom[rom_word_addr] : NOP_INSTRUCTION;
    assign fetch_valid = rst_n && fetch_in_range && !rom_busy;

    /* CPU read from SRAM, or ROM (stalls instruction fetch) */
    always_comb begin
        logic [ROM_ADDR_BITS-1:0] rom_word_addr;
        rom_word_addr = addr[ROM_ADDR_BITS+1:2];

        invalid_region = 1'b0;
        case (addr[19:16])
            REGION_ROM: begin
                rd_data = instruction_rom[rom_word_addr];
                invalid_region = |wr_strobe;
                rom_busy = valid ? 1'b1 : 1'b0;
            end
            REGION_RAM: begin
                rd_data = sram_rd_data;
                rom_busy = 1'b0;
            end
            default: begin
                invalid_region = 1'b1;
                rd_data = 'x;
                rom_busy = 1'b0;
            end
        endcase

        invalid_addr = (addr[31:20] != '0) || (addr[15:DATA_ADDR_BITS+2] != '0);
        error = valid && (invalid_addr || invalid_region);
    end


    rv32cpu dut (
        .clk         (clk),
        .rst_n       (rst_n),
        .instruction_fetch (instruction_fetch),
        .fetch_addr (fetch_addr),
        .fetch_valid (fetch_valid),
        .valid (valid),
        .wr_strobe (wr_strobe),
        .wr_data (wr_data),
        .addr (addr),
        .rd_valid (rd_valid),
        .rd_data (rd_data),
        .wr_ack (wr_ack),
        .error (error),
        .ext_irq (ext_irq)
    );

    task automatic try_load_rom(input string rom_path, output bit loaded);
        int rom_file;

        rom_file = $fopen(rom_path, "r");
        loaded = (rom_file != 0);
        if (loaded) begin
            $fclose(rom_file);
            $readmemh(rom_path, instruction_rom);
            if (verbose) begin
                $display("cpu_tb: loaded CPU test ROM from %s", rom_path);
            end
        end
    endtask

    task automatic try_load_expected(input string expected_path, output bit loaded);
        int expected_file;

        expected_file = $fopen(expected_path, "r");
        loaded = (expected_file != 0);
        if (loaded) begin
            $fclose(expected_file);
            $readmemh(expected_path, expected_registers);
            if (verbose) begin
                $display("cpu_tb: loaded CPU expected registers from %s", expected_path);
            end
        end
    endtask

    task automatic try_load_sram_expected(input string expected_path, output bit loaded);
        int expected_file;

        expected_file = $fopen(expected_path, "r");
        loaded = (expected_file != 0);
        if (loaded) begin
            $fclose(expected_file);
            $readmemh(expected_path, expected_sram);
            if (verbose) begin
                $display("cpu_tb: loaded CPU expected SRAM from %s", expected_path);
            end
        end
    endtask

    function automatic string replace_suffix(input string path, input string old_suffix, input string new_suffix);
        int suffix_pos;

        suffix_pos = path.len() - old_suffix.len();
        if (suffix_pos >= 0 && path.substr(suffix_pos, path.len() - 1) == old_suffix) begin
            replace_suffix = {path.substr(0, suffix_pos - 1), new_suffix};
        end else begin
            replace_suffix = path;
        end
    endfunction

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    initial begin
        bit rom_loaded;
        bit expected_loaded;
        string rom_path;
        string expected_path;
        string base_file_name;
        base_file_name = "crc.hex";

        for (int i = 0; i < ROM_WORDS; i++) begin
            instruction_rom[i] = NOP_INSTRUCTION;
        end
        for (int i = 0; i < NUM_REGS; i++) begin
            expected_registers[i] = 'x;
        end
        for (int i = 0; i < SRAM_WORDS; i++) begin
            expected_sram[i] = 'x;
        end

        rom_loaded = 1'b0;
        if ($value$plusargs("CPUTEST_HEX=%s", rom_path)) begin
            try_load_rom(rom_path, rom_loaded);
        end else begin
            try_load_rom(base_file_name, rom_loaded);
            if (!rom_loaded) begin
                try_load_rom({"mem/", base_file_name}, rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom({"../mem/", base_file_name}, rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom({"../../../../../cpu/mem/", base_file_name}, rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom({"../../../../../../cpu/mem/", base_file_name}, rom_loaded);
            end
        end

        if (!rom_loaded) begin
            $fatal(1, $sformatf("cpu_tb: could not load %s; run make cputest in firmware/", base_file_name));
        end

        verbose = $test$plusargs("CPUTEST_VERBOSE");
        dump_regs_on_fail = $test$plusargs("CPUTEST_DUMP_REGS") || verbose;

        expected_loaded = 1'b0;
        if ($value$plusargs("CPUTEST_EXPECTED=%s", expected_path)) begin
            try_load_expected(expected_path, expected_loaded);
        end else begin
            try_load_expected("cputest.expected", expected_loaded);
            if (!expected_loaded) begin
                try_load_expected("mem/cputest.expected", expected_loaded);
            end
            if (!expected_loaded) begin
                try_load_expected("../mem/cputest.expected", expected_loaded);
            end
            if (!expected_loaded) begin
                try_load_expected("../../../../../cpu/mem/cputest.expected", expected_loaded);
            end
            if (!expected_loaded) begin
                try_load_expected("../../../../../../cpu/mem/cputest.expected", expected_loaded);
            end
        end

        if (!expected_loaded) begin
            $fatal(1, "cpu_tb: could not load cputest.expected; run make cputest in firmware/");
        end

        if ($test$plusargs("CPUTEST_SKIP_REGS")) begin
            skip_register_check = 1'b1;
        end

        sram_check_words = 0;
        if (!$value$plusargs("CPUTEST_SRAM_WORDS=%d", sram_check_words)) begin
            sram_check_words = SRAM_WORDS;
        end

        mem_latency = 1;
        if ($value$plusargs("CPUTEST_MEM_LATENCY=%d", mem_latency)) begin
            if (mem_latency < 1) begin
                $fatal(1, "cpu_tb: CPUTEST_MEM_LATENCY must be >= 1 (got %0d)", mem_latency);
            end
            $display("cpu_tb: slow memory mode enabled, latency=%0d cycle(s)", mem_latency);
        end

        run_cycles = 20000;
        if (!$value$plusargs("CPUTEST_CYCLES=%d", run_cycles)) begin
            if (mem_latency > 1) begin
                run_cycles = 20000 * mem_latency;
            end
        end
        if (mem_latency > 1 || $test$plusargs("CPUTEST_CYCLES")) begin
            $display("cpu_tb: running for %0d clock cycles", run_cycles);
        end

        sram_expected_loaded = 1'b0;
        if ($value$plusargs("CPUTEST_SRAM_EXPECTED=%s", expected_path)) begin
            try_load_sram_expected(expected_path, sram_expected_loaded);
        end else if ($value$plusargs("CPUTEST_EXPECTED=%s", expected_path)) begin
            try_load_sram_expected(replace_suffix(expected_path, ".expected", ".sram.expected"), sram_expected_loaded);
        end else begin
            try_load_sram_expected("cputest.sram.expected", sram_expected_loaded);
            if (!sram_expected_loaded) begin
                try_load_sram_expected("mem/cputest.sram.expected", sram_expected_loaded);
            end
            if (!sram_expected_loaded) begin
                try_load_sram_expected("../mem/cputest.sram.expected", sram_expected_loaded);
            end
            if (!sram_expected_loaded) begin
                try_load_sram_expected("../../../../../cpu/mem/cputest.sram.expected", sram_expected_loaded);
            end
            if (!sram_expected_loaded) begin
                try_load_sram_expected("../../../../../../cpu/mem/cputest.sram.expected", sram_expected_loaded);
            end
        end
    end

    function automatic logic [31:0] rom_insn_at(input logic [31:0] pc);
        if (pc[31:ROM_ADDR_BITS+2] != '0) begin
            return 32'h0000_0013;
        end
        return instruction_rom[pc[ROM_ADDR_BITS+1:2]];
    endfunction

    task automatic dump_cpu_state;
        logic [31:0] fetch_pc;
        logic [31:0] decode_pc;
        logic [31:0] exec_pc;

        fetch_pc  = dut.fetch_regs.current_pc;
        decode_pc = dut.decode_regs.valid ? dut.decode_regs.current_pc : 32'hffff_ffff;
        exec_pc   = dut.execute_regs.valid ? dut.execute_regs.current_pc : 32'hffff_ffff;

        $display("cputest: --- CPU state @ %0t ---", $time);
        $display("cputest:   fetch  PC=0x%08x  insn=0x%08x",
                 fetch_pc, dut.fetch_regs.instruction);
        if (dut.decode_regs.valid) begin
            $display("cputest:   decode PC=0x%08x  mem=%0d wr=%0d  rs1=x%0d rs2=x%0d",
                     decode_pc,
                     dut.decode_regs.mem_ctrl.memory_request,
                     dut.decode_regs.mem_ctrl.memory_write,
                     dut.decode_regs.rs1_index,
                     dut.decode_regs.rs2_index);
        end else begin
            $display("cputest:   decode (invalid)");
        end
        if (dut.execute_regs.valid) begin
            $display("cputest:   exec   PC=0x%08x  mem=%0d wr=%0d  addr=0x%08x",
                     exec_pc,
                     dut.execute_regs.mem_ctrl.memory_request,
                     dut.execute_regs.mem_ctrl.memory_write,
                     dut.execute_regs.exec_result);
        end else begin
            $display("cputest:   exec   (invalid)");
        end
        $display("cputest:   bus    valid=%0d addr=0x%08x wr_strobe=0x%x wr_data=0x%08x rd_valid=%0d",
                 valid, {addr, 2'b00}, wr_strobe, wr_data, rd_valid);
        $display("cputest:   csr    mepc=0x%08x mcause=0x%08x mtval=0x%08x mtvec=0x%08x",
                 dut.machine_special_regs.mepc,
                 dut.machine_special_regs.mcause,
                 dut.machine_special_regs.mtval,
                 {dut.machine_special_regs.mtvec.base, 2'b00});
        $display("cputest:   sp(x2)=0x%08x gp(x3)=0x%08x",
                 dut.x_register_file[2], dut.x_register_file[3]);
        if (dump_regs_on_fail) begin
            $display("cputest:   --- register file ---");
            for (int i = 1; i < NUM_REGS; i++) begin
                $display("cputest:   x%0d=0x%08x", i, dut.x_register_file[i]);
            end
        end else begin
            $display("cputest:   (pass +CPUTEST_DUMP_REGS or +CPUTEST_VERBOSE for full x-reg dump)");
        end
        $display("cputest: ---");
    endtask

    task automatic fail_line(input string message);
        if (verbose) begin
            $error("[%0t] FAIL: %s", $time, message);
        end else begin
            $display("cputest: FAIL %s", message);
        end
    endtask

    task automatic fail_done;
        if (test_failed) begin
            return;
        end
        dump_cpu_state();
        test_failed = 1'b1;
    endtask

    task automatic fail(input string message);
        fail_line(message);
        fail_done();
    endtask

    task automatic check_cputest_registers;
        int unsigned mismatch_count;

        if (skip_register_check) begin
            return;
        end

        mismatch_count = 0;
        for (int i = 1; i < NUM_REGS; i++) begin
            logic [31:0] actual;
            logic [31:0] expected;

            expected = expected_registers[i];
            if ($isunknown(expected)) begin
                continue;
            end

            actual = dut.x_register_file[i];
            if (actual !== expected) begin
                fail_line($sformatf("x%0d mismatch: got 0x%08x expected 0x%08x", i, actual, expected));
                mismatch_count++;
            end
        end

        if (mismatch_count > 0) begin
            fail_done();
        end
    endtask

    task automatic check_sram_word(input int unsigned index, input logic [31:0] expected);
        // Kept for compatibility; prefer check_cputest_sram batch reporting.
        logic [31:0] actual;

        if ($isunknown(expected)) begin
            return;
        end

        actual = data_sram.sram_word_array[index];
        if (actual !== expected) begin
            fail_line($sformatf("SRAM word %0d mismatch: got 0x%08x expected 0x%08x", index, actual, expected));
        end
    endtask

    task automatic check_cputest_sram;
        int unsigned limit;
        int unsigned mismatch_count;

        if (!sram_expected_loaded) begin
            return;
        end

        limit = (sram_check_words == 0) ? SRAM_WORDS : sram_check_words;
        if (limit > SRAM_WORDS) begin
            limit = SRAM_WORDS;
        end

        mismatch_count = 0;
        for (int i = 0; i < limit; i++) begin
            logic [31:0] actual;
            logic [31:0] expected;

            expected = expected_sram[i];
            if ($isunknown(expected)) begin
                continue;
            end

            actual = data_sram.sram_word_array[i];
            if (actual !== expected) begin
                fail_line($sformatf("SRAM word %0d mismatch: got 0x%08x expected 0x%08x",
                                     i, actual, expected));
                mismatch_count++;
            end
        end

        if (mismatch_count > 0) begin
            fail_done();
        end
    endtask

    always_ff @(posedge clk) begin
        if (rst_n && valid && (invalid_addr || invalid_region)) begin
            fail($sformatf(
                "data bus access out of range: addr=0x%08x wr_strobe=0x%x wr_data=0x%08x (valid regions are ROM 0x00000000-0x00000fff and RAM 0x00010000-0x00010fff)",
                {addr, 2'b00}, wr_strobe, wr_data));
        end
    end

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            ext_irq <= 1'b0;
            irq_armed <= 1'b0;
            irq_done <= 1'b0;
            irq_settle_cycles <= 0;
        end else if (irq_test) begin
            // TODO: refactor this to be more flexible
            if (!irq_armed &&
                dut.x_register_file[18] == 32'h0000_010d &&
                dut.fetch_regs.current_pc == IRQ_WAIT_PC) begin
                irq_armed <= 1'b1;
                ext_irq <= 1'b1;
            end else begin
                ext_irq <= 1'b0;
            end

            if (irq_armed && dut.x_register_file[9] == 32'h0000_00FF) begin
                irq_done <= 1'b1;
            end

            if (irq_done) begin
                irq_settle_cycles <= irq_settle_cycles + 1;
            end
        end else begin
            ext_irq <= 1'b0;
        end
    end

    initial begin
        #0; // let plusarg/config initial block run first
        irq_test = $test$plusargs("CPUTEST_IRQ");
        rst_n = 1'b0;
        ext_irq = 1'b0;
        repeat (4) @(posedge clk);
        @(negedge clk);
        rst_n = 1'b1;

        if (irq_test) begin
            while (!irq_done) @(posedge clk);
            while (irq_settle_cycles < 200) @(posedge clk);
        end else begin
            repeat (run_cycles) @(posedge clk);
        end

        check_cputest_registers();
        check_cputest_sram();
        if (!test_failed) begin
            if (verbose) begin
                if (!skip_register_check) begin
                    $display("[%0t] PASS: CPU test firmware register results verified", $time);
                end else begin
                    $display("[%0t] PASS: CPU test firmware register check skipped", $time);
                end
                if (sram_expected_loaded) begin
                    $display("[%0t] PASS: CPU test firmware SRAM contents verified", $time);
                end
            end else begin
                $display("cputest: PASS");
            end
        end
        $finish;
    end

    initial begin
        #0;
        #(CLK_PERIOD * int'(run_cycles + 50000));
        if (!test_failed) begin
            fail("timed out waiting for CPU test firmware to complete");
        end
    end

endmodule
