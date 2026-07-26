# OpenOCD flash, reset, and GDB helpers.

OPENOCD ?= openocd
OPENOCD_IF ?= interface/stlink.cfg
OPENOCD_TGT ?= target/stm32f7x.cfg

.PHONY: flash reset gdbserver gdb

flash: $(BUILD_DIR)/$(TARGET).elf
	$(OPENOCD) -f $(OPENOCD_IF) -f $(OPENOCD_TGT) \
		-c "init" -c "program $< verify reset" -c "shutdown"

reset:
	$(OPENOCD) -f $(OPENOCD_IF) -f $(OPENOCD_TGT) \
		-c "init" -c "reset" -c "shutdown"

gdbserver:
	$(OPENOCD) -f $(OPENOCD_IF) -f $(OPENOCD_TGT)

gdb:
	gdb-multiarch $(BUILD_DIR)/$(TARGET).elf -ex "target remote :3333"
