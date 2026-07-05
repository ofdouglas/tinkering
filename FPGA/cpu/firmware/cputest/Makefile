include tests.mk

FIRMWARE_DIR := ..
CPU_DIR      := ../..
MEM_DIR      := $(CPU_DIR)/mem
BUILD_DIR    := build
SIM_DIR      := $(BUILD_DIR)/sim

PREFIX  := riscv64-unknown-elf-
# Base RV32I only: no M/A/F/D/C/B extensions. Matches rv32cpu RTL.
ARCH    := -march=rv32i -mabi=ilp32 -misa-spec=2.2
ASFLAGS := $(ARCH) -Wa,-march=rv32i
CFLAGS  := $(ARCH) -Os -ffreestanding -nostdlib -I$(FIRMWARE_DIR)
LDFLAGS := $(ARCH) -T $(FIRMWARE_DIR)/linker.ld -nostdlib
OBJDUMP := $(PREFIX)objdump -d -S -M no-aliases

SIM_BIN := $(SIM_DIR)/Vcpu_tb
MEM_LATENCY ?=
CYCLES ?=
RTL_SRCS := $(CPU_DIR)/rtl/rv32defs.sv \
            $(CPU_DIR)/rtl/rv32cpu.sv \
            $(CPU_DIR)/rtl/bus_interface.sv \
            $(CPU_DIR)/rtl/block_sram.sv \
            $(CPU_DIR)/sim/slow_memory_adapter.sv \
            $(CPU_DIR)/sim/cpu_tb.sv

TEST_HEXES := $(foreach t,$(TESTS),$(MEM_DIR)/$(t).hex)
TEST_EXPECTED_INSTALL := \
	$(foreach t,$(TESTS),$(if $(wildcard $(t).expected),$(MEM_DIR)/$(t).expected)) \
	$(foreach t,$(TESTS),$(if $(wildcard $(t).sram.expected),$(MEM_DIR)/$(t).sram.expected))

.PHONY: all build sim clean
all build: $(TEST_HEXES) $(TEST_EXPECTED_INSTALL)

sim: build $(SIM_BIN)
	@failed=0; \
	tests="$(if $(TEST),$(TEST),$(TESTS))"; \
	for t in $$tests; do \
		echo "=== cputest: $$t ==="; \
		extra=""; \
		for s in $(SKIP_REGS_TESTS); do \
			[ "$$t" = "$$s" ] && extra="+CPUTEST_SKIP_REGS"; \
		done; \
		if [ -f $$t.sram.expected ]; then \
			words=$$(awk '!/^@/ && NF {n++} END {print n+0}' $$t.sram.expected); \
			extra="$$extra +CPUTEST_SRAM_WORDS=$$words"; \
		fi; \
		[ -n "$(MEM_LATENCY)" ] && extra="$$extra +CPUTEST_MEM_LATENCY=$(MEM_LATENCY)"; \
		[ -n "$(CYCLES)" ] && extra="$$extra +CPUTEST_CYCLES=$(CYCLES)"; \
		if $(SIM_BIN) +CPUTEST_HEX=$(MEM_DIR)/$$t.hex +CPUTEST_EXPECTED=$(MEM_DIR)/$$t.expected $$extra; then \
			echo "=== PASS: $$t ==="; \
		else \
			echo "=== FAIL: $$t ==="; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	exit $$failed

$(SIM_BIN): $(RTL_SRCS) | $(SIM_DIR)
	verilator --binary -sv --timing -Wno-TIMESCALEMOD \
		--top-module cpu_tb --Mdir $(SIM_DIR) $(RTL_SRCS)

$(SIM_DIR) $(BUILD_DIR) $(MEM_DIR):
	mkdir -p $@

define ASM_TEST_RULES
$(BUILD_DIR)/$(1)/$(1).o: $(1).s | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(ASFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).elf: $(BUILD_DIR)/$(1)/$(1).o
	$(PREFIX)gcc $(LDFLAGS) -o $$@ $$^ -lgcc

$(BUILD_DIR)/$(1)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).elf
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $$< $$@
	$(OBJDUMP) $$< > $(BUILD_DIR)/$(1)/$(1).lst

$(MEM_DIR)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).hex | $(MEM_DIR)
	cp $$< $$@

$(BUILD_DIR)/$(1):
	mkdir -p $$@
endef

$(foreach t,$(ASM_TESTS),$(eval $(call ASM_TEST_RULES,$(t))))

define C_TEST_RULES
$(BUILD_DIR)/$(1)/start.o: $(FIRMWARE_DIR)/start.S | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(ASFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).o: $(1).c | $(BUILD_DIR)/$(1)
	$(PREFIX)gcc $(CFLAGS) -c $$< -o $$@

$(BUILD_DIR)/$(1)/$(1).elf: $(BUILD_DIR)/$(1)/start.o $(BUILD_DIR)/$(1)/$(1).o
	$(PREFIX)gcc $(LDFLAGS) -o $$@ $$^ -lgcc

$(BUILD_DIR)/$(1)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).elf
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $$< $$@
	$(OBJDUMP) $$< > $(BUILD_DIR)/$(1)/$(1).lst

$(MEM_DIR)/$(1).hex: $(BUILD_DIR)/$(1)/$(1).hex | $(MEM_DIR)
	cp $$< $$@

$(BUILD_DIR)/$(1):
	mkdir -p $$@
endef

$(foreach t,$(C_TESTS),$(eval $(call C_TEST_RULES,$(t))))

$(MEM_DIR)/%.expected: %.expected | $(MEM_DIR)
	cp $< $@

$(MEM_DIR)/%.sram.expected: %.sram.expected | $(MEM_DIR)
	cp $< $@

clean:
	rm -rf $(BUILD_DIR)
	$(foreach t,$(TESTS),rm -f $(MEM_DIR)/$(t).hex $(MEM_DIR)/$(t).expected $(MEM_DIR)/$(t).sram.expected;)
