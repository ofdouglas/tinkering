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

typedef enum logic [2:0] {
    ALU_ADD_SUB  = 3'b000,
    ALU_SLL      = 3'b001,
    ALU_SLT      = 3'b010,
    ALU_SLTU     = 3'b011,
    ALU_XOR      = 3'b100,
    ALU_SRL_SRA  = 3'b101,
    ALU_OR       = 3'b110,
    ALU_AND      = 3'b111
} alu_funct_3_e;

typedef enum logic [2:0] {
    ALU_BEQ      = 3'b000,
    ALU_BNE      = 3'b001,
    ALU_BLT      = 3'b100,
    ALU_BGE      = 3'b101,
    ALU_BLTU     = 3'b110,
    ALU_BGEU     = 3'b111
} alu_branch_funct_3_e;

typedef enum logic [2:0] {
    MEM_BYTE     = 3'b000,
    MEM_HALF     = 3'b001,
    MEM_WORD     = 3'b010,
} mem_funct_3_e;

///////////////////////////////////////////////////////////////////////////////
// Control Path Structs
///////////////////////////////////////////////////////////////////////////////

// TODO: compute all ALU mux values in decode, rather than passing instruction types
typedef struct {
    logic is_subtract;
    logic is_unsigned;
    logic is_branch_instr;
    logic is_lui_instr;
    logic is_auipc_instr;
} AluControls;

typedef struct {
    logic memory_request;
    logic memory_write;
    logic zero_extend;
} MemoryControls;

typedef struct {
    logic writeback_en;
    logic [4:0] rd_reg_select;
    logic is_jump_instr;
} WritebackControls;

///////////////////////////////////////////////////////////////////////////////
// Pipeline Registers
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    logic [31:0] fetch_pc;
    logic [31:0] pc_plus4;
    logic [31:0] instruction;
    logic        valid;
    logic        nop_instruction;
    logic        unaligned_pc;
} FetchStageRegs;

typedef struct {
    // Datapath
    logic [31:0]      left_operand;
    logic [31:0]      right_operand;
    logic [31:0]      fetch_pc;
    logic [31:0]      branch_immediate;
    logic [31:0]      store_value;

    // Control path
    logic [2:0]       funct_3;
    AluControls       alu_ctrl;
    MemoryControls    mem_ctrl;
    WritebackControls wb_ctrl;
    logic             invalid_opcode;
    logic             nop_instruction;
} DecodeStageRegs;

typedef struct {
    // Datapath
    logic [31:0]      exec_result;
    logic [31:0]      store_value;
    logic [31:0]      fetch_pc;

    // Control path
    logic [2:0]       funct_3;
    MemoryControls    mem_ctrl;
    WritebackControls wb_ctrl;
    logic             branch_taken;
    logic [31:0]      branch_pc;
} ExecuteStageRegs;

typedef struct {
    logic branch_taken;
    logic [31:0] branch_pc;
} ExecuteCombinatorial;

typedef struct {
    // Datapath
    logic [31:0]      writeback_data;
    logic [31:0]      pc_plus4;

    // Control path
    WritebackControls wb_ctrl;
    logic             mem_stall;
    logic             mem_unaligned;
} MemoryStageRegs;


endpackage