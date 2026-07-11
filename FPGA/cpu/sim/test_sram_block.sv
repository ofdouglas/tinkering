`timescale 1ns / 1ps
import test_data_pkg::*;

module test_sram_block #(
    parameter int WORD_ADDR_BITS = 10,
    parameter string DEFAULT_TEST_HEX = "crc.hex"
) (
    bus_slave_interface.slave bus
);

localparam int WORDS = 2 ** WORD_ADDR_BITS;

logic [31:0] expected_sram [0:WORDS-1];
bit expected_loaded = 1'b0;

block_sram #(.WORD_ADDR_BITS(WORD_ADDR_BITS)) rtl_sram (
    .bus(bus)
);

task automatic clear_expected;
    expected_loaded = 1'b0;
    for (int i = 0; i < WORDS; i++) begin
        expected_sram[i] = 'x;
    end
endtask

task automatic load_expected(input bit verbose, output bit loaded);
    string path;

    find_test_data_file(TEST_SRAM, DEFAULT_TEST_HEX, path, verbose, loaded);
    expected_loaded = loaded;
    if (loaded) begin
        $readmemh(path, expected_sram);
        if (verbose) begin
            $display("test_sram_block: loaded expected SRAM from %s", path);
        end
    end
endtask

task automatic verify_expected(
    input int unsigned requested_words,
    input bit verbose,
    input string fail_prefix,
    output int unsigned mismatch_count
);
    int unsigned limit;

    mismatch_count = 0;
    if (!expected_loaded) begin
        return;
    end

    limit = (requested_words == 0) ? WORDS : requested_words;
    if (limit > WORDS) begin
        limit = WORDS;
    end

    for (int i = 0; i < limit; i++) begin
        logic [31:0] actual;
        logic [31:0] expected;

        expected = expected_sram[i];
        if ($isunknown(expected)) begin
            continue;
        end

        actual = rtl_sram.sram_word_array[i];
        if (expected_word_mismatch(actual, expected, i, "SRAM", verbose, fail_prefix)) begin
            mismatch_count++;
        end
    end
endtask

function automatic bit has_expected;
    return expected_loaded;
endfunction

endmodule
