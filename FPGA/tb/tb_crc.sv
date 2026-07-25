module tb_crc;

localparam int DATA_WIDTH = 8;

logic clk;
logic reset;
logic [DATA_WIDTH-1:0] data;
logic data_valid;
logic ready;
logic [DATA_WIDTH-1:0] crc_out;
logic crc_valid;

logic [DATA_WIDTH-1:0] result_queue[$];

crc #(
    .WIDTH(8),
    .POLYNOMIAL(8'h1D),
    .INITIAL(8'hFF),
    .FINAL_XOR(0)
) crc_inst (
    .clk(clk),
    .reset(reset),
    .data(data),
    .data_valid(data_valid),
    .ready(ready),
    .crc_out(crc_out),
    .crc_valid(crc_valid)
);

always #5 clk = ~clk;

initial begin
    clk = 1'b0;
    data = '0;
end


task automatic process_data(input logic [DATA_WIDTH-1:0] input_data);
    data = input_data;
    data_valid = 1'b1;
    #10;
    data_valid = 1'b0;

    while(crc_valid == 1'b1) begin
        #10;
    end
    while(crc_valid == 1'b0) begin
        #10;
    end
    result_queue.push_back(crc_out);
endtask

initial begin
    reset = 1'b1;
    #10;
    reset = 1'b0;
    #10;

    process_data(8'h42);
    process_data(8'hFE);
    process_data(8'h00);
    process_data(8'h12);

    while(result_queue.size() > 0) begin
        $display("CRC: %h", result_queue.pop_front());
    end

    $finish;
end




endmodule
