module crc #(
    parameter int WIDTH = 8,
    parameter logic [WIDTH-1:0] POLYNOMIAL = 'h1D,
    parameter logic [WIDTH-1:0] INITIAL    = 'hFF,
    parameter logic [WIDTH-1:0] FINAL_XOR  = 'hFF
)(
    
    input logic clk,
    input logic reset,

    input logic [WIDTH-1:0] data,
    input logic data_valid,
    input logic initial_data,

    output logic ready,
    output logic [WIDTH-1:0] crc_out,
    output logic crc_valid
);

typedef enum logic[1:0] {
    STATE_INVALID = 2'b00,
    STATE_BUSY    = 2'b01,
    STATE_VALID   = 2'b10
} state_e;


logic [WIDTH-1:0]       shift_register;
logic [$clog2(WIDTH):0] shifts_remaining;
state_e state;

always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
        state            <= STATE_INVALID;
        shift_register   <= INITIAL;
        shifts_remaining <= WIDTH;
        crc_out          <= '0;
        crc_valid        <= 1'b0;
        ready            <= 1'b0;
    end else begin
        crc_valid        <= 1'b0;

        case(state)
            STATE_INVALID: begin
                if (data_valid) begin
                    state            <= STATE_BUSY;
                    shift_register   <= data ^ (initial_data ? INITIAL : shift_register);
                    shifts_remaining <= WIDTH;
                    ready            <= 1'b0;
                end else begin
                    ready            <= 1'b1;
                end
            end

            STATE_BUSY: begin
                ready <= 1'b0;

                if (shifts_remaining > '0) begin
                    shifts_remaining <= shifts_remaining - 1;

                    if (shift_register[WIDTH-1] == 1'b1) begin
                        shift_register <= {shift_register[WIDTH-2:0], 1'b0} ^ POLYNOMIAL;
                    end else begin
                        shift_register <= {shift_register[WIDTH-2:0], 1'b0};
                    end
                end else begin
                    state <= STATE_VALID;
                end
            end

            STATE_VALID: begin
                ready     <= 1'b1;
                crc_valid <= 1'b1;
                crc_out   <= shift_register[WIDTH-1:0] ^ FINAL_XOR;

                if (data_valid) begin
                    state            <= STATE_BUSY;
                    shift_register   <= data ^ INITIAL;
                    shifts_remaining <= WIDTH;
                    ready            <= 1'b0;
                end
            end
        endcase
    end
end

endmodule
