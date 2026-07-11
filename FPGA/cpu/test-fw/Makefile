include tests.mk

CPU_DIR    := ..
MEM_DIR    := $(CPU_DIR)/mem
BUILD_DIR  := build
SIM_DIR    := $(BUILD_DIR)/sim
ASM_DIR    := src/asm
C_DIR      := src/c
COMMON_DIR := src/common
EXPECTED_DIR := expected

PREFIX  := riscv64-unknown-elf-
# Base RV32I only: no M/A/F/D/C/B extensions. Matches rv32cpu RTL.
ARCH    := -march=rv32i -mabi=ilp32 -misa-spec=2.2
ASFLAGS := $(ARCH) -Wa,-march=rv32i
CFLAGS  := $(ARCH) -Os -ffreestanding -nostdlib -I$(COMMON_DIR)
LDFLAGS := $(ARCH) -T linker.ld -nostdlib
OBJDUMP := $(PREFIX)objdump -d -S -M no-aliases
HEX_ADDR_OFFSET := -0x00010000

SIM_BIN := $(SIM_DIR)/Vcpu_tb
MEM_LATENCY ?=
CYCLES ?=
VERBOSE ?= 0
DUMP_REGS ?= 0
IRQ ?= 0
RTL_SRCS := $(CPU_DIR)/rtl/cpu_config_pkg.sv \
            $(CPU_DIR)/sim/test_data_pkg.sv \
            $(CPU_DIR)/rtl/rv32defs.sv \
            $(CPU_DIR)/rtl/rv32cpu.sv \
            $(CPU_DIR)/rtl/bus_interface.sv \
            $(CPU_DIR)/rtl/block_sram.sv \
            $(CPU_DIR)/sim/test_sram_block.sv \
            $(CPU_DIR)/sim/slow_memory_adapter.sv \
            $(CPU_DIR)/sim/cpu_tb.sv

TEST_HEXES := $(foreach t,$(TESTS),$(MEM_DIR)/$(t).hex)
TEST_EXPECTED_INSTALL := \
	$(foreach t,$(TESTS),$(if $(wildcard $(EXPECTED_DIR)/$(t).regs),$(MEM_DIR)/$(t).regs)) \
	$(foreach t,$(TESTS),$(if $(wildcard $(EXPECTED_DIR)/$(t).sram),$(MEM_DIR)/$(t).sram))

.PHONY: all build sim spike-ref clean
all build: $(TEST_HEXES) $(TEST_EXPECTED_INSTALL)

spike-ref:
	SPIKE="$${SPIKE:-$$HOME/riscv-spike/bin/spike}" python3 tools/spike_check.py $(if $(TEST),$(TEST))

sim: build $(SIM_BIN)
	@passed=0; failed=0; total=0; fail_log=""; \
	tests="$(if $(TEST),$(TEST),$(TESTS))"; \
	for t in $$tests; do total=$$((total + 1)); done; \
	n=0; \
	for t in $$tests; do \
		n=$$((n + 1)); \
		extra=""; \
		for s in $(SKIP_REGS_TESTS); do \
			[ "$$t" = "$$s" ] && extra="+CPUTEST_SKIP_REGS"; \
		done; \
		if [ -f $(EXPECTED_DIR)/$$t.sram ]; then \
			words=$$(awk '!/^@/ && !/^\/\// && NF {n++} END {print n+0}' $(EXPECTED_DIR)/$$t.sram); \
			extra="$$extra +CPUTEST_SRAM_WORDS=$$words +CPUTEST_SRAM_EXPECTED=$(MEM_DIR)/$$t.sram"; \
		fi; \
		[ -n "$(MEM_LATENCY)" ] && extra="$$extra +CPUTEST_MEM_LATENCY=$(MEM_LATENCY)"; \
		[ -n "$(CYCLES)" ] && extra="$$extra +CPUTEST_CYCLES=$(CYCLES)"; \
		[ "$(VERBOSE)" = "1" ] && extra="$$extra +CPUTEST_VERBOSE"; \
		[ "$(DUMP_REGS)" = "1" ] && extra="$$extra +CPUTEST_DUMP_REGS"; \
		[ "$$t" = "trap" ] && extra="$$extra +CPUTEST_IRQ"; \
		if [ "$(VERBOSE)" = "1" ]; then \
			echo "=== cpu-test [$$n/$$total]: $$t ==="; \
			if $(SIM_BIN) +CPUTEST_HEX=$(MEM_DIR)/$$t.hex +CPUTEST_EXPECTED=$(MEM_DIR)/$$t.regs $$extra; then \
				passed=$$((passed + 1)); \
				echo "=== PASS: $$t ==="; \
			else \
				failed=$$((failed + 1)); \
				fail_log="$$fail_log  $$t: (see output above)\n"; \
			fi; \
		else \
			out=$$($(SIM_BIN) +CPUTEST_HEX=$(MEM_DIR)/$$t.hex +CPUTEST_EXPECTED=$(MEM_DIR)/$$t.regs $$extra 2>&1); rc=$$?; \
			if [ $$rc -eq 0 ] && echo "$$out" | grep -q 'cputest: PASS'; then \
				passed=$$((passed + 1)); \
				printf '  %-24s ok\n' "$$t"; \
			else \
				failed=$$((failed + 1)); \
				msg=$$(echo "$$out" | sed -n 's/^cputest: FAIL //p' | head -1); \
				[ -z "$$msg" ] && msg="simulator error (exit $$rc)"; \
				printf '  %-24s FAIL\n' "$$t"; \
				echo "$$out" | grep -E '^cputest: FAIL|^cputest: ---|^cputest:   |^%Error:|^%Fatal:' | sed 's/^/    /'; \
				[ -z "$$(echo "$$out" | grep -E '^cputest: FAIL|^%Error:|^%Fatal:')" ] && \
					echo "    $$msg"; \
				fail_log="$$fail_log  $$t: $$msg\n"; \
			fi; \
		fi; \
	done; \
	echo ""; \
	if [ $$failed -eq 0 ]; then \
		echo "$$passed/$$total passed"; \
	else \
		echo "$$passed/$$total passed, $$failed failed:"; \
		printf '%b' "$$fail_log"; \
	fi; \
	exit $$failed

