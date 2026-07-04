import RiscV_32_Definitions::*;

module rv32cpu(
    input  logic                clk,
    input  logic                rst_n,

    input  logic [31 : 0]       instruction_fetch,
    output logic [31 : 2]       fetch_addr,
    input  logic                fetch_valid,

    // CPU -> Bus Request
    output logic                valid,
    output logic [ 3 : 0]       wr_strobe,
    output logic [31 : 0]       wr_data,
    output logic [31 : 2]       addr,

    // Bus -> CPU Response
    input  logic                rd_valid,
    input  logic [31 : 0]       rd_data,
    input  logic                wr_ack,
    input  logic                error
);


///////////////////////////////////////////////////////////////////////////////
// Core Registers + Pipeline Registers
///////////////////////////////////////////////////////////////////////////////
logic [31:0] x_register_file[31:0];
// TODO: CSRs

FetchStageRegs fetch_regs;        // Pipeline Stage 1 result
DecodeStageRegs decode_regs;      // Pipeline Stage 2 results
ExecuteCombinatorial exec_comb;   // Pipeline Stage 3 result
ExecuteStageRegs execute_regs;    // 
MemoryStageRegs memory_regs;      // Pipeline Stage 4 result

logic stall;
assign stall = memory_regs.mem_stall;

///////////////////////////////////////////////////////////////////////////////
// Stage 1: Instruction Fetch
///////////////////////////////////////////////////////////////////////////////
logic [31:0] next_pc;

always_comb begin
    logic [31:0] pc_plus4;

    fetch_addr = fetch_regs.fetch_pc[31:2];

    pc_plus4 = fetch_regs.fetch_pc + 32'd4;
    next_pc = exec_comb.branch_taken ? exec_comb.branch_pc : pc_plus4;
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        fetch_regs <= '0;
    end else if (!stall) begin
        fetch_regs.unaligned_pc <= next_pc[1:0] != '0;

        // Flush on branch miss, by making it a NOP
        fetch_regs.valid <= fetch_valid && !exec_comb.branch_taken;

        if (fetch_valid) begin
            fetch_regs.fetch_pc <= next_pc;
            fetch_regs.instruction <= instruction_fetch;
        end
    end
end

///////////////////////////////////////////////////////////////////////////////
// Stage 2: Decode
///////////////////////////////////////////////////////////////////////////////
logic [6:0] opcode;
logic [2:0] funct_3;
logic [6:0] funct_7;

assign opcode = fetch_regs.instruction[6:0];
assign funct_3 = fetch_regs.instruction[14:12];
assign funct_7 = fetch_regs.instruction[31:25];

logic [31:0] left_op, right_op, store_value;

// Instruction types (1 hot)
logic is_upper_instr;       // U-type (LUI, AUIPC)
logic is_jump_instr;        // J-type (JAL, JALR)
logic is_branch_instr;      // B-type
logic is_load_instr;        // I-type (LOAD)
logic is_store_instr;       // S-type
logic is_imm_alu_instr;     // I-type (ALU-imm)
logic is_reg_alu_instr;     // R-type
logic is_fence_instr;       // FENCE/FENCE.I
logic is_system_instr;      // SYSTEM (ECALL/EBREAK/CSR)
logic invalid_opcode;
logic is_nop_instruction;

// Control signals
AluControls alu_ctrl;
MemoryControls mem_ctrl;
WritebackControls wb_ctrl;
logic valid;

// Register read results
logic [31:0] rs1_reg_mux, rs2_reg_mux;
logic [31:0] immediate_bits;

