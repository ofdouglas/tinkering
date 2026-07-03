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
    output logic [31 : 2] addr,

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
    fetch_addr = fetch_regs.fetch_pc[31:2];

    logic [31:0] pc_plus4 = fetch_regs.fetch_pc + 3'b100;
    next_pc = exec_comb.branch_taken ? exec_comb.branch_pc : pc_plus4;
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        fetch_regs <= '0;
    end else if (!stall) begin
        fetch_regs.instruction <= instruction_fetch;
        fetch_regs.unaligned_pc <= next_pc[1:0] != '0;

        fetch_regs.valid <= fetch_valid;
        if (fetch_valid) begin
            fetch_regs.fetch_pc <= next_pc;

            // Flush on branch miss, by making it a NOP
            fetch_regs.nop_instruction <= exec_comb.branch_taken;
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

// Miscellaneous controls
logic swap_operands;
logic is_subtract;
logic is_unsigned;
logic is_writeback;

// Register read results
logic [31:0] rs1_reg_output, rs2_reg_output;

always_comb begin
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
    swap_operands     = 1'b0;
    immediate_bits    = '0;

    // Determine instruction type
    case (opcode_e'(opcode))
        OPCODE_LUI:      is_upper_instr    = 1'b1; // U-type LUI
        OPCODE_AUIPC:    is_upper_instr    = 1'b1; // U-type AUIPC
        OPCODE_JAL:      is_jump_instr     = 1'b1; // J-type JAL
        OPCODE_JALR:     is_jump_instr     = 1'b1; // J-type JALR
        OPCODE_BRANCH:   is_branch_instr   = 1'b1; // B-type Branch
        OPCODE_LOAD:     is_load_instr     = 1'b1; // I-type LOAD
        OPCODE_STORE:    is_store_instr    = 1'b1; // S-type STORE
        OPCODE_IMM_ALU:  is_imm_alu_instr  = 1'b1; // I-type ALU
        OPCODE_REG_ALU:  is_reg_alu_instr  = 1'b1; // R-type ALU
        OPCODE_FENCE:    is_fence_instr    = 1'b1; // FENCE, FENCE.I
        OPCODE_SYSTEM:   is_system_instr   = 1'b1; // ECALL/EBREAK/CSR
        default:         invalid_opcode    = 1'b1;
    endcase

    // Assemble immediate value
    logic [31:0] instr = fetch_regs.instruction;
    case (opcode_e'(opcode))
        OPCODE_LUI:      immediate_bits = {instr[31:12], 12'h000};
        OPCODE_AUIPC:    immediate_bits = {instr[31:12], 12'h000};
        OPCODE_JAL:      immediate_bits = {12{instr[31]}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};
        OPCODE_JALR:     immediate_bits = {20{instr[31]}, instr[31:20]};
        OPCODE_BRANCH:   immediate_bits = {19{instr[31]}, instr[31], instr[7], instr[30:25], instr[11:8], 1'b0};
        OPCODE_LOAD:     immediate_bits = {20{instr[31]}, instr[31:20]};
        OPCODE_STORE:    immediate_bits = {20{instr[31]}, instr[31:25], instr[11:7]};
        OPCODE_IMM_ALU:  immediate_bits = {20{instr[31]}, instr[31:20]};
        // TODO: cleanup for immediate shifts: func7 bit shows up in immediate_bits, but is ignored in ALU
        default:         immediate_bits = '0;
    endcase

    // SUB opcode or branch
    is_sub = funct_7[5] || is_branch_instr;

    // BLTU, BGEU or SLTU
    is_unsigned = is_branch_instr ? (funct_3[2:1] == 2'b11) : (funct_3[2:0] == 3'b011);

    // BGE, BGEU
    swap_operands = is_branch_instr && (funct_3[2:1] == 2'b11);

    is_writeback = is_reg_alu_instr || is_imm_alu_instr || is_load_instr || is_upper_instr || is_jump_instr; // TODO: fence

    // Pipeline flush: make it NOP
    is_nop_instruction = fetch_regs.nop_instruction || exec_comb.branch_taken || invalid_opcode;

    // Read from register file
    logic [4:0] rs1_reg_select = fetch_regs.instruction[19:15];
    logic [4:0] rs2_reg_select = fetch_regs.instruction[24:20];
    logic [31:0] rs1_reg_read = (rs1_reg_select == '0) ? '0 : x_register_file[rs1_reg_select];
    logic [31:0] rs2_reg_read = (rs2_reg_select == '0) ? '0 : x_register_file[rs2_reg_select];

    // Mux with forwarded registers
    logic [31:0] rs1_reg_mux, rs2_reg_mux;

    if (memory_regs.wb_ctrl.writeback_en && (memory_regs.wb_ctrl.rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = memory_regs.writeback_data;
    end else if (execute_regs.wb_ctrl.writeback_en && (execute_regs.wb_ctrl.rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = execute_regs.writeback_data;
    end else begin
        rs1_reg_mux = rs1_reg_read;
    end

    if (memory_regs.wb_ctrl.writeback_en && (memory_regs.wb_ctrl.rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = memory_regs.writeback_data;
    end else if (execute_regs.wb_ctrl.writeback_en && (execute_regs.wb_ctrl.rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = execute_regs.writeback_data;
    end else begin
        rs2_reg_mux = rs2_reg_read;
    end

    store_value = rs2_reg_mux;

    // Assign ALU left and right operands
    case (opcode_e'(opcode))
        OPCODE_LUI:      begin // U-type LUI:  exec = imm
            left_op  = '0;
            right_op = immediate_bits;
        end 
        OPCODE_AUIPC:    begin // U-type AUIPC  uses address adder
            left_op  = '0;
            right_op = '0;
        end 
        OPCODE_JAL:      begin // J-type JAL    uses address adder
            left_op  = '0;
            right_op = '0;
        end 
        OPCODE_JALR:     begin // J-type JALR   uses address adder
            left_op  = '0;
            right_op = '0;
        end 
        OPCODE_BRANCH:   begin // B-type Branch exec = ((rs1 - rs2) < 0) ? 1 : 0
            // Swap operands for BGE, BGEU
            left_op  = swap_operands ? rs2_reg_mux : rs1_reg_mux;
            right_op = swap_operands ? rs1_reg_mux : rs2_reg_mux;
        end 
        OPCODE_LOAD:     begin // I-type LOAD:  addr = rs1 + imm
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
        end 
        OPCODE_STORE:    begin // S-type STORE: addr = rs1 + imm
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
        end 
        OPCODE_IMM_ALU:  begin // I-type ALU:   exec = rs1 + imm
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
        end 
        OPCODE_REG_ALU:  begin // R-type ALU:   exec = rs1 + rs2
            left_op  = rs1_reg_mux;
            right_op = rs2_reg_mux;
        end 
        OPCODE_FENCE:    begin // FENCE, FENCE.I
            left_op  = '0;  // TODO
            right_op = '0;
        end 
        OPCODE_SYSTEM:   begin // ECALL/EBREAK/CSR
            left_op  = '0;  // TODO
            right_op = '0;
        end 
    endcase
end

// TODO: handle stalls / flushes (branch taken)
always_ff @(posedge clk) begin
    if (!rst_n) begin
        decode_regs <= '0;
    end else if (!stall) begin
        // Data path
        decode_regs.left_operand                 <= left_op;
        decode_regs.right_operand                <= right_op;
        decode_regs.branch_immediate             <= immediate_bits;         // Used for AUIPC
        decode_regs.fetch_pc                     <= fetch_regs.fetch_pc;
        decode_regs.store_value                  <= store_value;

        // Control path
        decode_regs.alu_ctrl.is_subtract         <= is_subtract;
        decode_regs.alu_ctrl.is_unsigned         <= is_unsigned;
        decode_regs.alu_ctrl.is_branch_instr     <= is_branch_instr;

        decode_regs.mem_ctrl.memory_request      <= is_store_instr || is_load_instr;
        decode_regs.mem_ctrl.memory_write        <= is_store_instr;

        decode_regs.wb_ctrl.writeback_en         <= is_writeback;
        decode_regs.wb_ctrl.rd_reg_select        <= fetch_regs.instruction[11:7];
        decode_regs.wb_ctrl.is_jump_instr        <= is_jump_instr;

        decode_regs.invalid_opcode               <= invalid_opcode;
        decode_regs.nop_instruction              <= is_nop_instruction;
    end else begin
        decode_regs <= decode_regs;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 3: Execute
///////////////////////////////////////////////////////////////////////////////
logic [31:0] alu_result, pc_result;

always_comb begin
    logic zero_extend = decode_regs.alu_ctrl.is_unsigned;
    logic [31:0] alu_left = decode_regs.left_operand;
    logic [31:0] alu_right = decode_regs.right_operand;
    logic is_sub = decode_regs.alu_ctrl.is_subtract;

    logic [31:0] shifter_result;
    logic [32:0] adder_left, adder_right, adder_result;  // 1 extra bit for carry-out

    // Datapath Adder
    adder_left  = {(zero_extend ? 1'b0 : alu_left[31]), alu_left};
    adder_right = {(zero_extend ? 1'b0 : alu_right[31]), alu_right};
    adder_result = adder_left + (is_sub ? ~adder_right : adder_right) + is_sub;
    logic negative_flag = adder_result[32];
    logic zero_flag = adder_result[31:0] == '0;

    // Barrel shifter
    case (alu_funct_3_e'(decode_regs.funct_3))
        ALU_SLL:     shifter_result = alu_left << alu_right[4:0];
        ALU_SRL_SRA: shifter_result = is_sub ? ($signed(alu_left) >>> alu_right[4:0]) : (alu_left >> alu_right[4:0]);
        default:     shifter_result = '0;
    endcase

    // ALU output mux. TODO optimize by computing controls in decode stage
    if (decode_regs.alu_ctrl.is_branch_instr) begin
        case (alu_branch_funct_3_e'(decode_regs.funct_3))
            ALU_BEQ      : alu_result = {'0, zero_flag};
            ALU_BNE      : alu_result = {'0, ~zero_flag};
            ALU_BLT      : alu_result = {'0, negative_flag};
            ALU_BGE      : alu_result = {'0, negative_flag}; // Operands swapped in stage 2
            ALU_BLTU     : alu_result = {'0, negative_flag}; // TODO
            ALU_BGEU     : alu_result = {'0, negative_flag}; // TODO
        endcase
    end else if (decode_regs.mem_ctrl.memory_request) begin
        alu_result = adder_result[31:0];
    end else if (decode_regs.alu_ctrl.is_lui_instr) begin
        alu_result = decode_regs.right_operand;
    end else begin
        case (alu_funct_3_e'(decode_regs.funct_3))
            ALU_ADD_SUB  : alu_result = adder_result[31:0];
            ALU_SLL      : alu_result = shifter_result;
            ALU_SLT      : alu_result = {31'b0, negative_flag};      // TODO check this
            ALU_SLTU     : alu_result = {31'b0, negative_flag};      // TODO check this
            ALU_XOR      : alu_result = alu_left ^ alu_right;
            ALU_SRL_SRA  : alu_result = shifter_result;
            ALU_OR       : alu_result = alu_left | alu_right;
            ALU_AND      : alu_result = alu_left & alu_right;
        endcase
    end

    // PC Adder -- Read by earlier stages
    logic branch_taken = decode_regs.alu_ctrl.is_branch_instr && alu_result[0];
    exec_comb.branch_taken = branch_taken || decode_regs.wb_ctrl.is_jump_instr;
    exec_comb.branch_pc = decode_regs.fetch_pc + decode_regs.branch_immediate;
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        execute_regs <= '0;
    end else if (!stall) begin
        // TODO: clean this up, shorten exec stage critical path
        execute_regs.exec_result <= decode_regs.alu_ctrl.is_auipc_instr ? exec_comb.branch_pc : alu_result;

        // Forward unmodified rs2 for store instructions, unmodified PC for JAL/JALR
        execute_regs.store_value <= decode_regs.store_value;
        execute_regs.fetch_pc <= decode_regs.fetch_pc;

        execute_regs.funct_3  <= decode_regs.funct_3;
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
    logic zero_extend = execute_regs.funct_3[2];

    case (mem_funct_3_e'(execute_regs.funct_3)) begin
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

    case (byte_lanes) begin
        4'b0001 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[7:0]}   : {24{rd_data[7]}, rd_data[7:0]};
        4'b0010 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[15:8]}  : {24{rd_data[15]}, rd_data[15:8]};
        4'b0100 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[23:16]} : {24{rd_data[23]}, rd_data[23:16]};
        4'b1000 : shifted_rd_data = zero_extend ? {24'h000000, rd_data[31:24]} : {24{rd_data[31]}, rd_data[31:24]};
        4'b0011 : shifted_rd_data = zero_extend ? {16'h0000, rd_data[15:0]}    : {16{rd_data[31]}, rd_data[15:0]};
        4'b1100 : shifted_rd_data = zero_extend ? {16'h0000, rd_data[31:16]}   : {16{rd_data[31]}, rd_data[31:16]};
        4'b1111 : shifted_rd_data = rd_data;
        default : shifted_rd_data = rd_data;
    endcase     

    mem_stall = (execute_regs.mem_ctrl.memory_write && !wr_ack) || (execute_regs.mem_ctrl.memory_request && !rd_valid);
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        valid <= 1'b0;
        wr_strobe <= 4'h0;
        mem_stall <= 1'b0;
        memory_regs <= '0;
    end else if (!stall) begin
        wr_strobe <= execute_regs.mem_ctrl.memory_write ? byte_lanes : '0;
        valid <= execute_regs.mem_ctrl.memory_request && !memory_regs.mem_stall; // TODO: check this
        addr <= execute_regs.exec_result;
        wr_data <= shifted_wr_data;

        memory_regs.writeback_data <= execute_regs.mem_ctrl.memory_request ? shifted_rd_data : execute_regs.exec_result;
        memory_regs.wb_ctrl <= execute_regs.wb_ctrl;
        memory_regs.pc_plus4 <= execute_regs.fetch_pc + 3'b100; // JAL/JALR need PC+4

        memory_regs.mem_stall <= mem_stall;
    end else begin
        memory_regs <= memory_regs;
        memory_regs.mem_stall <= mem_stall;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 5: Writeback
///////////////////////////////////////////////////////////////////////////////
always_ff @(posedge clk) begin
    logic [31:0] data = memory_regs.wb_ctrl.is_jump_instr ? memory_regs.pc_plus4 : memory_regs.writeback_data;

    if (memory_regs.wb_ctrl.writeback_en && (memory_regs.wb_ctrl.rd_reg_select != '0)) begin
        x_register_file[memory_regs.wb_ctrl.rd_reg_select] <= data;
    end
end


endmodule