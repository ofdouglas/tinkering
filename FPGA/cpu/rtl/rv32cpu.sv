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
// assign stall = memory_regs.mem_stall;
assign stall = 1'b0;

///////////////////////////////////////////////////////////////////////////////
// Stage 1: Instruction Fetch
///////////////////////////////////////////////////////////////////////////////
logic [31:0] next_pc, pc_plus4;

// Address generation
always_comb begin
    fetch_addr = fetch_regs.current_pc[31:2];

    pc_plus4 = fetch_regs.current_pc + 32'd4;
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
            fetch_regs.current_pc  <= next_pc;
            fetch_regs.fetch_pc    <= fetch_regs.current_pc;
            fetch_regs.instruction <= instruction_fetch;
        end
    end
end

///////////////////////////////////////////////////////////////////////////////
// Stage 2: Decode
///////////////////////////////////////////////////////////////////////////////
// Instruction fields
logic [31:0] instr;
logic [6:0] opcode;
logic [6:0] funct_7;
logic [2:0] funct_3;

assign instr   = fetch_regs.instruction;
assign opcode  = fetch_regs.instruction[6:0];
assign funct_7 = fetch_regs.instruction[31:25];
assign funct_3 = fetch_regs.instruction[14:12];

// Datapath results
logic [31:0] rs1_reg_mux, rs2_reg_mux, immediate_bits, left_op, right_op, store_value;

// Control signals
AluControls        alu_ctrl;
JumpBranchControls jump_branch_ctrl;
MemoryControls     mem_ctrl;
WritebackControls  wb_ctrl;
logic              invalid_opcode;

