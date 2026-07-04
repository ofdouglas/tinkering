package RiscV_32_Definitions;

///////////////////////////////////////////////////////////////////////////////
// Instruction Decode Enums
///////////////////////////////////////////////////////////////////////////////

typedef enum logic [6:0] {
    OPCODE_LUI      = 7'b0110111,
    OPCODE_AUIPC    = 7'b0010111,
    OPCODE_JAL      = 7'b1101111,
    OPCODE_JALR     = 7'b1100111,
    OPCODE_BRANCH   = 7'b1100011,
    OPCODE_LOAD     = 7'b0000011,
    OPCODE_STORE    = 7'b0100011,
    OPCODE_IMM_ALU  = 7'b0010011,
    OPCODE_REG_ALU  = 7'b0110011,
    OPCODE_FENCE    = 7'b0001111,
    OPCODE_SYSTEM   = 7'b1110011
} opcode_e;

typedef enum logic [2:0] { // funct_3[2:0]
    ALU_ADD   = 3'b000,
    ALU_SLL   = 3'b001,
    ALU_SLT   = 3'b010,
    ALU_SLTU  = 3'b011,
    ALU_XOR   = 3'b100,
    ALU_SRL   = 3'b101,
    ALU_OR    = 3'b110,
    ALU_AND   = 3'b111
} alu_register_e;
localparam logic [2:0] ALU_SUB = ALU_ADD; // SUB shares funct3; funct7 differentiates it.
localparam logic [2:0] ALU_SRA = ALU_SRL; // SRA shares funct3; funct7 differentiates it.

typedef enum logic [2:0] { // funct_3[2:0]
    ALU_ADDI   = 3'b000,
    ALU_SLLI   = 3'b001,
    ALU_SLTI   = 3'b010,
    ALU_SLTIU  = 3'b011,
    ALU_XORI   = 3'b100,
    ALU_SRAI   = 3'b101,
    ALU_ORI    = 3'b110,
    ALU_ANDI   = 3'b111
} alu_immediate_e;
localparam logic [2:0] ALU_SRLI = ALU_SRAI; // SRAI shares funct3; imm[11:5] differentiates it.

typedef enum logic [1:0] { // funct_3[1:0]
    LOGIC_XOR   = 2'b00,
    LOGIC_LUI   = 2'b01,
    LOGIC_OR    = 2'b10,
    LOGIC_AND   = 2'b11
} alu_logic_e;

typedef enum logic [0:0] { // funct_3[2]
    SHIFT_LEFT   = 1'b0,
    SHIFT_RIGHT  = 1'b1
} alu_shift_e;

typedef enum logic [1:0] { // {funct_3[2], funct_3[0}
    CMP_EQ       = 2'b00,
    CMP_NE       = 2'b01,
    CMP_LT       = 2'b10,
    CMP_GE       = 2'b11
} cmp_type_e;

typedef enum logic [0:0] { // funct_3[1] 
    CMP_SIGNED   = 1'b0,
    CMP_UNSIGNED = 1'b1
} cmp_sign_e;

typedef enum logic [0:0] { // funct_7[5]
   ADDER_CTRL_ADD  = 1'b0,
   ADDER_CTRL_SUB  = 1'b1
} adder_ctrl_e;

typedef enum logic [0:0] { // funct_7[5]
   SHIFTER_CTRL_SRL  = 1'b0,
   SHIFTER_CTRL_SRA  = 1'b1
} shift_sign_e;

typedef enum logic [1:0] {
    ALU_MUX_LOGIC   = 2'b00,
    ALU_MUX_ADDER   = 2'b01,
    ALU_MUX_SHIFT   = 2'b10,
    ALU_MUX_COMPARE = 2'b11
} alu_mux_e;
localparam alu_mux_e ALU_MUX_DEFAULT = ALU_MUX_LOGIC;

typedef enum logic [2:0] {
    BRCH_BEQ      = 3'b000,
    BRCH_BNE      = 3'b001,
    BRCH_BLT      = 3'b100,
    BRCH_BGE      = 3'b101,
    BRCH_BLTU     = 3'b110,
    BRCH_BGEU     = 3'b111
} branch_funct_3_e;

typedef enum logic [1:0] {
    MEM_BYTE     = 2'b00,
    MEM_HALF     = 2'b01,
    MEM_WORD     = 2'b10
} mem_size_e;

typedef enum logic [0:0] { // funct_3[2]
    LOAD_SIGNED   = 1'b0,
    LOAD_UNSIGNED = 1'b1
} load_signed_e;

///////////////////////////////////////////////////////////////////////////////
// Control Path Structs
///////////////////////////////////////////////////////////////////////////////

// TODO: compute all ALU mux values in decode, rather than passing instruction types
typedef struct packed {
    adder_ctrl_e   adder_ctrl;
    shift_sign_e   shift_sign_ctrl;
    logic          is_unsigned;
    logic          is_lui_instr;
    alu_mux_e      alu_mux_ctrl;
} AluControls;

typedef struct packed {
    logic        is_jump_instr;
    logic        is_jump_register_instr;
    logic        is_branch_instr;
} JumpBranchControls;

typedef struct packed {
    logic memory_request;
    logic memory_write;
    logic zero_extend;
} MemoryControls;

typedef struct packed {
    logic writeback_en;
    logic [4:0] rd_reg_select;
} WritebackControls;

///////////////////////////////////////////////////////////////////////////////
// Pipeline Registers
///////////////////////////////////////////////////////////////////////////////

typedef struct packed {
    logic              valid;
    logic [31:0]       current_pc;
    logic [31:0]       fetch_pc;
    logic [31:0]       instruction;
    logic              unaligned_pc;
} FetchStageRegs;

typedef struct packed {
    // Datapath
    logic [31:0]       left_operand;
    logic [31:0]       right_operand;
    logic [31:0]       current_pc;
    logic [31:0]       branch_immediate;
    logic [31:0]       store_value;

    // Control path
    logic              valid;
    logic [2:0]        funct_3;
    AluControls        alu_ctrl;
    JumpBranchControls jump_branch_ctrl;
    MemoryControls     mem_ctrl;
    WritebackControls  wb_ctrl;
    logic              invalid_opcode;
    logic              nop_instruction;
} DecodeStageRegs;

typedef struct packed {
    // Datapath
    logic [31:0]       exec_result;
    logic [31:0]       store_value;
    logic [31:0]       current_pc;

    // Control path
    logic              valid;
    logic [2:0]        funct_3;
    JumpBranchControls jump_branch_ctrl;
    MemoryControls     mem_ctrl;
    WritebackControls  wb_ctrl;
} ExecuteStageRegs;

typedef struct packed {
    logic branch_taken;
    logic [31:0] branch_pc;
} ExecuteCombinatorial;

typedef struct packed {
    // Datapath
    logic [31:0]       writeback_data;
    logic [31:0]       pc_plus4;

    // Control path
    logic              valid;
    JumpBranchControls jump_branch_ctrl;
    WritebackControls  wb_ctrl;
    logic              mem_stall;
    logic              mem_unaligned;
} MemoryStageRegs;


endpackage