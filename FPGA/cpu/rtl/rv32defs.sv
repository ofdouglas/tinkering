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

typedef enum logic [1:0] {
    ALU_LEFT_SRC_PC        = 2'b00,
    ALU_LEFT_SRC_RS1       = 2'b01,
    ALU_LEFT_SRC_IMM       = 2'b10
} alu_left_src_e;
localparam alu_left_src_e ALU_LEFT_SRC_DONT_CARE = ALU_LEFT_SRC_PC;

typedef enum logic [0:0] {
    ALU_RIGHT_SRC_RS2       = 1'b0,
    ALU_RIGHT_SRC_IMM       = 1'b1
} alu_right_src_e;
localparam alu_right_src_e ALU_RIGHT_SRC_DONT_CARE = ALU_RIGHT_SRC_RS2;

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

typedef enum logic [1:0] {
    PRIVILEGE_MODE_USER        = 2'b00,
    PRIVILEGE_MODE_SUPERVISOR  = 2'b01,
    PRIVILEGE_MODE_RESERVED    = 2'b10,
    PRIVILEGE_MODE_MACHINE     = 2'b11
} privilege_mode_e;

typedef enum logic [1:0] {
    // RW* = read/write (all 3 values are equivalent)
    CSR_ACCESS_RW0        = 2'b00,
    CSR_ACCESS_RW1        = 2'b01,
    CSR_ACCESS_RW2        = 2'b10,
    // Read-only CSR
    CSR_ACCESS_READ_ONLY = 2'b11
} csr_access_type_e;

typedef enum logic [4:0] {
    IRQ_SRC_RESERVED0     = 5'd0,
    IRQ_SRC_SSI           = 5'd1,  // Supervisor-level Software Interrupt
    IRQ_SRC_RESERVED2     = 5'd2,
    IRQ_SRC_MSI           = 5'd3,  // Machine-level Software Interrupt
    IRQ_SRC_RESERVED4     = 5'd4,
    IRQ_SRC_STI           = 5'd5,  // Supervisor-level Timer Interrupt
    IRQ_SRC_RESERVED6     = 5'd6,
    IRQ_SRC_MTI           = 5'd7,  // Machine-level Timer Interrupt
    IRQ_SRC_RESERVED8     = 5'd8,
    IRQ_SRC_SEI           = 5'd9,  // Supervisor-level External Interrupt
    IRQ_SRC_RESERVED10    = 5'd10,
    IRQ_SRC_MEI           = 5'd11, // Machine-level External Interrupt
    IRQ_SRC_RESERVED12    = 5'd12,
    IRQ_SRC_LCOFI         = 5'd13,
    IRQ_SRC_RESERVED14    = 5'd14,
    IRQ_SRC_RESERVED15    = 5'd15,
    IRQ_SRC_RESERVED16    = 5'd16,
    IRQ_SRC_RESERVED17    = 5'd17,
    IRQ_SRC_RESERVED18    = 5'd18,
    IRQ_SRC_RESERVED19    = 5'd19,
    IRQ_SRC_RESERVED20    = 5'd20,
    IRQ_SRC_RESERVED21    = 5'd21,
    IRQ_SRC_RESERVED22    = 5'd22,
    IRQ_SRC_RESERVED23    = 5'd23,
    IRQ_SRC_RESERVED24    = 5'd24,
    IRQ_SRC_RESERVED25    = 5'd25,
    IRQ_SRC_RESERVED26    = 5'd26,
    IRQ_SRC_RESERVED27    = 5'd27,
    IRQ_SRC_RESERVED28    = 5'd28,
    IRQ_SRC_RESERVED29    = 5'd29,
    IRQ_SRC_RESERVED30    = 5'd30,
    IRQ_SRC_RESERVED31    = 5'd31
} irq_source_e;
localparam irq_source_e IRQ_SRC_DONT_CARE = IRQ_SRC_RESERVED0;

typedef enum logic [4:0] {
    TRAP_INST_ADDR_MISALIGNED  = 5'd0,
    TRAP_INST_ACCESS_FAULT     = 5'd1,
    TRAP_ILLEGAL_INST          = 5'd2,
    TRAP_BREAKPOINT            = 5'd3,
    TRAP_LOAD_ADDR_MISALIGNED  = 5'd4,
    TRAP_LOAD_ACCESS_FAULT     = 5'd5,
    TRAP_STORE_ADDR_MISALIGNED = 5'd6,
    TRAP_STORE_ACCESS_FAULT    = 5'd7,
    TRAP_ECALL_U               = 5'd8,  // Environment call from U-mode
    TRAP_ECALL_S               = 5'd9,  // Environment call from S-mode
    TRAP_RESERVED10            = 5'd10,
    TRAP_ECALL_M               = 5'd11, // Environment call from M-mode
    TRAP_INST_PAGE_FAULT       = 5'd12,
    TRAP_LOAD_PAGE_FAULT       = 5'd13,
    TRAP_RESERVED14            = 5'd14,
    TRAP_STORE_PAGE_FAULT      = 5'd15,
    TRAP_RESERVED16            = 5'd16,
    TRAP_RESERVED17            = 5'd17,
    TRAP_SOFTWARE_CHECK        = 5'd18,
    TRAP_HARDWARE_ERROR        = 5'd19,
    TRAP_RESERVED20            = 5'd20,
    TRAP_RESERVED21            = 5'd21,
    TRAP_RESERVED22            = 5'd22,
    TRAP_RESERVED23            = 5'd23,
    TRAP_CUSTOM24              = 5'd24, // Designated for custom use
    TRAP_CUSTOM25              = 5'd25,
    TRAP_CUSTOM26              = 5'd26,
    TRAP_CUSTOM27              = 5'd27,
    TRAP_CUSTOM28              = 5'd28,
    TRAP_CUSTOM29              = 5'd29,
    TRAP_CUSTOM30              = 5'd30,
    TRAP_CUSTOM31              = 5'd31
} trap_type_e;

