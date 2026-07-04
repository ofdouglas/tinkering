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