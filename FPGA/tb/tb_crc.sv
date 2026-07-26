`timescale 1ns / 1ps

// Class-based CRC testbench for Verilator (XSim elaboration crashes on this style).
module tb_crc;

localparam int DATA_WIDTH = 8;

class TestCase;
    byte unsigned data[];
    byte unsigned expected_crc;
    byte unsigned test_result;

    static function TestCase create(byte unsigned payload[], byte unsigned expected_crc);
        TestCase tc = new();
        tc.data = payload;
        tc.expected_crc = expected_crc;
        tc.test_result = 0;
        return tc;
    endfunction
endclass

TestCase tests_to_run[$];
TestCase tests_completed[$];
int tests_passed = 0;
int tests_failed = 0;

logic clk;
logic reset;
logic [DATA_WIDTH-1:0] data;
logic data_valid;
logic initial_data;
logic ready;
logic [DATA_WIDTH-1:0] crc_out;
logic crc_valid;

crc #(
    .WIDTH(DATA_WIDTH),
    .POLYNOMIAL(8'h1D),
    .INITIAL(8'hFF),
    .FINAL_XOR(8'hFF)
) crc_sae_j1850 (
    .clk(clk),
    .reset(reset),
    .data(data),
    .data_valid(data_valid),
    .initial_data(initial_data),
    .ready(ready),
    .crc_out(crc_out),
    .crc_valid(crc_valid)
);

always #5 clk = ~clk;

initial begin
    clk = 1'b0;
    data = '0;
    initial_data = 1'b0;
end

task automatic load_test_cases();
    byte payload[];
    // Single byte tests
    payload = '{8'h42};
    tests_to_run.push_back(TestCase::create(payload, 8'h12));
    payload = '{8'hFE};
    tests_to_run.push_back(TestCase::create(payload, 8'he2));
    payload = '{8'h00};
    tests_to_run.push_back(TestCase::create(payload, 8'h3b));
    payload = '{8'h12};
    tests_to_run.push_back(TestCase::create(payload, 8'hcc));

    // Two byte tests
    payload = '{8'h12, 8'h34};
    tests_to_run.push_back(TestCase::create(payload, 8'hAC));
    payload = '{8'h99, 8'hC1};
    tests_to_run.push_back(TestCase::create(payload, 8'hFD));
    payload = '{8'h80, 8'h01};
    tests_to_run.push_back(TestCase::create(payload, 8'h6A));

    // Four byte tests
    payload = '{8'hDE, 8'hAD, 8'hBE, 8'hEF};
    tests_to_run.push_back(TestCase::create(payload, 8'hB3));
    payload = '{8'h80, 8'h03, 8'hE0, 8'h01};
    tests_to_run.push_back(TestCase::create(payload, 8'hAB));
    payload = '{8'h77, 8'h33, 8'hEE, 8'h11};
    tests_to_run.push_back(TestCase::create(payload, 8'h9B));

    // String tests
    payload = '{8'h48, 8'h65, 8'h6c, 8'h6c, 8'h6f, 8'h2c, 8'h20, 8'h77, 8'h6f, 8'h72, 8'h6c, 8'h64, 8'h21}; // "Hello, world!"
    tests_to_run.push_back(TestCase::create(payload, 8'h96));
endtask

task automatic process_data(
    input  logic [DATA_WIDTH-1:0] input_data,
    input  logic                  use_initial,
    output logic [DATA_WIDTH-1:0] result
);
    initial_data = use_initial;
    data = input_data;
    data_valid = 1'b1;
    #10;
    data_valid = 1'b0;

    while (crc_valid == 1'b1) begin
        #10;
    end
    while (crc_valid == 1'b0) begin
        #10;
    end
    result = crc_out;
endtask

task automatic run_test_case(ref TestCase test_case);
    logic [DATA_WIDTH-1:0] result;
    for (int byte_index = 0; byte_index < test_case.data.size(); byte_index++) begin
        process_data(
            test_case.data[byte_index],
            (byte_index == 0),
            result
        );
    end
    test_case.test_result = result;
endtask

function automatic bit verify_test_case(TestCase test_case);
    return test_case.test_result == test_case.expected_crc;
endfunction

function automatic void print_test_case_failure(int index, TestCase test_case);
    $display("Test case %d failed:", index);
    for (int byte_index = 0; byte_index < test_case.data.size(); byte_index++) begin
        $display("  data[%0d]: %02h", byte_index, test_case.data[byte_index]);
    end
    $display("  expected CRC: %02h", test_case.expected_crc);
    $display("  actual CRC:   %02h", test_case.test_result);
endfunction

task automatic run_all_test_cases();
    TestCase test_case;
    while (tests_to_run.size() > 0) begin
        test_case = tests_to_run.pop_front();
        run_test_case(test_case);
        tests_completed.push_back(test_case);
    end
endtask

function automatic void verify_all_test_cases();
    foreach (tests_completed[i]) begin
        if (!verify_test_case(tests_completed[i])) begin
            print_test_case_failure(i, tests_completed[i]);
            tests_failed++;
        end else begin
            tests_passed++;
        end
    end
endfunction

function automatic void print_test_suite_results();
    int num_tests = tests_completed.size();
    if (tests_failed > 0) begin
        $display("FAIL: %d / %d", tests_failed, num_tests);
    end else begin
        $display("PASS: %d / %d", tests_passed, num_tests);
    end
endfunction

initial begin
    string dump_path;
    if ($test$plusargs("trace")) begin
        if (!$value$plusargs("dumpfile=%s", dump_path))
            dump_path = "tb_crc.fst";
        $dumpfile(dump_path);
        $dumpvars(0, tb_crc);
    end
end

initial begin
    load_test_cases();

    reset = 1'b1;
    #10;
    reset = 1'b0;
    #10;

    run_all_test_cases();
    verify_all_test_cases();
    print_test_suite_results();

    $finish;
end

endmodule
