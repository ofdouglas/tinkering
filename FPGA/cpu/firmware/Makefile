PREFIX = riscv64-unknown-elf-
ARCH   = -march=rv32i -mabi=ilp32
CFLAGS = $(ARCH) -Os -ffreestanding -nostdlib
LDFLAGS = $(ARCH) -T linker.ld -nostdlib
HEX_INSTALL = ../mem/firmware.hex
CPUTEST_HEX_INSTALL = ../mem/cputest.hex

all: firmware.hex firmware.lst

cputest: cputest.hex cputest.lst

firmware.hex: firmware.elf
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(HEX_INSTALL))
	cp $@ $(HEX_INSTALL)
firmware.lst: firmware.elf
	$(PREFIX)objdump -d -S $< > $@

cputest.hex: cputest.elf
	$(PREFIX)objcopy -O verilog --verilog-data-width=4 $< $@
	@mkdir -p $(dir $(CPUTEST_HEX_INSTALL))
	cp $@ $(CPUTEST_HEX_INSTALL)
cputest.lst: cputest.elf
	$(PREFIX)objdump -d -S $< > $@

firmware.elf: start.o main.o
	$(PREFIX)gcc $(LDFLAGS) -o $@ $^ -lgcc

cputest.elf: cputest/cputest.o
	$(PREFIX)gcc $(LDFLAGS) -o $@ $^ -lgcc

start.o: start.S
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

main.o: main.c
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

cputest/cputest.o: cputest/cputest.s
	$(PREFIX)gcc $(CFLAGS) -c $< -o $@

.PHONY: all cputest clean
clean:
	rm -f *.o cputest/*.o firmware.elf firmware.hex firmware.lst cputest.elf cputest.hex cputest.lst $(HEX_INSTALL) $(CPUTEST_HEX_INSTALL)
