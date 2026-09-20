# --- MCU / toolchain ----------------------------------------------------------
MCU_PART := attiny2313a
F_CPU := 16000000UL

MPLABX_ROOT := /opt/microchip/mplabx/v6.35
IPECMD := $(MPLABX_ROOT)/mplab_platform/mplab_ipe/ipecmd.sh -TPPK5 -P$(MCU_PART) -ORIISP

HEXDUMP_FILE := ./dump.hex
IPE_HEX_DUMP := $(IPECMD) -GF$(abspath $(HEXDUMP_FILE))

MDB_FILE := ./mdb.txt
MDBCMD := $(MPLABX_ROOT)/mplab_platform/bin/mdb.sh $(abspath $(MDB_FILE))

GCC_FLAGS := -ffreestanding -nostdlib -mmcu=$(MCU_PART) -DF_CPU=$(F_CPU) -Os

#opt/microchip/mplabx/v6.35/mplab_platform/mplab_ipe/ipecmd.sh -TPPK5 -PATTINY2313A -ORIISP -GF/home/ofdouglas/Design/WireSpaces/foo.hex

all: main.hex

.PHONY: flash connect clean

connect:
	$(IPECMD) -OK

fuses:
	$(IPECMD) -MC -F$(abspath fuses.hex)

flash_ipe: main.hex
	$(IPECMD) -E -MP -F$(abspath main.hex)

flash_mdb: main.hex
	$(MDBCMD)

flash: flash_ipe

main.hex: main.S
	avr-gcc $(GCC_FLAGS) -o main.elf main.S
	avr-objcopy -O ihex main.elf main.hex

dump:
	$(IPE_HEX_DUMP)

clean:
	rm -f main.elf main.hex