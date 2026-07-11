import RiscV_32_Definitions::*;

module rv32cpu #(
    parameter logic [31:0] RESET_PC = 32'h0000_0000
) (
    input  logic                clk,
    input  logic                rst_n,

    input  logic [31 : 0]       instruction_fetch,
    output logic [31 : 2]       fetch_addr,
    input  logic                fetch_valid,
    input  logic                ext_irq,

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
// Core Registers, Special Registers, and Pipeline Registers
///////////////////////////////////////////////////////////////////////////////
logic [31:0]       x_register_file[31:0];
MachineSpecialRegs machine_special_regs;
privilege_mode_e   privilege_mode;

FetchStageRegs       fetch_regs;    // Pipeline Stage 1 result
DecodeStageRegs      decode_regs;   // Pipeline Stage 2 results
ExecuteStageRegs     execute_regs;  // Pipeline Stage 3 results
MemoryStageRegs      memory_regs;   // Pipeline Stage 4 result

///////////////////////////////////////////////////////////////////////////////
// Control Signals (Combinatorial)
///////////////////////////////////////////////////////////////////////////////
CpuControl         cpu_ctrl;

logic external_interrupt;
assign external_interrupt = ext_irq &&
                            machine_special_regs.mstatus.mie &&
                            machine_special_regs.mie.mei_enable;

logic exception_entry;
assign exception_entry = cpu_ctrl.decode_trap || external_interrupt || cpu_ctrl.illegal_instruction;

///////////////////////////////////////////////////////////////////////////////
// Stage 1: Instruction Fetch
///////////////////////////////////////////////////////////////////////////////
logic [31:0] next_pc, pc_plus4;
logic instruction_flush;
logic branch_redirect_taken;


assign instruction_flush = external_interrupt || cpu_ctrl.decode_trap || cpu_ctrl.exception_return || branch_redirect_taken;

// Address generation
always_comb begin
    fetch_addr = fetch_regs.current_pc[31:2];
    pc_plus4 = fetch_regs.current_pc + 32'd4;

    if (exception_entry) begin
        next_pc = {machine_special_regs.mtvec.base, 2'b00};
    end else if (cpu_ctrl.exception_return) begin
        next_pc = machine_special_regs.mepc;
    end else if (branch_redirect_taken) begin
        next_pc = cpu_ctrl.branch_pc;
    end else begin
        next_pc = pc_plus4;
    end
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        fetch_regs <= '0;
        fetch_regs.current_pc <= RESET_PC;
        fetch_regs.fetch_pc <= RESET_PC;
    end else if (!cpu_ctrl.decode_flush && !cpu_ctrl.mem_stall) begin
        fetch_regs.unaligned_pc <= next_pc[1:0] != '0;

        fetch_regs.valid <= fetch_valid && !instruction_flush;

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
logic [4:0] rs1_reg_select, rs2_reg_select;
logic [6:0] funct_7;
logic [2:0] funct_3;
logic [11:0] csr_address, funct_12;

assign instr   = fetch_regs.instruction;
assign opcode  = fetch_regs.instruction[6:0];
assign rs1_reg_select = instr[19:15];
assign rs2_reg_select = instr[24:20];
assign funct_7 = fetch_regs.instruction[31:25];
assign funct_3 = fetch_regs.instruction[14:12];
assign csr_address = fetch_regs.instruction[31:20];
assign funct_12 = fetch_regs.instruction[31:20];

// Datapath results
logic [31:0] rs1_reg_mux, rs2_reg_mux, immediate_bits, csr_read;

// Control signals
AluControls        alu_ctrl;
JumpBranchControls jump_branch_ctrl;
MemoryControls     mem_ctrl;
WritebackControls  wb_ctrl;
logic              csr_instruction;
logic              csr_wb_hazard;
logic              csr_wb_stall; // Stall on repeated access to same CSR
logic              invalid_csr_read_address;
logic              invalid_csr_write_address;

assign csr_instruction = (opcode == OPCODE_SYSTEM && funct_3 != system_funct3_e'(SYS3_OTHER));

assign csr_wb_hazard = (decode_regs.valid && decode_regs.wb_ctrl.csr_writeback_en && decode_regs.wb_ctrl.csr_address == csr_address) ||
                       (execute_regs.valid && execute_regs.wb_ctrl.csr_writeback_en && execute_regs.wb_ctrl.csr_address == csr_address) ||
                       // TODO: get rid of this, CSR writes retire in Memory Stage
                       (memory_regs.valid && memory_regs.wb_ctrl.csr_writeback_en && memory_regs.wb_ctrl.csr_address == csr_address); 

assign csr_wb_stall = csr_wb_hazard && csr_instruction;

// Get instruction values
always_comb begin
    // Read register file
    logic mem_wb_hazard;
    logic load_use_hazard;
    mem_wb_hazard = memory_regs.valid && memory_regs.wb_ctrl.writeback_en && memory_regs.wb_ctrl.rd_reg_select != '0;

    if (rs1_reg_select == '0) begin
        rs1_reg_mux = '0;
    end else if (mem_wb_hazard && memory_regs.wb_ctrl.rd_reg_select == rs1_reg_select) begin
        rs1_reg_mux = memory_regs.writeback_data;
    end else begin
        rs1_reg_mux = x_register_file[rs1_reg_select];
    end

    if (rs2_reg_select == '0) begin
        rs2_reg_mux = '0;
    end else if (mem_wb_hazard && memory_regs.wb_ctrl.rd_reg_select == rs2_reg_select) begin
        rs2_reg_mux = memory_regs.writeback_data;
    end else begin
        rs2_reg_mux = x_register_file[rs2_reg_select];
    end

    if (decode_regs.valid && decode_regs.mem_ctrl.memory_request) begin
        load_use_hazard = !decode_regs.mem_ctrl.memory_write &&
            (decode_regs.wb_ctrl.rd_reg_select == rs1_reg_select || decode_regs.wb_ctrl.rd_reg_select == rs2_reg_select);
    end else begin
        load_use_hazard = 1'b0;
    end

    cpu_ctrl.decode_flush = load_use_hazard || csr_wb_stall;

    // Assemble immediate value
    case (opcode_e'(opcode))
        OPCODE_LUI:      immediate_bits = {instr[31:12], 12'h000};
        OPCODE_AUIPC:    immediate_bits = {instr[31:12], 12'h000};
        OPCODE_JAL:      immediate_bits = {{11{instr[31]}}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};
        OPCODE_JALR:     immediate_bits = {{20{instr[31]}}, instr[31:20]};
        OPCODE_BRANCH:   immediate_bits = {{19{instr[31]}}, instr[31], instr[7], instr[30:25], instr[11:8], 1'b0};
        OPCODE_LOAD:     immediate_bits = {{20{instr[31]}}, instr[31:20]};
        OPCODE_STORE:    immediate_bits = {{20{instr[31]}}, instr[31:25], instr[11:7]};
        OPCODE_IMM_ALU:  immediate_bits = {{20{instr[31]}}, instr[31:20]};
        OPCODE_SYSTEM:   immediate_bits = {27'd0, rs1_reg_select[4:0]};
        // TODO: cleanup for immediate shifts: func7 bit shows up in immediate_bits, but is ignored in ALU
        default:         immediate_bits = 'x;
    endcase
end

// Read CSRs (Always read, ignored if not a CSR instruction)
always_comb begin
    logic [3:0] csr_class_bits;
    logic [7:0] csr_offset_bits;

    csr_class_bits   = instr[31:28];
    csr_offset_bits = instr[27:20];
    csr_read = '0;
    invalid_csr_read_address = 1'b0;

    case (csr_class_e'(csr_class_bits))
        CSR_CLASS_MACHINE_RW: begin
            case (csr_machine_rw_offset_e'(csr_offset_bits))
                CSR_ADDRESS_MSTATUS: csr_read = read_mstatus(machine_special_regs.mstatus);
                CSR_ADDRESS_MISA:    csr_read = MISA_VALUE;
                CSR_ADDRESS_MIE:     csr_read = read_mie(machine_special_regs.mie);
                CSR_ADDRESS_MIP:     csr_read = read_mip(machine_special_regs.mip);
                CSR_ADDRESS_MTVEC:   csr_read = machine_special_regs.mtvec;
                CSR_ADDRESS_MEPC:    csr_read = machine_special_regs.mepc;
                CSR_ADDRESS_MCAUSE:  csr_read = machine_special_regs.mcause;
                CSR_ADDRESS_MTVAL:   csr_read = machine_special_regs.mtval;
                default:             invalid_csr_read_address = 1'b1;
            endcase
        end
        CSR_CLASS_MACHINE_AUX_RW: begin
            // Not implemented yet
            invalid_csr_read_address = 1'b1;
        end
        CSR_CLASS_MACHINE_TIME: begin
            // Not implemented yet
            invalid_csr_read_address = 1'b1;
        end
        CSR_CLASS_MACHINE_INFO: begin
            case (csr_machine_info_offset_e'(csr_offset_bits))
                CSR_ADDRESS_MVENDORID:  csr_read = 32'h00000000;
                CSR_ADDRESS_MARCHID:    csr_read = 32'h000000FD;
                CSR_ADDRESS_MIMPID:     csr_read = 32'h00000001;
                CSR_ADDRESS_MHARTID:    csr_read = 32'h00000000;
                CSR_ADDRESS_MCONFIGPTR: csr_read = 32'h00000000;
                default:                invalid_csr_read_address = 1'b1;
            endcase
        end
        default: begin
            invalid_csr_read_address = 1'b1;
        end
    endcase
end

// Decode instruction, set controls/operands
always_comb begin
    cpu_ctrl.illegal_instruction = 1'b0;
    cpu_ctrl.invalid_opcode    = 1'b0;
    cpu_ctrl.decode_trap       = 1'b0;
    cpu_ctrl.exception_return  = 1'b0;
    alu_ctrl          = '0;
    alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
    alu_ctrl.alu_left_src = ALU_LEFT_SRC_DONT_CARE;
    alu_ctrl.alu_right_src = ALU_RIGHT_SRC_DONT_CARE;
    jump_branch_ctrl  = '0;
    mem_ctrl          = '0;
    wb_ctrl           = '0;
    wb_ctrl.rd_reg_select = instr[11:7];
    wb_ctrl.csr_address = instr[31:20];

    // Decode Opcode and set controls/operands
    case (opcode_e'(opcode))
        OPCODE_LUI: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_DONT_CARE;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_IMM;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;
            alu_ctrl.logic_ctrl = LOGIC_LUI;
            wb_ctrl.writeback_en = 1'b1;
        end
        OPCODE_AUIPC: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_PC;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_IMM;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            wb_ctrl.writeback_en = 1'b1;
        end
        OPCODE_JAL: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_DONT_CARE;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_DONT_CARE;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            jump_branch_ctrl.is_jump_instr = 1'b1;
            wb_ctrl.writeback_en = 1'b1;
        end
        OPCODE_JALR: begin
            // JALR uses the PC Adder; re-use left op to save pipeline space
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_DONT_CARE;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            jump_branch_ctrl.is_jump_instr = 1'b1;
            jump_branch_ctrl.is_jump_register_instr = 1'b1;
            wb_ctrl.writeback_en = 1'b1;
        end
        OPCODE_BRANCH: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_RS2;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
            alu_ctrl.is_unsigned = (branch_funct_3_e'(funct_3) == BRCH_BLTU) || 
                                   (branch_funct_3_e'(funct_3) == BRCH_BGEU);
            alu_ctrl.adder_ctrl = ADDER_CTRL_SUB;
            jump_branch_ctrl.is_branch_instr = 1'b1;
        end
        OPCODE_LOAD: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_IMM;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
            wb_ctrl.writeback_en = 1'b1;
            mem_ctrl.memory_request = 1'b1;
        end
        OPCODE_STORE: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_IMM;
            alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
            mem_ctrl.memory_request = 1'b1;
            mem_ctrl.memory_write = 1'b1;
        end
        OPCODE_IMM_ALU: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_IMM;
            alu_ctrl.is_unsigned  = alu_immediate_e'(funct_3) == ALU_SLTIU;
            alu_ctrl.shift_sign_ctrl = funct_7[5] ? SHIFTER_CTRL_SRA : SHIFTER_CTRL_SRL;
            wb_ctrl.writeback_en  = 1'b1;

            case (alu_immediate_e'(funct_3))
                ALU_ADDI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
                ALU_SLLI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_SLTI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_SLTIU: alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_XORI:  begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_XOR;
                end
                ALU_SRAI:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_ORI:   begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_OR;
                end
                ALU_ANDI:  begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_AND;
                end
                default:   alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            endcase
        end
        OPCODE_REG_ALU: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_RS1;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_RS2;
            alu_ctrl.is_unsigned = (alu_register_e'(funct_3) == ALU_SLTU);
            alu_ctrl.adder_ctrl = ((alu_register_e'(funct_3) == ALU_ADD) && (funct_7[5] == 1'b0))
                                   ? ADDER_CTRL_ADD : ADDER_CTRL_SUB;
            alu_ctrl.shift_sign_ctrl = funct_7[5] ? SHIFTER_CTRL_SRA : SHIFTER_CTRL_SRL;
            wb_ctrl.writeback_en = 1'b1;

            case (alu_register_e'(funct_3))
                ALU_ADD:  alu_ctrl.alu_mux_ctrl = ALU_MUX_ADDER;
                ALU_SLL:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_SLT:  alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_SLTU: alu_ctrl.alu_mux_ctrl = ALU_MUX_COMPARE;
                ALU_XOR:  begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_XOR;
                end
                ALU_SRL:  alu_ctrl.alu_mux_ctrl = ALU_MUX_SHIFT;
                ALU_OR:   begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_OR;
                end
                ALU_AND:  begin
                    alu_ctrl.alu_mux_ctrl = ALU_MUX_LOGIC;   
                    alu_ctrl.logic_ctrl = LOGIC_AND;
                end
                default:  alu_ctrl.alu_mux_ctrl = ALU_MUX_DEFAULT;
            endcase
        end
        OPCODE_FENCE: begin
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_DONT_CARE;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_DONT_CARE;
            cpu_ctrl.invalid_opcode = 1'b1; // Not implemented yet
        end
        OPCODE_SYSTEM: begin
            case (system_funct3_e'(funct_3))
                SYS3_OTHER: begin
                    case (system_funct12_e'(funct_12))
                        SYS12_MRET: begin
                            cpu_ctrl.exception_return = 1'b1;
                        end
                        SYS12_WFI: begin
                            cpu_ctrl.invalid_opcode = 1'b1;
                        end
                        SYS12_ECALL: begin
                            cpu_ctrl.decode_trap = 1'b1;
                        end
                        SYS12_EBREAK: begin
                            cpu_ctrl.invalid_opcode = 1'b1;
                        end
                        default: begin
                            cpu_ctrl.invalid_opcode = 1'b1;
                        end
                    endcase
                end
                SYS3_CSRRW: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_RS1;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_ZERO;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_OR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_writeback_en = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                end
                SYS3_CSRRC: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_RS1;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_CSR;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_CLEAR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                    wb_ctrl.csr_writeback_en = (rs1_reg_select == '0) ? 1'b0 : 1'b1;
                end
                SYS3_CSRRS: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_RS1;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_CSR;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_OR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                    wb_ctrl.csr_writeback_en = (rs1_reg_select == '0) ? 1'b0 : 1'b1;
                end
                SYS3_CSRRWI: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_IMM;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_ZERO;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_OR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                    wb_ctrl.csr_writeback_en = (rs1_reg_select == '0) ? 1'b0 : 1'b1;
                end
                SYS3_CSRRSI: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_IMM;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_CSR;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_OR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                    wb_ctrl.csr_writeback_en = (rs1_reg_select == '0) ? 1'b0 : 1'b1;
                end
                SYS3_CSRRCI: begin
                    alu_ctrl.alu_left_src    = ALU_LEFT_SRC_IMM;
                    alu_ctrl.alu_right_src   = ALU_RIGHT_SRC_CSR;
                    alu_ctrl.alu_mux_ctrl    = ALU_MUX_LOGIC;
                    alu_ctrl.logic_ctrl      = LOGIC_CLEAR;
                    wb_ctrl.writeback_en     = 1'b1;
                    wb_ctrl.csr_read         = 1'b1;
                    wb_ctrl.csr_writeback_en = (rs1_reg_select == '0) ? 1'b0 : 1'b1;
                end
                default: begin
                    cpu_ctrl.invalid_opcode = 1'b1;
                end
            endcase
        end
        default: begin
            cpu_ctrl.invalid_opcode = 1'b1;
            alu_ctrl.alu_left_src  = ALU_LEFT_SRC_DONT_CARE;
            alu_ctrl.alu_right_src = ALU_RIGHT_SRC_DONT_CARE;
        end
    endcase

    cpu_ctrl.illegal_instruction = cpu_ctrl.invalid_opcode && fetch_regs.valid && !branch_redirect_taken;
end

always_ff @(posedge clk) begin
    logic flush_instruction;
    flush_instruction = branch_redirect_taken || cpu_ctrl.illegal_instruction;

    if (!rst_n) begin
        decode_regs <= '0;
    end else if (cpu_ctrl.mem_stall) begin
        decode_regs <= decode_regs;
    end else if (cpu_ctrl.decode_flush || cpu_ctrl.invalid_opcode) begin
        decode_regs <= '0;
    end else begin
        // Data path
        decode_regs.rs1_reg                <= rs1_reg_mux;
        decode_regs.rs2_reg                <= rs2_reg_mux;
        decode_regs.current_pc             <= fetch_regs.fetch_pc;
        decode_regs.immediate              <= immediate_bits;
        decode_regs.rs1_index              <= rs1_reg_select;
        decode_regs.rs2_index              <= rs2_reg_select;
        decode_regs.csr_read               <= csr_read;
        decode_regs.csr_instruction        <= csr_instruction;

        // Control path
        decode_regs.valid                  <= fetch_regs.valid && !flush_instruction;
        decode_regs.funct_3                <= funct_3;
        decode_regs.alu_ctrl               <= alu_ctrl;
        decode_regs.jump_branch_ctrl       <= jump_branch_ctrl;
        decode_regs.mem_ctrl               <= mem_ctrl;
        decode_regs.wb_ctrl                <= wb_ctrl;
    end
end

///////////////////////////////////////////////////////////////////////////////
// Stage 3: Execute
///////////////////////////////////////////////////////////////////////////////
logic [31:0] alu_left, alu_right, rs2_store_value;
logic [31:0] logic_result, shifter_result, adder_sum, alu_result;
logic negative_flag;
logic compare_result;

// Operand muxing
always_comb begin
    logic ex_regs_wb, mem_regs_wb;
    ex_regs_wb  = execute_regs.valid && execute_regs.wb_ctrl.writeback_en && execute_regs.wb_ctrl.rd_reg_select != '0;
    mem_regs_wb = memory_regs.valid && memory_regs.wb_ctrl.writeback_en && memory_regs.wb_ctrl.rd_reg_select != '0;

    // ALU Left Operand
    case (decode_regs.alu_ctrl.alu_left_src)
        ALU_LEFT_SRC_RS1: begin
            if (ex_regs_wb && (execute_regs.wb_ctrl.rd_reg_select == decode_regs.rs1_index)) begin
                alu_left = execute_regs.exec_result;
            end else if (mem_regs_wb && (memory_regs.wb_ctrl.rd_reg_select == decode_regs.rs1_index)) begin
                alu_left = memory_regs.writeback_data;
            end else begin
                alu_left = decode_regs.rs1_reg;
            end
        end
        ALU_LEFT_SRC_PC:  begin
            alu_left = decode_regs.current_pc;
        end
        ALU_LEFT_SRC_IMM: begin
            alu_left = decode_regs.immediate;
        end
        default: alu_left = 'x;
    endcase

    // ALU Right Operand
    case (decode_regs.alu_ctrl.alu_right_src)
        ALU_RIGHT_SRC_RS2: begin
            if (ex_regs_wb && (execute_regs.wb_ctrl.rd_reg_select == decode_regs.rs2_index)) begin
                alu_right = execute_regs.exec_result;
            end else if (mem_regs_wb && (memory_regs.wb_ctrl.rd_reg_select == decode_regs.rs2_index)) begin
                alu_right = memory_regs.writeback_data;
            end else begin
                alu_right = decode_regs.rs2_reg;
            end
        end
        ALU_RIGHT_SRC_IMM: begin
            alu_right = decode_regs.immediate;
        end
        ALU_RIGHT_SRC_CSR: begin
            alu_right = decode_regs.csr_read;
        end
        default: alu_right = 'x;
    endcase

    // rs2 for store instructions and CSR read
    if (decode_regs.csr_instruction) begin
        rs2_store_value = decode_regs.csr_read;
    end else if (ex_regs_wb && (execute_regs.wb_ctrl.rd_reg_select == decode_regs.rs2_index)) begin
        rs2_store_value = execute_regs.exec_result;
    end else if (mem_regs_wb && (memory_regs.wb_ctrl.rd_reg_select == decode_regs.rs2_index)) begin
        rs2_store_value = memory_regs.writeback_data;
    end else begin
        rs2_store_value = decode_regs.rs2_reg;
    end
end

// Shifter and Logic units
always_comb begin
    logic is_arithmetic;
    is_arithmetic = decode_regs.alu_ctrl.shift_sign_ctrl == SHIFTER_CTRL_SRA;

    case (alu_logic_e'(decode_regs.alu_ctrl.logic_ctrl))
        LOGIC_XOR:   logic_result = alu_left ^ alu_right;
        LOGIC_LUI:   logic_result = decode_regs.immediate;
        LOGIC_OR:    logic_result = alu_left | alu_right;
        LOGIC_AND:   logic_result = alu_left & alu_right;
        LOGIC_CLEAR: logic_result = ~alu_left & alu_right;
        default:     logic_result = 'x;
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
    address_sum = base_address + decode_regs.immediate[31:0];
    cpu_ctrl.branch_pc = {address_sum[31:1], 1'b0};

    branch_taken = decode_regs.jump_branch_ctrl.is_branch_instr && compare_result;
    branch_redirect_taken = decode_regs.valid && (branch_taken || decode_regs.jump_branch_ctrl.is_jump_instr);
end

// ALU Result
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
    end else if (!cpu_ctrl.mem_stall) begin
        execute_regs.exec_result      <= alu_result;
        // Forward unmodified rs2 for store instructions, unmodified PC for JAL/JALR
        execute_regs.rs2_csr_reg      <= rs2_store_value;
        execute_regs.current_pc       <= decode_regs.current_pc;
        execute_regs.rs2_index        <= decode_regs.rs2_index;

        execute_regs.valid            <= decode_regs.valid;
        execute_regs.funct_3          <= decode_regs.funct_3;
        execute_regs.jump_branch_ctrl <= decode_regs.jump_branch_ctrl;
        execute_regs.mem_ctrl         <= decode_regs.mem_ctrl;
        execute_regs.wb_ctrl          <= decode_regs.wb_ctrl;
    end else begin
        execute_regs <= execute_regs;
    end
end


///////////////////////////////////////////////////////////////////////////////
// Stage 4A: Memory Access
///////////////////////////////////////////////////////////////////////////////
logic [3:0] byte_lanes;
logic [31:0] shifted_wr_data, shifted_rd_data;

assign valid = execute_regs.valid && execute_regs.mem_ctrl.memory_request;
assign wr_data = shifted_wr_data;
assign addr = execute_regs.exec_result[31:2];
assign wr_strobe = (execute_regs.valid && execute_regs.mem_ctrl.memory_write) ? byte_lanes : '0;

always_comb begin
    logic [31:0] rs2_reg_mux;
    logic  mem_wb_hazard;
    mem_size_e mem_size;
    logic      zero_extend;
    logic      is_load;
    logic      is_store;

    mem_wb_hazard = memory_regs.valid && memory_regs.wb_ctrl.writeback_en && memory_regs.wb_ctrl.rd_reg_select != '0;
    if (mem_wb_hazard && memory_regs.wb_ctrl.rd_reg_select == execute_regs.rs2_index) begin
        rs2_reg_mux = memory_regs.writeback_data;
    end else begin
        rs2_reg_mux = execute_regs.rs2_csr_reg;
    end

    mem_size = mem_size_e'(execute_regs.funct_3[1:0]);
    zero_extend = (execute_regs.funct_3[2] == LOAD_UNSIGNED);
    is_store = execute_regs.valid && execute_regs.mem_ctrl.memory_write;
    is_load = execute_regs.valid && execute_regs.mem_ctrl.memory_request && !is_store;

    cpu_ctrl.mem_stall = (is_store && !wr_ack) || (is_load && !rd_valid);
    cpu_ctrl.mem_unaligned = 1'b0; // TODO:
    cpu_ctrl.bus_error = 1'b0; // TODO:

    case (mem_size)
        MEM_BYTE : begin
            byte_lanes = 1'b0001 << execute_regs.exec_result[1:0];
            shifted_wr_data = rs2_reg_mux << (execute_regs.exec_result[1:0] * 8);
        end
        MEM_HALF : begin
            byte_lanes = (execute_regs.exec_result[1]) ? 4'b1100 : 4'b0011;
            shifted_wr_data = rs2_reg_mux << (execute_regs.exec_result[1] ? 16 : 0);
        end
        MEM_WORD : begin
            byte_lanes = 4'b1111;
            shifted_wr_data = rs2_reg_mux;
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
        4'b0011 : shifted_rd_data = zero_extend ? {16'd0, rd_data[15:0]}    : {{16{rd_data[15]}}, rd_data[15:0]};
        4'b1100 : shifted_rd_data = zero_extend ? {16'd0, rd_data[31:16]}   : {{16{rd_data[31]}}, rd_data[31:16]};
        4'b1111 : shifted_rd_data = rd_data;
        default : shifted_rd_data = 'x;
    endcase
end

always_ff @(posedge clk) begin
    if (!rst_n) begin
        memory_regs <= '0;
    end else if (!cpu_ctrl.mem_stall) begin
        memory_regs.writeback_data <= (execute_regs.mem_ctrl.memory_request && !execute_regs.mem_ctrl.memory_write) ? shifted_rd_data : execute_regs.exec_result;
        memory_regs.csr_read_data     <= execute_regs.rs2_csr_reg;
        memory_regs.pc_plus4    <= execute_regs.current_pc + 32'd4; // JAL/JALR need PC+4

        memory_regs.valid <= execute_regs.valid;
        memory_regs.jump_branch_ctrl <= execute_regs.jump_branch_ctrl;
        memory_regs.wb_ctrl <= execute_regs.wb_ctrl;
    end
end

///////////////////////////////////////////////////////////////////////////////
// Stage 4B: Special Registers
///////////////////////////////////////////////////////////////////////////////
logic [31:0] mcause_data;
logic [31:0] mtval_data;

// Determine trap type for mcause and trap value for mtval
always_comb begin
    // TODO: handle other trap types
    if (cpu_ctrl.decode_trap) begin
        mcause_data = write_mcause(0, TRAP_ECALL_M);
        mtval_data = '0;
    end else if (cpu_ctrl.illegal_instruction) begin
        mcause_data = write_mcause(0, TRAP_ILLEGAL_INST);
        // Debug encoding for illegal instruction traps:
        // [31:24]=0xdb, [23:17]=opcode,
        // [16]=fetch valid, [15]=decode valid, [14]=branch redirect,
        // [13]=branch suppression term, [12]=invalid opcode,
        // [11]=illegal instruction, [10]=fetch_valid input,
        // [9]=instruction_flush, [8]=decode_flush, [7]=mem_stall,
        // [6]=decode jump, [5]=decode jalr, [4]=decode branch,
        // [3]=execute valid, [2]=external interrupt,
        // [1]=decode trap, [0]=exception return.
        mtval_data = {
            8'hdb,
            opcode,
            fetch_regs.valid,
            decode_regs.valid,
            branch_redirect_taken,
            (decode_regs.valid && branch_redirect_taken),
            cpu_ctrl.invalid_opcode,
            cpu_ctrl.illegal_instruction,
            fetch_valid,
            instruction_flush,
            cpu_ctrl.decode_flush,
            cpu_ctrl.mem_stall,
            decode_regs.jump_branch_ctrl.is_jump_instr,
            decode_regs.jump_branch_ctrl.is_jump_register_instr,
            decode_regs.jump_branch_ctrl.is_branch_instr,
            execute_regs.valid,
            external_interrupt,
            cpu_ctrl.decode_trap,
            cpu_ctrl.exception_return
        };
    end else if (external_interrupt) begin
        mcause_data = write_mcause(1, IRQ_SRC_MEI);
        mtval_data = '0;
    end else begin
        mcause_data = machine_special_regs.mcause;
        mtval_data = machine_special_regs.mtval;
    end
end

// Special Registers
always_ff @(posedge clk) begin
    logic [31:0] csr_regs_wb_data;
    logic        csr_writeback_en;
    logic [3:0]  csr_class_bits;
    logic [7:0]  csr_offset_bits;

    csr_regs_wb_data = execute_regs.exec_result;
    csr_writeback_en = execute_regs.valid && execute_regs.wb_ctrl.csr_writeback_en;
    csr_class_bits   = execute_regs.wb_ctrl.csr_address[11:8];
    csr_offset_bits  = execute_regs.wb_ctrl.csr_address[7:0];

    if (!rst_n) begin
        machine_special_regs.mstatus <= '0;
        machine_special_regs.mie     <= '0;
        machine_special_regs.mip     <= '0;
        machine_special_regs.mepc    <= '0;
        machine_special_regs.mcause  <= '0;
        machine_special_regs.mtval   <= '0;
        machine_special_regs.mtvec   <= {30'd0, MTVEC_MODE_DIRECT};
        privilege_mode               <= PRIVILEGE_MODE_MACHINE;
    end else if (cpu_ctrl.exception_return) begin
        machine_special_regs.mstatus.mie  <= machine_special_regs.mstatus.mpie;
        machine_special_regs.mstatus.mpie <= 1'b1;
    end else if (exception_entry) begin
        machine_special_regs.mstatus.mie  <= '0;
        machine_special_regs.mstatus.mpie <= machine_special_regs.mstatus.mie;
        machine_special_regs.mepc         <= fetch_regs.fetch_pc;
        machine_special_regs.mcause       <= mcause_data;
        machine_special_regs.mtval        <= mtval_data;
    end else if (!cpu_ctrl.mem_stall && csr_writeback_en) begin
        case (csr_class_e'(csr_class_bits))
            CSR_CLASS_MACHINE_RW: begin
                case (csr_machine_rw_offset_e'(csr_offset_bits))
                    CSR_ADDRESS_MIE:     machine_special_regs.mie     <= write_mie(csr_regs_wb_data);
                    CSR_ADDRESS_MIP:     machine_special_regs.mip     <= write_mip(csr_regs_wb_data);
                    CSR_ADDRESS_MTVEC:   machine_special_regs.mtvec   <= {csr_regs_wb_data[31:2], MTVEC_MODE_DIRECT};
                    CSR_ADDRESS_MSTATUS: machine_special_regs.mstatus <= write_mstatus(csr_regs_wb_data);
                    CSR_ADDRESS_MEPC:    machine_special_regs.mepc    <= csr_regs_wb_data;
                    CSR_ADDRESS_MCAUSE:  machine_special_regs.mcause  <= csr_regs_wb_data;
                    CSR_ADDRESS_MTVAL:   machine_special_regs.mtval   <= csr_regs_wb_data;
                    default:             invalid_csr_write_address = 1'b1;
                endcase
            end
            CSR_CLASS_MACHINE_AUX_RW: begin
                // Not implemented yet
                invalid_csr_write_address = 1'b1;
            end
            CSR_CLASS_MACHINE_TIME: begin
                // Not implemented yet
                invalid_csr_write_address = 1'b1;
            end
            default:   
                invalid_csr_write_address = 1'b1;
        endcase
    end else begin
        machine_special_regs <= machine_special_regs;
    end
end

///////////////////////////////////////////////////////////////////////////////
// Stage 5  Writeback
///////////////////////////////////////////////////////////////////////////////
logic [31:0] xregs_wb_data;

always_comb begin
    // Writeback to X-Register File: CSR, JALR, or ALU/Load results
    if (memory_regs.wb_ctrl.csr_read) begin
        xregs_wb_data = memory_regs.csr_read_data;
    end else if (memory_regs.jump_branch_ctrl.is_jump_instr) begin
        xregs_wb_data = memory_regs.pc_plus4;
    end else begin
        xregs_wb_data = memory_regs.writeback_data;
    end
end

// X-Register File
always_ff @(posedge clk) begin
    if (!cpu_ctrl.mem_stall && memory_regs.valid && memory_regs.wb_ctrl.writeback_en) begin
        if (memory_regs.wb_ctrl.rd_reg_select != '0) begin
            // Writeback of ALU result, Load data, or CSR read data
            x_register_file[memory_regs.wb_ctrl.rd_reg_select] <= xregs_wb_data;
        end
    end
end

endmodule