typedef enum logic [7:0] { // Trap vector base address mode
    MTVEC_MODE_DIRECT =   8'd0,
    MTVEC_MODE_VECTORED = 8'd1,
    MTVEC_MODE_RESERVED = 8'd2
} mtvec_mode_e;


typedef enum logic [7:0] { // Lower 8 bits of CSR address
    // Machine trap setup (0x00–0x06, 0x10, 0x12)
    CSR_ADDRESS_MSTATUS        = 8'h00,
    CSR_ADDRESS_MISA           = 8'h01,
    CSR_ADDRESS_MEDELEG        = 8'h02,
    CSR_ADDRESS_MIDELEG        = 8'h03,
    CSR_ADDRESS_MIE            = 8'h04,
    CSR_ADDRESS_MTVEC          = 8'h05,
    CSR_ADDRESS_MCOUNTEREN     = 8'h06,
    CSR_ADDRESS_MSTATUSH       = 8'h10, // RV32 only
    CSR_ADDRESS_MDELEGH        = 8'h12, // Upper 32 bits of medeleg, RV32 only

    // Machine trap handling (0x40–0x44, 0x4A–0x4B)
    CSR_ADDRESS_MSCRATCH       = 8'h40,
    CSR_ADDRESS_MEPC           = 8'h41,
    CSR_ADDRESS_MCAUSE         = 8'h42,
    CSR_ADDRESS_MTVAL          = 8'h43,
    CSR_ADDRESS_MIP            = 8'h44,
    CSR_ADDRESS_MTINST         = 8'h4A,
    CSR_ADDRESS_MTVAL2         = 8'h4B,

    // Machine information registers (0x11–0x15)
    CSR_ADDRESS_MVENDORID      = 8'h11,
    CSR_ADDRESS_MARCHID        = 8'h12, // full address 0xF12
    CSR_ADDRESS_MIMPID         = 8'h13,
    CSR_ADDRESS_MHARTID        = 8'h14,
    CSR_ADDRESS_MCONFIGPTR     = 8'h15
} machine_csr_address_e;


typedef struct packed {
    csr_access_type_e  access_type;
    privilege_mode_e   min_priviledge;
    logic [7:0]        address;
} CsrAddressFields;


///////////////////////////////////////////////////////////////////////////////
// Control Path Structs
///////////////////////////////////////////////////////////////////////////////


// TODO: compute all ALU mux values in decode, rather than passing instruction types
typedef struct packed {
    // Overall control
    alu_left_src_e  alu_left_src;
    alu_right_src_e alu_right_src;
    alu_mux_e       alu_mux_ctrl;

    // Special functions
    adder_ctrl_e    adder_ctrl;
    shift_sign_e    shift_sign_ctrl;
    logic           is_unsigned;
    logic           is_lui_instr;
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
    // Datapath
    logic [31:0]       current_pc;
    logic [31:0]       fetch_pc;
    logic [31:0]       instruction;

    // Control path
    logic              valid;
    logic              unaligned_pc;
} FetchStageRegs;

typedef struct packed {
    // Datapath
    logic [31:0]       rs1_reg;
    logic [31:0]       rs2_reg;
    logic [31:0]       current_pc;
    logic [31:0]       immediate;
    logic  [4:0]       rs1_index;
    logic  [4:0]       rs2_index;

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
    logic [31:0]       rs2_reg;
    logic [31:0]       current_pc;
    logic  [4:0]       rs2_index;

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
    logic              mem_unaligned;
} MemoryStageRegs;


///////////////////////////////////////////////////////////////////////////////
// Machine Trap Control and Status Registers
///////////////////////////////////////////////////////////////////////////////

typedef struct packed {
    logic [29:0]   base; // Machine Trap Vector Base Address
    mtvec_mode_e   mode;
} mtvec_csr_t;

typedef struct packed {
    logic [31:0] irq_enable_bits; // access with irq_source_e (TODO: clean up this access method)
} mie_csr_t;

typedef struct packed {
    logic [31:0] irq_pending_bits; // access with irq_source_e (TODO: clean up this access method)
} mip_csr_t;

typedef struct packed {
    logic            mie;  // Bit 3: Global Machine Interrupt Enable
    logic            mpie; // Bit 7: Machine Previous Interrupt Enable
    privilege_mode_e mpp;  // Bits 12:11: Machine Previous Privilege
} mstatus_csr_t;

typedef struct packed { 
    logic [31:0] mepc;  // Machine Exception Program Counter (faulting instruction address if sync)
                        // interrupted instruction if async (ex: external interrupt)
                        // mret returns from handler to this address, so sync traps must increment it!
} mepc_csr_t;

typedef struct packed {
    logic        is_interrupt; // 0 = trap (synchronous), 1 = interrupt (asynchronous)
    logic [30:0] exception_code;
} mcause_csr_t;

typedef struct packed {
    logic [31:0] mtval; // Machine Trap Value (faulting bus address or instruction bits)
} mtval_csr_t;



endpackage