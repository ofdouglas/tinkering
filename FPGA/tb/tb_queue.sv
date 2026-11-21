`timescale 1ns / 1ps

module queue_test;

    localparam int WIDTH = 8;
    localparam int DEPTH = 4;
    localparam time CLK_PERIOD = 10ns;

    logic clk;
    logic rst_n;
    logic [WIDTH-1:0] in_data;
    logic in_valid;
    logic in_ready;
    logic [WIDTH-1:0] out_data;
    logic out_valid;
    logic out_strobe;

    int unsigned tests_passed = 0;
    int unsigned tests_failed = 0;

    queue #(.WIDTH(WIDTH), .DEPTH(DEPTH)) dut (
        .clk        (clk),
        .rst_n      (rst_n),
        .in_data    (in_data),
        .in_valid   (in_valid),
        .in_ready   (in_ready),
        .out_data   (out_data),
        .out_valid  (out_valid),
        .out_strobe (out_strobe)
    );

    initial begin
        clk = 1'b0;
        forever #(CLK_PERIOD / 2) clk = ~clk;
    end

    initial begin
        string dump_path;
        if ($test$plusargs("trace")) begin
            if (!$value$plusargs("dumpfile=%s", dump_path))
                dump_path = "queue_test.fst";
            $dumpfile(dump_path);
            $dumpvars(0, queue_test);
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

    task automatic apply_reset;
        rst_n = 1'b0;
        in_valid = 1'b0;
        in_data = '0;
        out_strobe = 1'b0;
        repeat (4) @(posedge clk);
        rst_n = 1'b1;
        repeat (2) @(posedge clk);
    endtask

    task automatic push_byte(input logic [WIDTH-1:0] data);
        @(negedge clk);
        in_data = data;
        in_valid = 1'b1;
        if (in_ready !== 1'b1) begin
            fail($sformatf("push 0x%02x while in_ready=0", data));
        end
        @(posedge clk);
        @(negedge clk);
        in_valid = 1'b0;
    endtask

    task automatic pop_byte(output logic [WIDTH-1:0] data);
        @(negedge clk);
        if (out_valid !== 1'b1) begin
            fail("pop while out_valid=0");
        end
        data = out_data;
        out_strobe = 1'b1;
        @(posedge clk);
        @(negedge clk);
        out_strobe = 1'b0;
    endtask

    task automatic test_reset_empty;
        apply_reset();
        @(negedge clk);
        check(out_valid === 1'b0, "empty after reset");
        check(in_ready === 1'b1, "ready after reset");
    endtask

    task automatic test_fill_and_drain;
        logic [WIDTH-1:0] got;
        apply_reset();

        for (int i = 0; i < DEPTH; i++) begin
            push_byte(8'(i + 8'hA0));
        end
        @(negedge clk);
        check(in_ready === 1'b0, "in_ready low when full");
        check(out_valid === 1'b1, "out_valid high when full");

        @(negedge clk);
        in_data = 8'hFF;
        in_valid = 1'b1;
        @(posedge clk);
        @(negedge clk);
        in_valid = 1'b0;
        check(in_ready === 1'b0, "full queue rejects extra push");

        for (int i = 0; i < DEPTH; i++) begin
            pop_byte(got);
            check(got === 8'(i + 8'hA0),
                $sformatf("drain[%0d] (got 0x%02x expected 0x%02x)", i, got, 8'(i + 8'hA0)));
        end
        @(negedge clk);
        check(out_valid === 1'b0, "empty after drain");
        check(in_ready === 1'b1, "ready after drain");
    endtask

    task automatic test_wraparound;
        logic [WIDTH-1:0] got;
        apply_reset();

        push_byte(8'h11);
        push_byte(8'h22);
        pop_byte(got);
        check(got === 8'h11, "wrap first pop");
        pop_byte(got);
        check(got === 8'h22, "wrap second pop");

        for (int i = 0; i < DEPTH; i++) begin
            push_byte(8'(8'h30 + i));
        end
        for (int i = 0; i < DEPTH; i++) begin
            pop_byte(got);
            check(got === 8'(8'h30 + i),
                $sformatf("wrap drain[%0d] (got 0x%02x expected 0x%02x)", i, got, 8'(8'h30 + i)));
        end
    endtask

    task automatic test_simultaneous_push_pop;
        logic [WIDTH-1:0] got;
        apply_reset();

        push_byte(8'h81);
        @(negedge clk);
        in_data = 8'h82;
        in_valid = 1'b1;
        out_strobe = 1'b1;
        got = out_data;
        @(posedge clk);
        @(negedge clk);
        in_valid = 1'b0;
        out_strobe = 1'b0;
        check(got === 8'h81, "simultaneous pop returns old head");

        pop_byte(got);
        check(got === 8'h82, "item pushed during pop is next out");
        @(negedge clk);
        check(out_valid === 1'b0, "empty after simultaneous push/pop drain");
    endtask

    task automatic test_empty_pop_ignored;
        apply_reset();
        @(negedge clk);
        out_strobe = 1'b1;
        @(posedge clk);
        @(negedge clk);
        out_strobe = 1'b0;
        check(out_valid === 1'b0, "empty pop does not create data");
        check(in_ready === 1'b1, "empty pop leaves in_ready set");
    endtask

    initial begin
        rst_n = 1'b0;
        in_valid = 1'b0;
        in_data = '0;
        out_strobe = 1'b0;

        test_reset_empty();
        test_fill_and_drain();
        test_wraparound();
        test_simultaneous_push_pop();
        test_empty_pop_ignored();

        $display("");
        if (tests_failed != 0) begin
            $fatal(1, "queue_test: FAIL %0d / %0d", tests_failed, tests_passed + tests_failed);
        end
        $display("queue_test: PASS %0d / %0d", tests_passed, tests_passed);
        $finish;
    end

    initial begin
        #1ms;
        $fatal(1, "queue_test: timed out");
    end

endmodule
