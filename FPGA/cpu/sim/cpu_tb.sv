`timescale 1ns / 1ps

module cpu_tb;

    localparam time CLK_PERIOD = 10ns;  // 100 MHz, matches Nexys Video sysclk
    localparam time TEST_TIMEOUT = 5us;
    localparam int ROM_ADDR_BITS = 8;
    localparam int ROM_WORDS = 2 ** ROM_ADDR_BITS;
    localparam int DATA_ADDR_BITS = 6;
    localparam int DATA_WORDS = 2 ** DATA_ADDR_BITS;
    localparam int NUM_REGS = 32;
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
    logic wr_ack;
    logic error;

    logic [31:0] instruction_rom [0:ROM_WORDS-1];
    logic [31:0] expected_registers [0:NUM_REGS-1];
    logic [ROM_ADDR_BITS-1:0] rom_word_addr;
    logic fetch_in_range;
    logic [31:0] data_ram [0:DATA_WORDS-1];
    logic [DATA_ADDR_BITS-1:0] data_word_addr;
    logic data_in_range;
    logic data_write;

    assign rom_word_addr = fetch_addr[ROM_ADDR_BITS+1:2];
    assign fetch_in_range = (fetch_addr[31:ROM_ADDR_BITS+2] == '0);
    assign instruction_fetch = fetch_in_range ? instruction_rom[rom_word_addr] : NOP_INSTRUCTION;
    assign fetch_valid = rst_n && fetch_in_range;

    assign data_word_addr = addr[DATA_ADDR_BITS+1:2];
    assign data_in_range = (addr[31:DATA_ADDR_BITS+2] == '0);
    assign data_write = valid && (wr_strobe != 4'b0000);
    assign rd_valid = valid && (wr_strobe == 4'b0000) && data_in_range;
    assign rd_data = data_in_range ? data_ram[data_word_addr] : '0;
    assign wr_ack = data_write && data_in_range;
    assign error = valid && !data_in_range;

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
        .error (error)
    );

    task automatic try_load_rom(input string rom_path, output bit loaded);
        int rom_file;

        rom_file = $fopen(rom_path, "r");
        loaded = (rom_file != 0);
        if (loaded) begin
            $fclose(rom_file);
            $readmemh(rom_path, instruction_rom);
            $display("cpu_tb: loaded CPU test ROM from %s", rom_path);
        end
    endtask

    task automatic try_load_expected(input string expected_path, output bit loaded);
        int expected_file;

        expected_file = $fopen(expected_path, "r");
        loaded = (expected_file != 0);
        if (loaded) begin
            $fclose(expected_file);
            $readmemh(expected_path, expected_registers);
            $display("cpu_tb: loaded CPU expected registers from %s", expected_path);
        end
    endtask

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    initial begin
        for (int i = 0; i < DATA_WORDS; i++) begin
            data_ram[i] = '0;
        end
    end

    always_ff @(posedge clk) begin
        if (rst_n && data_write && data_in_range) begin
            for (int i = 0; i < 4; i++) begin
                if (wr_strobe[i]) begin
                    data_ram[data_word_addr][8*i +: 8] <= wr_data[8*i +: 8];
                end
            end
        end
    end

    initial begin
        bit rom_loaded;
        bit expected_loaded;
        string rom_path;
        string expected_path;

        for (int i = 0; i < ROM_WORDS; i++) begin
            instruction_rom[i] = NOP_INSTRUCTION;
        end
        for (int i = 0; i < NUM_REGS; i++) begin
            expected_registers[i] = 'x;
        end

        rom_loaded = 1'b0;
        if ($value$plusargs("CPUTEST_HEX=%s", rom_path)) begin
            try_load_rom(rom_path, rom_loaded);
        end else begin
            try_load_rom("cputest.hex", rom_loaded);
            if (!rom_loaded) begin
                try_load_rom("mem/cputest.hex", rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom("../mem/cputest.hex", rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom("../../../../../cpu/mem/cputest.hex", rom_loaded);
            end
            if (!rom_loaded) begin
                try_load_rom("../../../../../../cpu/mem/cputest.hex", rom_loaded);
            end
        end

        if (!rom_loaded) begin
            $fatal(1, "cpu_tb: could not load cputest.hex; run make cputest in firmware/");
        end

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
    end

    task automatic fail(input string message);
        $error("[%0t] FAIL: %s", $time, message);
        test_failed = 1'b1;
        $finish;
    endtask

    task automatic check_register(input int unsigned index, input logic [31:0] expected);
        logic [31:0] actual;

        actual = dut.x_register_file[index];
        if (actual !== expected) begin
            fail($sformatf("x%0d mismatch: got 0x%08x expected 0x%08x", index, actual, expected));
        end
    endtask

    task automatic check_cputest_registers;
        for (int i = 1; i < NUM_REGS; i++) begin
            check_register(i, expected_registers[i]);
        end
    endtask

    always_ff @(posedge clk) begin
        if (rst_n && valid && !data_in_range) begin
            fail($sformatf("data bus access out of range: addr=0x%08x wr_strobe=0x%x", {addr, 2'b00}, wr_strobe));
        end
    end

    initial begin
        rst_n = 1'b0;
        repeat (4) @(posedge clk);
        @(negedge clk);
        rst_n = 1'b1;

        repeat (450) @(posedge clk);
        check_cputest_registers();
        if (!test_failed) begin
            $display("[%0t] PASS: CPU test firmware register results verified", $time);
        end
        $finish;
    end

    initial begin
        #TEST_TIMEOUT;
        fail("timed out waiting for CPU test firmware to complete");
    end

endmodule
