package test_data_pkg;

typedef enum {
    TEST_ROM,
    TEST_REGS,
    TEST_SRAM
} test_data_kind_e;

function automatic string replace_suffix(input string path, input string old_suffix, input string new_suffix);
    int suffix_pos;

    suffix_pos = path.len() - old_suffix.len();
    if (suffix_pos >= 0 && path.substr(suffix_pos, path.len() - 1) == old_suffix) begin
        replace_suffix = {path.substr(0, suffix_pos - 1), new_suffix};
    end else begin
        replace_suffix = path;
    end
endfunction

function automatic string test_data_label(input test_data_kind_e kind);
    case (kind)
        TEST_ROM:  test_data_label = "CPU test ROM";
        TEST_REGS: test_data_label = "CPU expected registers";
        TEST_SRAM: test_data_label = "CPU expected SRAM";
    endcase
endfunction

function automatic bit file_readable(input string path);
    int fd;

    if (path.len() == 0) begin
        return 1'b0;
    end
    fd = $fopen(path, "r");
    file_readable = (fd != 0);
    if (fd != 0) begin
        $fclose(fd);
    end
endfunction

task automatic find_test_data_file(
    input test_data_kind_e kind,
    input string default_test_hex,
    output string found_path,
    input bit verbose,
    output bit found
);
    string path;
    string regs_path;
    string hex_path;
    string data_dir;
    string basename;

    found = 1'b0;
    found_path = "";
    basename = "";

    case (kind)
        TEST_ROM: begin
            if (($value$plusargs("TEST_HEX=%s", path) ||
                 $value$plusargs("CPUTEST_HEX=%s", path)) &&
                file_readable(path)) begin
                found_path = path;
                found = 1'b1;
            end else begin
                basename = default_test_hex;
            end
        end
        TEST_REGS: begin
            if (($value$plusargs("TEST_REGS_EXPECTED=%s", path) ||
                 $value$plusargs("TEST_EXPECTED=%s", path) ||
                 $value$plusargs("CPUTEST_EXPECTED=%s", path)) &&
                file_readable(path)) begin
                found_path = path;
                found = 1'b1;
            end else if (($value$plusargs("TEST_HEX=%s", hex_path) ||
                          $value$plusargs("CPUTEST_HEX=%s", hex_path)) &&
                         file_readable(replace_suffix(hex_path, ".hex", ".regs"))) begin
                found_path = replace_suffix(hex_path, ".hex", ".regs");
                found = 1'b1;
            end else begin
                basename = replace_suffix(default_test_hex, ".hex", ".regs");
            end
        end
        TEST_SRAM: begin
            bit hex_plusarg_seen;

            hex_plusarg_seen = 1'b0;
            if (($value$plusargs("TEST_SRAM_EXPECTED=%s", path) ||
                 $value$plusargs("CPUTEST_SRAM_EXPECTED=%s", path)) &&
                file_readable(path)) begin
                found_path = path;
                found = 1'b1;
            end else if (($value$plusargs("TEST_REGS_EXPECTED=%s", regs_path) ||
                          $value$plusargs("TEST_EXPECTED=%s", regs_path) ||
                          $value$plusargs("CPUTEST_EXPECTED=%s", regs_path)) &&
                         file_readable(replace_suffix(regs_path, ".regs", ".sram"))) begin
                found_path = replace_suffix(regs_path, ".regs", ".sram");
                found = 1'b1;
            end else if ($value$plusargs("TEST_HEX=%s", hex_path) ||
                         $value$plusargs("CPUTEST_HEX=%s", hex_path)) begin
                hex_plusarg_seen = 1'b1;
                if (file_readable(replace_suffix(hex_path, ".hex", ".sram"))) begin
                    found_path = replace_suffix(hex_path, ".hex", ".sram");
                    found = 1'b1;
                end
            end

            // Only use the module default when no explicit test hex was provided.
            if (!found && !hex_plusarg_seen) begin
                basename = replace_suffix(default_test_hex, ".hex", ".sram");
            end
        end
    endcase

    if (!found && basename.len() > 0 && ($value$plusargs("TEST_DATA_DIR=%s", data_dir) ||
                   $value$plusargs("CPUTEST_MEM_DIR=%s", data_dir)) &&
        file_readable({data_dir, "/", basename})) begin
        found_path = {data_dir, "/", basename};
        found = 1'b1;
    end
    if (!found && basename.len() > 0 && file_readable(basename)) begin
        found_path = basename;
        found = 1'b1;
    end
    if (!found && basename.len() > 0 && file_readable({"mem/", basename})) begin
        found_path = {"mem/", basename};
        found = 1'b1;
    end
    if (!found && basename.len() > 0 && file_readable({"../mem/", basename})) begin
        found_path = {"../mem/", basename};
        found = 1'b1;
    end
    if (!found && basename.len() > 0 && file_readable({"../../../../../cpu/mem/", basename})) begin
        found_path = {"../../../../../cpu/mem/", basename};
        found = 1'b1;
    end
    if (!found && basename.len() > 0 && file_readable({"../../../../../../cpu/mem/", basename})) begin
        found_path = {"../../../../../../cpu/mem/", basename};
        found = 1'b1;
    end
    if (found && verbose) begin
        $display("test_data: found %s at %s", test_data_label(kind), found_path);
    end
endtask

function automatic bit expected_word_mismatch(
    input logic [31:0] actual,
    input logic [31:0] expected,
    input int unsigned index,
    input string label,
    input bit verbose,
    input string fail_prefix
);
    if ($isunknown(expected)) begin
        return 1'b0;
    end
    if (actual === expected) begin
        return 1'b0;
    end
    if (verbose) begin
        $error("[%0t] FAIL: %s word %0d mismatch: got 0x%08x expected 0x%08x",
               $time, label, index, actual, expected);
    end else begin
        $display("%s%s word %0d mismatch: got 0x%08x expected 0x%08x",
                 fail_prefix, label, index, actual, expected);
    end
    return 1'b1;
endfunction

endpackage
