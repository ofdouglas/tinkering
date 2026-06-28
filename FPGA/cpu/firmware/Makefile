PREFIX = riscv64-unknown-elf-
ARCH   = -march=rv32i -mabi=ilp32
CFLAGS = $(ARCH) -Os -ffreestanding -nostdlib
LDFLAGS = $(ARCH) -T linker.ld -nostdlib
HEX_INSTALL = ../mem/firmware.hex

all: firmware.hex firmware.lst

firmware.hex: firmware.elf
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(HEX_INSTALL))
	cp $@ $(HEX_INSTALL)
firmware.lst: firmware.elf
	$(PREFIX)objdump -d -S $< > $@

firmware.elf: start.o main.o
	$(PREFIX)gcc $(LDFLAGS) -o $@ $^ -lgcc

start.o: start.S
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

main.o: main.c
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o firmware.elf firmware.hex firmware.lst $(HEX_INSTALL)