// Read register file, with forwarding logic
always_comb begin
    logic [4:0] rs1_reg_select;
    logic [4:0] rs2_reg_select;
    logic [31:0] rs1_reg_read;
    logic [31:0] rs2_reg_read;

   // Read from register file
    rs1_reg_select = fetch_regs.instruction[19:15];
    rs2_reg_select = fetch_regs.instruction[24:20];
    rs1_reg_read = (rs1_reg_select == '0) ? '0 : x_register_file[rs1_reg_select];
    rs2_reg_read = (rs2_reg_select == '0) ? '0 : x_register_file[rs2_reg_select];

    // Mux with forwarded registers
    if (memory_regs.wb_ctrl.writeback_en && (memory_regs.wb_ctrl.rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = memory_regs.writeback_data;
    end else if (execute_regs.wb_ctrl.writeback_en && (execute_regs.wb_ctrl.rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = execute_regs.exec_result;
    end else begin
        rs1_reg_mux = rs1_reg_read;
    end

    if (memory_regs.wb_ctrl.writeback_en && (memory_regs.wb_ctrl.rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = memory_regs.writeback_data;
    end else if (execute_regs.wb_ctrl.writeback_en && (execute_regs.wb_ctrl.rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = execute_regs.exec_result;
    end else begin
        rs2_reg_mux = rs2_reg_read;
    end

    store_value = rs2_reg_mux;
end

// Decode instruction, set controls/operands
always_comb begin
    logic [31:0] instr;

    is_upper_instr    = 1'b0;
    is_jump_instr     = 1'b0;
    is_branch_instr   = 1'b0;
    is_load_instr     = 1'b0;
    is_store_instr    = 1'b0;
    is_imm_alu_instr  = 1'b0;
    is_reg_alu_instr  = 1'b0;
    is_fence_instr    = 1'b0;
    is_system_instr   = 1'b0;
    invalid_opcode    = 1'b0;
    is_writeback      = 1'b0;
    is_unsigned       = 1'b0;
    alu_mux_ctrl      = ALU_MUX_DEFAULT;
    left_op           = '0;
    right_op          = '0;
    immediate_bits    = '0;


    // Assemble immediate value
    instr = fetch_regs.instruction;
    case (opcode_e'(opcode))
        OPCODE_LUI:      immediate_bits = {instr[31:12], 12'h000};
        OPCODE_AUIPC:    immediate_bits = {instr[31:12], 12'h000};
        OPCODE_JAL:      immediate_bits = {{11{instr[31]}}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};
        OPCODE_JALR:     immediate_bits = {{20{instr[31]}}, instr[31:20]};
        OPCODE_BRANCH:   immediate_bits = {{19{instr[31]}}, instr[31], instr[7], instr[30:25], instr[11:8], 1'b0};
        OPCODE_LOAD:     immediate_bits = {{20{instr[31]}}, instr[31:20]};
        OPCODE_STORE:    immediate_bits = {{20{instr[31]}}, instr[31:25], instr[11:7]};
        OPCODE_IMM_ALU:  immediate_bits = {{20{instr[31]}}, instr[31:20]};
        // TODO: cleanup for immediate shifts: func7 bit shows up in immediate_bits, but is ignored in ALU
        default:         immediate_bits = '0;
    endcase

    // Decode Opcode and set controls/operands
    case (opcode_e'(opcode))
        OPCODE_LUI: begin
            is_upper_instr = 1'b1; // U-type LUI
            wb_ctrl.is_writeback = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
            left_op  = '0;
            right_op = immediate_bits;
        end
        OPCODE_AUIPC: begin
            is_upper_instr = 1'b1; // U-type AUIPC
            wb_ctrl.is_writeback = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            left_op  = fetch_regs.fetch_pc;
            right_op = immediate_bits;
        end
        OPCODE_JAL: begin
            is_jump_instr = 1'b1; // J-type JAL
            wb_ctrl.is_writeback = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            left_op  = '0;
            right_op = '0;
        end
        OPCODE_JALR: begin
            is_jump_instr = 1'b1; // J-type JALR
            wb_ctrl.is_writeback = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            left_op  = '0;
            right_op = '0;
        end
        OPCODE_BRANCH: begin
            is_branch_instr = 1'b1; // B-type Branch
            alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
            left_op  = rs1_reg_mux;
            right_op = rs2_reg_mux;
            alu_ctrl.is_unsigned = (branch_funct_3_e'(funct_3) == BRCH_BLTU) || 
                                   (branch_funct_3_e'(funct_3) == BRCH_BGEU);
        end
        OPCODE_LOAD: begin
            wb_ctrl.writeback_en = 1'b1;
            is_load_instr = 1'b1; // I-type LOAD
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
        end
        OPCODE_STORE: begin
            is_store_instr = 1'b1; // S-type STORE
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
        end
        OPCODE_IMM_ALU: begin
            is_imm_alu_instr = 1'b1; // I-type ALU
            wb_ctrl.writeback_en = 1'b1;
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
            alu_ctrl.is_unsigned = (alu_immediate_e'(funct_3) == ALU_SLTIU);
            case (alu_immediate_e'(funct_3))
                ALU_ADDI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
                ALU_SLLI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_SLTI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_SLTIU: alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_XORI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_SRLI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_ORI:   alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_ANDI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                default:   alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            endcase
        end
        OPCODE_REG_ALU: begin
            is_reg_alu_instr = 1'b1; // R-type ALU
            wb_ctrl.writeback_en = 1'b1;
            left_op  = rs1_reg_mux;
            right_op = rs2_reg_mux;
            alu_ctrl.is_unsigned = (alu_register_e'(funct_3) == ALU_SLTU);
            case (alu_register_e'(funct_3))
                ALU_ADD:  alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
                ALU_SLL:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_SLT:  alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_SLTU: alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_XOR:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_SRL:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_OR:   alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_AND:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                default:  alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            endcase
        end
        OPCODE_FENCE: begin
            is_fence_instr = 1'b1; // FENCE, FENCE.I
            left_op  = '0; // TODO
            right_op = '0;
        end
        OPCODE_SYSTEM: begin
            is_system_instr = 1'b1; // ECALL/EBREAK/CSR
            left_op  = '0; // TODO
            right_op = '0;
        end
        default: begin
            invalid_opcode = 1'b1;
            left_op  = '0;
            right_op = '0;
        end
    endcase

    // Pipeline flush: make it NOP
    is_nop_instruction = fetch_regs.nop_instruction || exec_comb.branch_taken || invalid_opcode;
end

// TODO: handle stalls / flushes (branch taken)
always_ff @(posedge clk) begin
    if (!rst_n) begin
        decode_regs <= '0;
    end else if (!stall) begin
        // Data path
        decode_regs.left_operand                 <= left_op;
        decode_regs.right_operand                <= right_op;
        decode_regs.branch_immediate             <= immediate_bits;
        decode_regs.fetch_pc                     <= fetch_regs.fetch_pc;
        decode_regs.store_value                  <= store_value;

        // Control path
        decode_regs.funct_3                      <= funct_3;
        decode_regs.alu_ctrl.funct_7_bit5        <= funct_7[5];
        decode_regs.alu_ctrl.is_unsigned         <= is_unsigned;
        decode_regs.jump_branch_ctrl.is_jump_instr   <= is_jump_instr;
        decode_regs.jump_branch_ctrl.is_branch_instr <= is_branch_instr;

        decode_regs.mem_ctrl.memory_request      <= is_store_instr || is_load_instr;
        decode_regs.mem_ctrl.memory_write        <= is_store_instr;

        decode_regs.wb_ctrl.writeback_en         <= is_writeback;
        decode_regs.wb_ctrl.rd_reg_select        <= fetch_regs.instruction[11:7];

        decode_regs.invalid_opcode               <= invalid_opcode;
        decode_regs.nop_instruction              <= is_nop_instruction;
        decode_regs.valid                        <= valid;
    end else begin
        decode_regs <= decode_regs;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 3: Execute
///////////////////////////////////////////////////////////////////////////////
logic [31:0] alu_left, alu_right;

assign alu_left = decode_regs.left_operand;
assign alu_right = decode_regs.right_operand;

logic [31:0] logic_result, shifter_result, adder_result;
logic negative_flag;
logic compare_result;

// Shifter and Logic units
always_comb begin
    logic [1:0] alu_logic_ctrl;
    logic [0:0] alu_shift_ctrl;
    logic is_arithmetic;

    is_arithmetic = decode_regs.alu_ctrl.funct_7_bit5 == FUNCT_7_SRA;
    alu_logic_ctrl = decode_regs.funct_3[1:0];
    alu_shift_ctrl = decode_regs.funct_3[2];

    case (alu_logic_e'(alu_logic_ctrl))
        LOGIC_XOR: logic_result = decode_regs.left_operand ^ decode_regs.right_operand;
        LOGIC_LUI: logic_result = decode_regs.right_operand;
        LOGIC_OR:  logic_result = decode_regs.left_operand | decode_regs.right_operand;
        LOGIC_AND: logic_result = decode_regs.left_operand & decode_regs.right_operand;
        default:   logic_result = 'x;
    endcase

    case (alu_shift_e'(alu_shift_ctrl))
        SHIFT_LEFT:  shifter_result = alu_left << alu_right[4:0];
        SHIFT_RIGHT: shifter_result = is_arithmetic ? ($signed(alu_left) >>> alu_right[4:0]) : (alu_left >> alu_right[4:0]);
        default:     shifter_result = 'x;
    endcase
end

// Datapath Adder 
always_comb begin
    logic is_subtract;
    logic is_unsigned;
    logic [32:0] adder_left, adder_right, carry_in, adder_result;  // 1 extra bit for carry-out

    is_subtract = decode_regs.alu_ctrl.funct_7_bit5 == FUNCT_7_SUB;
    is_unsigned = decode_regs.alu_ctrl.is_unsigned;

    // Datapath Adder
    adder_left  = {(is_unsigned ? 1'b0 : alu_left[31]), alu_left};
    adder_right = {(is_unsigned ? 1'b0 : alu_right[31]), alu_right};
    carry_in = {32'b0, is_subtract};
    adder_result = adder_left + (is_subtract ? ~adder_right : adder_right) + carry_in;
    negative_flag = adder_result[32];
end

// Comparator and PC Adder
always_comb begin
    logic [1:0] cmp_ctrl;
    logic equal_flag;
    logic branch_taken;

    cmp_ctrl = decode_regs.funct_3[1:0];
    equal_flag = alu_left == alu_right;

    case (cmp_sign_e'(cmp_ctrl))
        CMP_EQ:  compare_result = equal_flag;
        CMP_NE:  compare_result = ~equal_flag;
        CMP_LT:  compare_result = negative_flag;
        CMP_GE:  compare_result = ~negative_flag;
        default: compare_result = 'x;
    endcase

    // PC Adder -- Read by earlier stages
    branch_taken = decode_regs.jump_branch_ctrl.is_branch_instr && compare_result;
    exec_comb.branch_taken = branch_taken || decode_regs.jump_branch_ctrl.is_jump_instr;
    exec_comb.branch_pc = decode_regs.fetch_pc + decode_regs.branch_immediate;
end

// ALU Mux
always_comb begin
    logic [31:0] alu_result;
    logic [1:0] alu_mux_ctrl;
    alu_mux_ctrl = decode_regs.funct_3[1:0];

    case (alu_mux_e'(alu_mux_ctrl))
        ALU_MUX_LOGIC:   alu_result = logic_result;
        ALU_MUX_ADDER:   alu_result = adder_result;
        ALU_MUX_SHIFT:   alu_result = shifter_result;
        ALU_MUX_COMPARE: alu_result = {31'b0, compare_result};
        default:         alu_result = 'x;
    endcase
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        execute_regs <= '0;
    end else if (!stall) begin
        execute_regs.exec_result <= alu_result;

        // Forward unmodified rs2 for store instructions, unmodified PC for JAL/JALR
        execute_regs.store_value <= decode_regs.store_value;
        execute_regs.fetch_pc <= decode_regs.fetch_pc;

        execute_regs.funct_3  <= decode_regs.funct_3;
        execute_regs.jump_branch_ctrl <= decode_regs.nop_instruction ? '0 : decode_regs.jump_branch_ctrl;
        execute_regs.mem_ctrl <= decode_regs.nop_instruction ? '0 : decode_regs.mem_ctrl;
        execute_regs.wb_ctrl  <= decode_regs.nop_instruction ? '0 : decode_regs.wb_ctrl;
    end else begin
        execute_regs <= execute_regs;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 4: Memory Access
///////////////////////////////////////////////////////////////////////////////
logic [3:0] byte_lanes;
logic [31:0] shifted_wr_data, shifted_rd_data;
logic mem_stall;

always_comb begin
    logic zero_extend;

    zero_extend = execute_regs.funct_3[2];

    case (mem_funct_3_e'(execute_regs.funct_3))
        MEM_BYTE : begin
            byte_lanes = 1'b0001 << execute_regs.exec_result[1:0];
            shifted_wr_data = execute_regs.store_value << (execute_regs.exec_result[1:0] * 8);
        end
        MEM_HALF : begin
            byte_lanes = (execute_regs.exec_result[1]) ? 4'b1100 : 4'b0011;
            shifted_wr_data = execute_regs.store_value << (execute_regs.exec_result[1] ? 16 : 0);
        end
        MEM_WORD : begin
            byte_lanes = 4'b1111;
            shifted_wr_data = execute_regs.store_value;
        end
        default: begin
            byte_lanes = 4'b0000;
            shifted_wr_data = '0;
        end
        // TODO: unaligned access traps
    endcase

    case (byte_lanes)
        4'b0001 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[7:0]}   : {{24{rd_data[7]}}, rd_data[7:0]};
        4'b0010 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[15:8]}  : {{24{rd_data[15]}}, rd_data[15:8]};
        4'b0100 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[23:16]} : {{24{rd_data[23]}}, rd_data[23:16]};
        4'b1000 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[31:24]} : {{24{rd_data[31]}}, rd_data[31:24]};
        4'b0011 : shifted_rd_data = zero_extend ? {16'h0000, rd_data[15:0]}    : {{16{rd_data[31]}}, rd_data[15:0]};
        4'b1100 : shifted_rd_data = zero_extend ? {16'h0000, rd_data[31:16]}   : {{16{rd_data[31]}}, rd_data[31:16]};
        4'b1111 : shifted_rd_data = rd_data;
        default : shifted_rd_data = rd_data;
    endcase     

    mem_stall = (execute_regs.mem_ctrl.memory_write && !wr_ack) || (execute_regs.mem_ctrl.memory_request && !rd_valid);
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        valid <= 1'b0;
        wr_strobe <= 4'h0;
        memory_regs <= '0;
    end else begin  
        // TODO: if other stall sources are added, this block ignores them!
        // TOOD: assumes 1-cycle access, never stalls! Check rd_valid and wr_ack!
        memory_regs.mem_stall <= 1'b0;
    
        valid <= execute_regs.mem_ctrl.memory_request;
        wr_data <= shifted_wr_data;
        addr <= execute_regs.exec_result[31:2];
        wr_strobe <= execute_regs.mem_ctrl.memory_write ? byte_lanes : '0;

        memory_regs.writeback_data <= execute_regs.mem_ctrl.memory_request ? shifted_rd_data : execute_regs.exec_result;
        memory_regs.jump_branch_ctrl <= execute_regs.jump_branch_ctrl;
        memory_regs.wb_ctrl <= execute_regs.wb_ctrl;
        memory_regs.pc_plus4 <= execute_regs.fetch_pc + 32'd4; // JAL/JALR need PC+4
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 5: Writeback
///////////////////////////////////////////////////////////////////////////////
always_ff @(posedge clk) begin
    automatic logic [31:0] data;
    data = memory_regs.jump_branch_ctrl.is_jump_instr ? memory_regs.pc_plus4 : memory_regs.writeback_data;

    if (!stall && memory_regs.wb_ctrl.writeback_en) begin
        if (memory_regs.wb_ctrl.rd_reg_select != '0) begin
            x_register_file[memory_regs.wb_ctrl.rd_reg_select] <= data;
        end
    end
end


endmodule