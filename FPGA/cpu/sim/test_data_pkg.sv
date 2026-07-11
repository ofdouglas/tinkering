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

    fd = $fopen(path, "r");
    file_readable = (fd != 0);
    if (fd != 0) begin
        $fclose(fd);
    end
endfunction

task automatic append_mem_search_paths(input string basename, ref string paths[$]);
    string search_dirs[$] = '{"", "mem/", "../mem/"};

    foreach (search_dirs[i]) begin
        paths.push_back({search_dirs[i], basename});
    end
endtask

task automatic gather_test_data_candidates(
    input test_data_kind_e kind,
    input string default_test_hex,
    ref string paths[$]
);
    string path;
    string regs_path;
    string hex_path;

    paths.delete();
    case (kind)
        TEST_ROM: begin
            if ($value$plusargs("TEST_HEX=%s", hex_path) ||
                $value$plusargs("CPUTEST_HEX=%s", hex_path)) begin
                paths.push_back(hex_path);
            end else begin
                append_mem_search_paths(default_test_hex, paths);
            end
        end
        TEST_REGS: begin
            if ($value$plusargs("TEST_REGS_EXPECTED=%s", path) ||
                $value$plusargs("TEST_EXPECTED=%s", path) ||
                $value$plusargs("CPUTEST_EXPECTED=%s", path)) begin
                paths.push_back(path);
            end
            if ($value$plusargs("TEST_HEX=%s", hex_path) ||
                $value$plusargs("CPUTEST_HEX=%s", hex_path)) begin
                paths.push_back(replace_suffix(hex_path, ".hex", ".regs"));
            end
            append_mem_search_paths("cputest.regs", paths);
        end
        TEST_SRAM: begin
            if ($value$plusargs("TEST_SRAM_EXPECTED=%s", path) ||
                $value$plusargs("CPUTEST_SRAM_EXPECTED=%s", path)) begin
                paths.push_back(path);
            end
            if ($value$plusargs("TEST_REGS_EXPECTED=%s", regs_path) ||
                $value$plusargs("TEST_EXPECTED=%s", regs_path) ||
                $value$plusargs("CPUTEST_EXPECTED=%s", regs_path)) begin
                paths.push_back(replace_suffix(regs_path, ".regs", ".sram"));
            end
            if ($value$plusargs("TEST_HEX=%s", hex_path) ||
                $value$plusargs("CPUTEST_HEX=%s", hex_path)) begin
                paths.push_back(replace_suffix(hex_path, ".hex", ".sram"));
            end
            append_mem_search_paths("cputest.sram", paths);
        end
    endcase
endtask

task automatic find_test_data_file(
    input test_data_kind_e kind,
    input string default_test_hex,
    output string found_path,
    input bit verbose,
    output bit found
);
    string paths[$];

    found = 1'b0;
    found_path = "";
    gather_test_data_candidates(kind, default_test_hex, paths);
    foreach (paths[i]) begin
        if (file_readable(paths[i])) begin
            found = 1'b1;
            found_path = paths[i];
            if (verbose) begin
                $display("test_data: found %s at %s", test_data_label(kind), found_path);
            end
            return;
        end
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
