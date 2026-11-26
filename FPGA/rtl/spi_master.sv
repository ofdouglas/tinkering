module spi_master (
    input  logic clk, 
    input  logic rst_n,

    // User Interface
    input  logic [7:0] data_in,
    input  logic valid_in,
    output logic in_ready,
    output logic [7:0] data_out,
    output logic valid_out,

    // SPI bus
    output logic sck,
    output logic mosi, 
    input  logic miso
);

localparam int   DATA_BITS = 8;
localparam int   CLK_DIV_BITS = 10;
localparam int   SHIFT_COUNT_BITS = $clog2(DATA_BITS) + 1;

// 50 MHz / 50 --> 1 MHz base clk (500 kHz SPI sck)
localparam logic [CLK_DIV_BITS-1:0] kClkDivMaxValue = CLK_DIV_BITS'(49);

logic [CLK_DIV_BITS-1:0] sck_div_counter;
logic                    sck_en_strobe;
logic                    sck_phase_bit;
logic                    transfer_active;
logic [SHIFT_COUNT_BITS-1:0] shift_counter;
logic [DATA_BITS-1:0]        rx_shift_reg, tx_shift_reg;

// Input synchronizer (metastability)
logic [1:0] miso_sync;

assign in_ready = !transfer_active;

always_ff @(posedge clk) begin
    if (!rst_n) begin
        // Internal registers
        sck_div_counter <= '0;
        sck_en_strobe   <= '0;
        sck_phase_bit   <= '0;
        transfer_active <= '0;
        shift_counter   <= '0;
        rx_shift_reg    <= '0;
        tx_shift_reg    <= '0;
        miso_sync       <= '0;
        // Registered i/o -- User
        data_out        <= '0;
        valid_out       <= '0;
        // Registered i/o -- SPI bus
        sck             <= '0;
        mosi            <= '0;
    end else begin
        sck_en_strobe   <= '0;
        valid_out       <= '0;

        miso_sync <= {miso, miso_sync[1]};

        if (sck_div_counter == kClkDivMaxValue) begin
            sck_div_counter <= '0;
            sck_en_strobe   <= 1'b1;
        end else begin
            sck_div_counter <= sck_div_counter + 1'b1;
        end

        if (!transfer_active && valid_in) begin
            transfer_active <= 1'b1;
            sck_phase_bit   <= 1'b0;
            shift_counter   <= '0;
            tx_shift_reg    <= data_in;
        end

        if (transfer_active && sck_en_strobe) begin
            sck_phase_bit <= !sck_phase_bit;

            if (sck_phase_bit == '0) begin
                sck           <= '0;
                mosi          <= tx_shift_reg[DATA_BITS-1];
                tx_shift_reg  <= {tx_shift_reg[DATA_BITS-2:0], 1'b0};
            end else begin
                if (shift_counter == (DATA_BITS)) begin
                    data_out         <= rx_shift_reg;
                    valid_out        <= '1;
                    transfer_active  <= '0;
                    sck              <= '0;
                end else begin
                    sck              <= '1;
                    rx_shift_reg     <= {rx_shift_reg[DATA_BITS-2:0], miso_sync[0]};
                    shift_counter    <= shift_counter + 1'b1;
                end
            end
        end
    end
end

endmodule