$(SIM_BIN): $(RTL_SRCS) | $(SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD \
		--top-module cpu_tb --Mdir $(SIM_DIR) $(RTL_SRCS)

$(SIM_DIR) $(BUILD_DIR) $(MEM_DIR):
	mkdir -p $@

define ASM_TEST_RULES
$(BUILD_DIR)/$(1)/$(1).o: $(ASM_DIR)/$(1).s | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(ASFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).elf $(BUILD_DIR)/$(1)/$(1).map: $(BUILD_DIR)/$(1)/$(1).o
	$(PREFIX)gcc $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(1)/$(1).map -o $(BUILD_DIR)/$(1)/$(1).elf $$^ -lgcc

$(BUILD_DIR)/$(1)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).elf $(BUILD_DIR)/$(1)/$(1).map
	$(PREFIX)objcopy --change-addresses $(HEX_ADDR_OFFSET) -O verilog --verilog-data-width=4 $$< $$@
	$(OBJDUMP) $$< > $(BUILD_DIR)/$(1)/$(1).lst

$(MEM_DIR)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).hex | $(MEM_DIR)
	cp $$< $$@

$(BUILD_DIR)/$(1):
	mkdir -p $$@
endef

$(foreach t,$(ASM_TESTS),$(eval $(call ASM_TEST_RULES,$(t))))

define C_TEST_RULES
$(BUILD_DIR)/$(1)/start.o: $(COMMON_DIR)/start.S | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(ASFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).o: $(C_DIR)/$(1).c | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(CFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).elf $(BUILD_DIR)/$(1)/$(1).map: $(BUILD_DIR)/$(1)/start.o $(BUILD_DIR)/$(1)/$(1).o
	$(PREFIX)gcc $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(1)/$(1).map -o $(BUILD_DIR)/$(1)/$(1).elf $$^ -lgcc

$(BUILD_DIR)/$(1)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).elf $(BUILD_DIR)/$(1)/$(1).map
	$(PREFIX)objcopy --change-addresses $(HEX_ADDR_OFFSET) -O verilog --verilog-data-width=4 $$< $$@
	$(OBJDUMP) $$< > $(BUILD_DIR)/$(1)/$(1).lst

$(MEM_DIR)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).hex | $(MEM_DIR)
	cp $$< $$@

$(BUILD_DIR)/$(1):
	mkdir -p $$@
endef

$(foreach t,$(C_TESTS),$(eval $(call C_TEST_RULES,$(t))))

$(MEM_DIR)/%.regs: $(EXPECTED_DIR)/%.regs | $(MEM_DIR)
	cp $< $@

$(MEM_DIR)/%.sram: $(EXPECTED_DIR)/%.sram | $(MEM_DIR)
	cp $< $@

clean:
	rm -rf $(BUILD_DIR)
	$(foreach t,$(TESTS),rm -f $(MEM_DIR)/$(t).hex $(MEM_DIR)/$(t).regs $(MEM_DIR)/$(t).sram;)
