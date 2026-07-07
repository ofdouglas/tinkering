ASM_TESTS := basic_alu pipelined_alu alu_boundary branch load_store load_use jump_lui_auipc csr_basic csr_immediate csr_mixed trap illegal
C_TESTS   := crc store_hazard
TESTS     := $(ASM_TESTS) $(C_TESTS)

# Verilator is 2-state; xxxxxxxx in .expected reads as 0. Skip reg check for these tests.
SKIP_REGS_TESTS := crc store_hazard

# Spike uses ROM @ 0x1000 (debug module @ 0). Skip CSR + PC-layout-sensitive asm tests.
SPIKE_SKIP_TESTS := csr_basic csr_immediate csr_mixed jump_lui_auipc load_use trap illegal

# Tests with a $(test).sram.expected file get SRAM checks; word count is derived from that file at sim time.
SRAM_TESTS := $(patsubst %.sram.expected,%,$(wildcard *.sram.expected))

crc_SRCS          := ../start.S crc.c
store_hazard_SRCS := ../start.S store_hazard.c
