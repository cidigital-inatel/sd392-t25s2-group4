# Compila o exemplo e roda no ISS:  make run
CROSS   ?= riscv-none-elf-
CFLAGS  := -march=rv32imac_zicsr -mabi=ilp32 -O2 -Wall -ffreestanding -nostdlib -nostartfiles
ISS     := ../../build/iss

all: hello.elf hello.dis

hello.elf: crt0.S main.c link.ld
	$(CROSS)gcc $(CFLAGS) -T link.ld -Wl,-Map=hello.map -o $@ crt0.S main.c -lgcc

hello.dis: hello.elf
	$(CROSS)objdump -d $< > $@

run: hello.elf
	$(ISS) hello.elf --stats

trace: hello.elf
	$(ISS) hello.elf --trace hello.trace

clean:
	rm -f hello.elf hello.dis hello.map hello.trace

.PHONY: all run trace clean
