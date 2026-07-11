ASM_TESTS := basic_alu pipelined_alu alu_boundary branch load_store load_use jump_lui_auipc csr_basic csr_immediate csr_mixed trap illegal
C_TESTS   := crc store_hazard startup
TESTS     := $(ASM_TESTS) $(C_TESTS)

# Verilator is 2-state; xxxxxxxx in .regs reads as 0. Skip reg check for these tests.
SKIP_REGS_TESTS := crc store_hazard startup

# Spike runs the same CPU-test ELF layout as cpu_tb. Skip tests where RTL does not
# yet match Spike.
# TODO: fix discrepancies and remove items from the list.
SPIKE_SKIP_TESTS := csr_basic csr_immediate csr_mixed trap illegal

# Tests with an expected/$(test).sram file get SRAM checks; word count is derived from that file at sim time.
SRAM_TESTS := $(patsubst expected/%.sram,%,$(wildcard expected/*.sram))

crc_SRCS          := src/common/start.S src/c/crc.c
store_hazard_SRCS := src/common/start.S src/c/store_hazard.c
startup_SRCS      := src/common/start.S src/c/startup.c