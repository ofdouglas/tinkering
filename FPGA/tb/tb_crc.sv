`timescale 1ns / 1ps

module tb_crc;

localparam int DATA_WIDTH = 8;

// ---------------------------------------------------------------------------
// Vivado-safe test vectors (static arrays; extend test_len for multi-byte)
// ---------------------------------------------------------------------------
localparam int NUM_TESTS = 4;
localparam int MAX_BYTES = 100;

byte test_payload[NUM_TESTS][MAX_BYTES];
int test_len[NUM_TESTS];
byte test_expected[NUM_TESTS];
byte test_actual[NUM_TESTS];
int tests_passed = 0;
int tests_failed = 0;

// ---------------------------------------------------------------------------
// Class-based approach (commented out — XSim crashes during elaboration)
// ---------------------------------------------------------------------------
// class TestCase;
//   rand byte unsigned data[];
//   rand byte unsigned expected_crc;
//   rand byte unsigned test_result;
//
//   constraint c_data_size {
//     data.size() inside {[1:100]};
//   }
//
//   function new(byte unsigned incoming_data[], byte unsigned expected_crc = 8'h0);
//     this.data = incoming_data;
//     this.expected_crc = expected_crc;
//     this.test_result = 0;
//   endfunction
// endclass
//
// TestCase tests_to_run[$];
// TestCase tests_completed[$];

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

task automatic load_test_vectors();
    test_payload[0][0] = 8'h42;
    test_len[0] = 1;
    test_expected[0] = 8'h12;
    test_payload[1][0] = 8'hFE;
    test_len[1] = 1;
    test_expected[1] = 8'he2;
    test_payload[2][0] = 8'h00;
    test_len[2] = 1;
    test_expected[2] = 8'h3b;
    test_payload[3][0] = 8'h12;
    test_len[3] = 1;
    test_expected[3] = 8'hcc;
    // Multi-byte example (uncomment, bump NUM_TESTS, set expected CRC):
    // test_payload[4][0] = 8'hFE;
    // test_payload[4][1] = 8'h02;
    // test_len[4] = 2;
    // test_expected[4] = 8'h??;
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

task automatic run_test_case(input int test_index);
    logic [DATA_WIDTH-1:0] result;
    for (int byte_index = 0; byte_index < test_len[test_index]; byte_index++) begin
        process_data(
            test_payload[test_index][byte_index],
            (byte_index == 0),
            result
        );
    end
    test_actual[test_index] = result;
endtask

function bit verify_test_case(input int test_index);
    return test_actual[test_index] == test_expected[test_index];
endfunction

function void print_test_case_failure(input int test_index);
    $display("Test case %0d failed:", test_index);
    for (int byte_index = 0; byte_index < test_len[test_index]; byte_index++) begin
        $display("  data[%0d]: %02h", byte_index, test_payload[test_index][byte_index]);
    end
    $display("  expected CRC: %02h", test_expected[test_index]);
    $display("  actual CRC:   %02h", test_actual[test_index]);
endfunction

task automatic run_all_test_cases();
    for (int test_index = 0; test_index < NUM_TESTS; test_index++) begin
        run_test_case(test_index);
    end
endtask

function void verify_all_test_cases();
    for (int test_index = 0; test_index < NUM_TESTS; test_index++) begin
        if (!verify_test_case(test_index)) begin
            print_test_case_failure(test_index);
            tests_failed++;
        end else begin
            tests_passed++;
        end
    end
endfunction

function void print_test_suite_results();
    if (tests_failed > 0) begin
        $display("FAIL: %d / %d", tests_failed, NUM_TESTS);
    end else begin
        $display("PASS: %d / %d", tests_passed, NUM_TESTS);
    end
endfunction

// ---------------------------------------------------------------------------
// Class-based tasks (commented out — restore when moving off Vivado XSim)
// ---------------------------------------------------------------------------
// task run_test_case(TestCase test_case);
//     process_data(test_case.data[0], 1'b1, test_case.test_result);
// endtask
//
// function bit verify_test_case(TestCase test_case);
//     return test_case.test_result == test_case.expected_crc;
// endfunction
//
// function void print_test_case_failure(TestCase test_case);
//     $display("Test case failed: %h", test_case.data[0]);
//     $display("Expected CRC: %h", test_case.expected_crc);
//     $display("Actual CRC: %h", test_case.test_result);
// endfunction
//
// task run_all_test_cases();
//     TestCase test_case;
//     while (tests_to_run.size() > 0) begin
//         test_case = tests_to_run.pop_front();
//         run_test_case(test_case);
//         tests_completed.push_back(test_case);
//     end
// endtask
//
// function void verify_all_test_cases();
//     foreach (tests_completed[i]) begin
//         if (!verify_test_case(tests_completed[i])) begin
//             print_test_case_failure(tests_completed[i]);
//             tests_failed++;
//         end else begin
//             tests_passed++;
//         end
//     end
// endfunction

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
    load_test_vectors();

    reset = 1'b1;
    #10;
    reset = 1'b0;
    #10;

    run_all_test_cases();
    verify_all_test_cases();
    print_test_suite_results();

    $finish;
end

// initial begin
//     tests_to_run.push_back(TestCase::new('{8'h42}, 8'h12));
//     tests_to_run.push_back(TestCase::new('{8'hFE}, 8'he2));
//     tests_to_run.push_back(TestCase::new('{8'h00}, 8'h3b));
//     tests_to_run.push_back(TestCase::new('{8'h12}, 8'hcc));
//
//     reset = 1'b1;
//     #10;
//     reset = 1'b0;
//     #10;
//
//     run_all_test_cases();
//     verify_all_test_cases();
//     print_test_suite_results();
//
//     $finish;
// end

endmodule