// Read register file, with forwarding logic
always_comb begin
    logic [4:0] rs1_reg_select, rs2_reg_select, mem_rd_reg_select, wb_rd_reg_select;
    logic mem_valid, wb_valid;

    rs1_reg_select = instr[19:15];
    rs2_reg_select = instr[24:20];
    mem_rd_reg_select = memory_regs.wb_ctrl.rd_reg_select;
    wb_rd_reg_select = execute_regs.wb_ctrl.rd_reg_select;
    mem_valid = memory_regs.valid && memory_regs.wb_ctrl.writeback_en;
    wb_valid = execute_regs.valid && execute_regs.wb_ctrl.writeback_en;

    if (rs1_reg_select == '0) begin
        rs1_reg_mux = '0;
    end else if (mem_valid && (mem_rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = memory_regs.writeback_data;
    end else if (wb_valid && (wb_rd_reg_select == rs1_reg_select)) begin
        rs1_reg_mux = execute_regs.exec_result;
    end else begin
        rs1_reg_mux = x_register_file[rs1_reg_select];
    end

    if (rs2_reg_select == '0) begin
        rs2_reg_mux = '0;
    end else if (mem_valid && (mem_rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = memory_regs.writeback_data;
    end else if (wb_valid && (wb_rd_reg_select == rs2_reg_select)) begin
        rs2_reg_mux = execute_regs.exec_result;
    end else begin
        rs2_reg_mux = x_register_file[rs2_reg_select];
    end
end

// Assemble immediate value
always_comb begin
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
        default:         immediate_bits = 'x;
    endcase
end

// Decode instruction, set controls/operands
always_comb begin
    invalid_opcode    = 1'b0;
    alu_ctrl          = '0;
    alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
    jump_branch_ctrl  = '0;
    mem_ctrl          = '0;
    wb_ctrl           = '0;
    wb_ctrl.rd_reg_select = instr[11:7];
    left_op           = 'x;
    right_op          = 'x;

    // Decode Opcode and set controls/operands
    case (opcode_e'(opcode))
        OPCODE_LUI: begin
            left_op  = 'x;
            right_op = immediate_bits;
            wb_ctrl.writeback_en = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
            alu_ctrl.is_lui_instr = 1'b1;
        end
        OPCODE_AUIPC: begin
            left_op  = fetch_regs.fetch_pc;
            right_op = immediate_bits;
            wb_ctrl.writeback_en = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
        end
        OPCODE_JAL: begin
            left_op  = 'x;
            right_op = 'x;
            jump_branch_ctrl.is_jump_instr = 1'b1;
            wb_ctrl.writeback_en = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
        end
        OPCODE_JALR: begin
            // JALR uses the PC Adder; re-use left op to save pipeline space
            left_op  = rs1_reg_mux;
            right_op = 'x;
            jump_branch_ctrl.is_jump_instr = 1'b1;
            jump_branch_ctrl.is_jump_register_instr = 1'b1;
            wb_ctrl.writeback_en = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
        end
        OPCODE_BRANCH: begin
            left_op  = rs1_reg_mux;
            right_op = rs2_reg_mux;
            jump_branch_ctrl.is_branch_instr = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
            alu_ctrl.is_unsigned = (branch_funct_3_e'(funct_3) == BRCH_BLTU) || 
                                   (branch_funct_3_e'(funct_3) == BRCH_BGEU);
            alu_ctrl.adder_ctrl = ADDER_CTRL_SUB;
        end
        OPCODE_LOAD: begin
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
            wb_ctrl.writeback_en = 1'b1;
            mem_ctrl.memory_request = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
        end
        OPCODE_STORE: begin
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
            mem_ctrl.memory_request = 1'b1;
            mem_ctrl.memory_write = 1'b1;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
        end
        OPCODE_IMM_ALU: begin
            left_op  = rs1_reg_mux;
            right_op = immediate_bits;
            wb_ctrl.writeback_en  = 1'b1;
            alu_ctrl.is_unsigned  = alu_immediate_e'(funct_3) == ALU_SLTIU;
            alu_ctrl.shift_sign_ctrl = funct_7[5] ? SHIFTER_CTRL_SRA : SHIFTER_CTRL_SRL;

            case (alu_immediate_e'(funct_3))
                ALU_ADDI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
                ALU_SLLI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_SLTI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_SLTIU: alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_XORI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_SRAI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_ORI:   alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                ALU_ANDI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
                default:   alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            endcase
        end
        OPCODE_REG_ALU: begin
            left_op  = rs1_reg_mux;
            right_op = rs2_reg_mux;
            wb_ctrl.writeback_en = 1'b1;
            alu_ctrl.is_unsigned = (alu_register_e'(funct_3) == ALU_SLTU);
            alu_ctrl.adder_ctrl = ((alu_register_e'(funct_3) == ALU_ADD) && (funct_7[5] == 1'b0))
                                   ? ADDER_CTRL_ADD : ADDER_CTRL_SUB;
            alu_ctrl.shift_sign_ctrl = funct_7[5] ? SHIFTER_CTRL_SRA : SHIFTER_CTRL_SRL;

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
            left_op  = 'x; // TODO
            right_op = 'x;
        end
        OPCODE_SYSTEM: begin
            left_op  = 'x; // TODO
            right_op = 'x;
        end
        default: begin
            invalid_opcode = 1'b1;
            left_op  = 'x;
            right_op = 'x;
        end
    endcase
end

always_ff @(posedge clk) begin
    logic flush_instruction;
    flush_instruction = exec_comb.branch_taken || invalid_opcode;

    if (!rst_n) begin
        decode_regs <= '0;
    end else if (!stall) begin
        // Data path
        decode_regs.left_operand                 <= left_op;
        decode_regs.right_operand                <= right_op;
        decode_regs.current_pc                   <= fetch_regs.fetch_pc;
        decode_regs.branch_immediate             <= immediate_bits;
        decode_regs.store_value                  <= rs2_reg_mux;

        // Control path
        decode_regs.valid                        <= fetch_regs.valid && !flush_instruction;
        decode_regs.funct_3                      <= funct_3;
        decode_regs.alu_ctrl                     <= alu_ctrl;
        decode_regs.jump_branch_ctrl             <= jump_branch_ctrl;
        decode_regs.mem_ctrl                     <= mem_ctrl;
        decode_regs.wb_ctrl                      <= wb_ctrl;
        decode_regs.invalid_opcode               <= invalid_opcode;
    end else begin
        decode_regs <= decode_regs;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 3: Execute
///////////////////////////////////////////////////////////////////////////////
logic [31:0] alu_left, alu_right;
logic [31:0] logic_result, shifter_result, adder_sum;
logic negative_flag;
logic compare_result;

assign alu_left = decode_regs.left_operand;
assign alu_right = decode_regs.right_operand;

// Shifter and Logic units
always_comb begin
    alu_logic_e logic_ctrl;
    logic is_arithmetic;

    is_arithmetic = decode_regs.alu_ctrl.shift_sign_ctrl == SHIFTER_CTRL_SRA;
    logic_ctrl =  decode_regs.alu_ctrl.is_lui_instr ? LOGIC_LUI : alu_logic_e'(decode_regs.funct_3[1:0]);

    case (alu_logic_e'(logic_ctrl))
        LOGIC_XOR: logic_result = decode_regs.left_operand ^ decode_regs.right_operand;
        LOGIC_LUI: logic_result = decode_regs.right_operand;
        LOGIC_OR:  logic_result = decode_regs.left_operand | decode_regs.right_operand;
        LOGIC_AND: logic_result = decode_regs.left_operand & decode_regs.right_operand;
        default:   logic_result = 'x;
    endcase

    case (alu_shift_e'(decode_regs.funct_3[2]))
        SHIFT_LEFT: begin
            shifter_result = alu_left << alu_right[4:0];
        end
        SHIFT_RIGHT: begin
            if (is_arithmetic) begin
                shifter_result = $signed(alu_left) >>> alu_right[4:0];
            end else begin
                shifter_result = alu_left >> alu_right[4:0];
            end
        end
        default: begin
            shifter_result = 'x;
        end
    endcase
end

// Datapath Adder 
always_comb begin
    logic is_subtract;
    logic is_unsigned;
    logic [32:0] adder_left, adder_right, carry_in, adder_result;  // 1 extra bit for carry-out

    is_subtract = decode_regs.alu_ctrl.adder_ctrl == ADDER_CTRL_SUB;
    is_unsigned = decode_regs.alu_ctrl.is_unsigned;

    // Datapath Adder
    adder_left  = {(is_unsigned ? 1'b0 : alu_left[31]), alu_left};
    adder_right = {(is_unsigned ? 1'b0 : alu_right[31]), alu_right};
    carry_in = {32'b0, is_subtract};
    adder_result = adder_left + (is_subtract ? ~adder_right : adder_right) + carry_in;
    negative_flag = adder_result[32];

    adder_sum = adder_result[31:0];
end

// Comparator and PC Adder / Branch Decision (read by earlier stages)
always_comb begin
    logic [31:0] base_address, address_sum;
    cmp_type_e cmp_type;
    logic equal_flag;
    logic less_flag;
    logic branch_taken;

    cmp_type = decode_regs.jump_branch_ctrl.is_branch_instr
        ? cmp_type_e'({decode_regs.funct_3[2], decode_regs.funct_3[0]})
        : CMP_LT;
    equal_flag = alu_left == alu_right;
    less_flag = decode_regs.alu_ctrl.is_unsigned
        ? (alu_left < alu_right)
        : ($signed(alu_left) < $signed(alu_right));

    case (cmp_type)
        CMP_EQ:  compare_result = equal_flag;
        CMP_NE:  compare_result = ~equal_flag;
        CMP_LT:  compare_result = less_flag;
        CMP_GE:  compare_result = ~less_flag;
        default: compare_result = 'x;
    endcase

    // JALR uses rs1 + immediate; all other branching uses PC + immediate
    base_address = decode_regs.jump_branch_ctrl.is_jump_register_instr ? alu_left[31:0] : decode_regs.current_pc[31:0];
    address_sum = base_address + decode_regs.branch_immediate[31:0];
    exec_comb.branch_pc = {address_sum[31:1], 1'b0};

    branch_taken = decode_regs.jump_branch_ctrl.is_branch_instr && compare_result;
    exec_comb.branch_taken = decode_regs.valid && (branch_taken || decode_regs.jump_branch_ctrl.is_jump_instr);
end

// ALU Result
logic [31:0] alu_result;
always_comb begin
    case (alu_mux_e'(decode_regs.alu_ctrl.alu_mux_ctrl))
        ALU_MUX_LOGIC:   alu_result = logic_result;
        ALU_MUX_ADDER:   alu_result = adder_sum;
        ALU_MUX_SHIFT:   alu_result = shifter_result;
        ALU_MUX_COMPARE: alu_result = {31'b0, compare_result};
        default:         alu_result = 'x;
    endcase
end

// ALU Mux and Pipeline Registers
always_ff @(posedge clk) begin
    if (!rst_n) begin
        execute_regs <= '0;
    end else if (!stall && decode_regs.valid) begin
        execute_regs.exec_result <= alu_result;

        // Forward unmodified rs2 for store instructions, unmodified PC for JAL/JALR
        execute_regs.store_value <= decode_regs.store_value;
        execute_regs.current_pc <= decode_regs.current_pc;

        execute_regs.valid <= 1'b1;
        execute_regs.funct_3  <= decode_regs.funct_3;
        execute_regs.jump_branch_ctrl <= decode_regs.jump_branch_ctrl;
        execute_regs.mem_ctrl <= decode_regs.mem_ctrl;
        execute_regs.wb_ctrl  <= decode_regs.wb_ctrl;
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
    mem_size_e mem_size;
    logic      zero_extend;
    logic      is_load;
    logic      is_store;

    mem_size = mem_size_e'(execute_regs.funct_3[1:0]);
    zero_extend = (execute_regs.funct_3[2] == LOAD_UNSIGNED);
    is_load = execute_regs.mem_ctrl.memory_request;
    is_store = execute_regs.mem_ctrl.memory_write;

    mem_stall = (is_store && !wr_ack) || (is_load && !rd_valid);

    case (mem_size)
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
            shifted_wr_data = 'x;
        end
        // TODO: unaligned access traps
    endcase

    case (byte_lanes)
        4'b0001 : shifted_rd_data = zero_extend ? {24'd0, rd_data[7:0]}   : {{24{rd_data[7]}}, rd_data[7:0]};
        4'b0010 : shifted_rd_data = zero_extend ? {24'd0, rd_data[15:8]}  : {{24{rd_data[15]}}, rd_data[15:8]};
        4'b0100 : shifted_rd_data = zero_extend ? {24'd0, rd_data[23:16]} : {{24{rd_data[23]}}, rd_data[23:16]};
        4'b1000 : shifted_rd_data = zero_extend ? {24'd0, rd_data[31:24]} : {{24{rd_data[31]}}, rd_data[31:24]};
        4'b0011 : shifted_rd_data = zero_extend ? {16'd0, rd_data[15:0]}    : {{16{rd_data[31]}}, rd_data[15:0]};
        4'b1100 : shifted_rd_data = zero_extend ? {16'd0, rd_data[31:16]}   : {{16{rd_data[31]}}, rd_data[31:16]};
        4'b1111 : shifted_rd_data = rd_data;
        default : shifted_rd_data = 'x;
    endcase
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
    
        // Disable Load/Store unit for now
        valid <= 1'b0;
        // valid <= execute_regs.valid && execute_regs.mem_ctrl.memory_request;
        wr_data <= shifted_wr_data;
        addr <= execute_regs.exec_result[31:2];
        wr_strobe <= execute_regs.mem_ctrl.memory_write ? byte_lanes : '0;

        memory_regs.valid <= execute_regs.valid; // TODO
        memory_regs.writeback_data <= execute_regs.mem_ctrl.memory_request ? shifted_rd_data : execute_regs.exec_result;
        memory_regs.jump_branch_ctrl <= execute_regs.jump_branch_ctrl;
        memory_regs.wb_ctrl <= execute_regs.wb_ctrl;
        memory_regs.pc_plus4 <= execute_regs.current_pc + 32'd4; // JAL/JALR need PC+4
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 5: Writeback
///////////////////////////////////////////////////////////////////////////////
always_ff @(posedge clk) begin
    logic [31:0] data;
    data = memory_regs.jump_branch_ctrl.is_jump_instr ? memory_regs.pc_plus4 : memory_regs.writeback_data;

    if (!stall && memory_regs.valid && memory_regs.wb_ctrl.writeback_en) begin
        if (memory_regs.wb_ctrl.rd_reg_select != '0) begin
            x_register_file[memory_regs.wb_ctrl.rd_reg_select] <= data;
        end
    end
end


endmodule