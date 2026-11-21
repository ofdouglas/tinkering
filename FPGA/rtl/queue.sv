module queue #(parameter WIDTH = 8, parameter DEPTH = 4) (
    input  logic clk,
    input  logic rst_n,

    // Input port
    input  logic [WIDTH-1:0] in_data,
    input  logic in_valid,
    output logic in_ready,

    // Output port
    output logic [WIDTH-1:0] out_data,
    output logic out_valid,
    input  logic out_strobe
);

/******************************************************************************
 * Registers
 *****************************************************************************/

// Queue storage
logic [WIDTH-1:0] array [DEPTH];

localparam int kMaxIndex = DEPTH-1;
localparam int kIndexBits = DEPTH > 1 ? $clog2(DEPTH) : 1;
localparam int kNumItemsBits = DEPTH > 1 ? $clog2(DEPTH+1) : 1;

// Next index to write to or read from
logic [kIndexBits-1:0] write_index, read_index;

// Register tracking current number of items
logic [kNumItemsBits-1:0] num_items;

/******************************************************************************
 * Logic
 *****************************************************************************/

logic push, pop;
assign in_ready  = num_items < DEPTH;
assign out_valid = num_items != 0;
assign push      = in_ready && in_valid;
assign pop       = out_strobe && out_valid;
assign out_data  = array[read_index];

function automatic logic [kIndexBits-1:0] increment_index(logic [kIndexBits-1:0] index);
    return (index == kMaxIndex) ? '0 : (index + 1'b1);
endfunction

always_ff @(posedge clk) begin
    if (!rst_n) begin
        write_index <= '0;
        read_index  <= '0;
        num_items   <= '0;
    end else begin
        if (pop) begin
            read_index <= increment_index(read_index);
        end

        if (push) begin
            array[write_index] <= in_data;
            write_index <= increment_index(write_index);
        end

        case ({push, pop})
            2'b10:   num_items <= num_items + 1'b1;
            2'b01:   num_items <= num_items - 1'b1;
            default: ; // Num items is unchanged
        endcase
    end
end

endmodule
