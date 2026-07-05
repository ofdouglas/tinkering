# Risc-V32I CPU Design

## Datapath Overview

| Opcode   | ALU Left | ALU Right | PC Left  | PC Right | WB Result |
| -------- | -------- | --------- | -------- |--------- |---------- |
| LUI      |          |           |          |          | ?         |
| AUIPC    |          |           | pc       | imm20    | PC Adder  |
| JAL      |          |           | pc       | imm20    | PC+4      |
| JALR     |          |           | rs1      | imm12    | PC+4      |
| BRANCH   | rs1      | rs2       | pc       | imm12    | -         |
| LOAD     | rs1      | imm12     |          |          | mem       |
| STORE    | rs1      | imm12     |          |          | -         |
| IMM_ALU  | rs1      | imm12     |          |          | ALU       |
| REG_ALU  | rs1      | rs2       |          |          | ALU       |
| FENCE    |          |           |          |          |           |
| SYSTEM   |          |           |          |          |           |


| Opcode        | Instruction | ALU Output | PC Output | Writeback   |
| ------------- | ----------- | ---------- | --------- | ----------- |
| LUI           | LUI         | Logic      | -         | ALU         |
| REG_ALU       | XOR         | Logic      | -         | ALU         |
| REG_ALU       | OR          | Logic      | -         | ALU         |
| REG_ALU       | AND         | Logic      | -         | ALU         |
| IMM_ALU       | XORI        | Logic      | -         | ALU         |
| IMM_ALU       | ORI         | Logic      | -         | ALU         |
| IMM_ALU       | ANDI        | Logic      | -         | ALU         |
| IMM_ALU       | ADDI        | Adder      | -         | ALU         |
| REG_ALU       | ADD         | Adder      | -         | ALU         |
| REG_ALU       | SUB         | Adder      | -         | ALU         |
| LOAD          | LB          | Adder      | -         | Mem         |
| LOAD          | LH          | Adder      | -         | Mem         |
| LOAD          | LW          | Adder      | -         | Mem         |
| LOAD          | LBU         | Adder      | -         | Mem         |
| LOAD          | LHU         | Adder      | -         | Mem         |
| STORE         | SB          | Adder      | -         | -           |
| STORE         | SH          | Adder      | -         | -           |
| STORE         | SW          | Adder      | -         | -           |
| IMM_ALU       | SLLI        | Shifter    | -         | ALU         |
| IMM_ALU       | SRLI        | Shifter    | -         | ALU         |
| IMM_ALU       | SRAI        | Shifter    | -         | ALU         |
| REG_ALU       | SLL         | Shifter    | -         | ALU         |
| REG_ALU       | SRL         | Shifter    | -         | ALU         |
| REG_ALU       | SRA         | Shifter    | -         | ALU         |
| REG_ALU       | SLT         | Comp       | -         | ALU         |
| REG_ALU       | SLTU        | Comp       | -         | ALU         |
| IMM_ALU       | SLTI        | Comp       | -         | ALU         |
| IMM_ALU       | SLTIU       | Comp       | -         | ALU         |
| BRANCH        | BEQ         | Comp       | PC Adder  | -           |
| BRANCH        | BNE         | Comp       | PC Adder  | -           |
| BRANCH        | BLT         | Comp       | PC Adder  | -           |
| BRANCH        | BGE         | Comp       | PC Adder  | -           |
| BRANCH        | BLTU        | Comp       | PC Adder  | -           |
| BRANCH        | BGEU        | Comp       | PC Adder  | -           |
| AUIPC         | AUIPC       | Adder      | -         | ALU         |
| JAL           | JAL         | -          | PC Adder  | PC+4        |
| JALR          | JALR        | -          | PC Adder  | PC+4        |
| FENCE         | FENCE       | ?          | ?         | -           | 
| FENCE         | FENCE.TSO   | ?          | ?         | -           |
| FENCE         | BREAK       | ?          | ?         | -           |
| SYSTEM        | ECALL       | ?          | ?         | -           |
| SYSTEM        | EBREAK      | ?          | ?         | -           |


## Privileged ISA Notes

### CSRs
Address format
- 12 bits
- top 2 bits:  {11} == read-only, else R/W
- next 2 bits: minimum priviledge required
- last 8 bits: actual address

Illegal instruction fault: 
- access non-existent CSR
- access higher priviledge CSR
- write to read-only CSR

Priviledge Levels and Interrupt Enables
- IRQs at higher priority levels are always *enabled*, independent of mstatus.xIE
- IRQs at higher priority levels are always *disabled*, independent of mstatus.xIE


Machine Trap Registers:

| Opcode  | Name (M)                  | Purpose                                                   |
|---------|---------------------------|---------------------------------------------------------|
| mtvec   | Trap Vector Base          |                                        |
| mstatus | Status                    | Global interrupt enable, previous privilege mode, etc.  |
| mie     | Interrupt Enable          | Mask to enable/disable specific interrupts.   |
| mip     | Interrupt Pending         | Indicates which interrupts are pending.       |
| mepc    | Exception Program Counter | Holds PC to return to after trap/interrupt. |
| mcause  | Cause                     | Reason code for the last trap or interrupt.            |
| mtval   | Trap Value                | Additional info for certain traps (fault address or instruction). |
| priv    | Priviledge Mode           | U/S/M mode; implementation-defined and not user-readable |

Exception Entry:
- save mepc, mcause, mtval
- increase priviledge level (optional; 'horizontal' trap keeps prio)
- ack IRQ?
- update mstatus.{mie <= 0, mpie <= mie, mpp <= priviledge}
- PC <= {mvtec.base, 2'b00}, OR for async (irq) in vectored mode, {mtvec.base + 4 * mcause.exception_code}

Exception Handling:
- SW is responsible for saving *any/all* registers used in handler
- SW is responsible for incrementing mepc for synchronous trap (avoid mret infinite loop)
- return via mret

Exception Return:
- restore priviledge to mstatus.mpp
- update mstatus.{mie <= mpie, mpie <= 1, mpp <= ?}
- PC <= mepc


### Traps (Synchronous)

| Name                  | Num |
| --------------------- | ----|
| INST_ADDR_MISALIGNED  | 0   |
| INST_ACCESS_FAULT     | 1   |
| ILLEGAL_INST          | 2   |
| BREAKPOINT            | 3   |
| LOAD_ADDR_MISALIGNED  | 4   |
| LOAD_ACCESS_FAULT     | 5   |
| STORE_ADDR_MISALIGNED | 6   |
| STORE_ACCESS_FAULT    | 7   |
| ECALL_U               | 8   |
| ECALL_M               | 11  |

### Interrupts (Asynchronous)

| Name                | Num |
| ------------------- | ----|
| M_SW_IRQ  (MSI)     | 3   |
| M_TIM_IRQ (MTI)     | 3   |
| M_EXT     (MEI)     | 11  |

- ECALL is synchronous trap, saves ECALL's PC to mret
- Ext IRQ is async trap, saves interrupted instruction PC to mret

- Exit trap by 'mret': pc <- mepc
- Sync trap: must increment mepc before mret to avoid infinite loop