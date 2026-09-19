CC      ?= gcc
OBJCOPY ?= objcopy

CFLAGS  ?= -nostdinc -nostdlib -fno-stack-protector -static -fPIE -ffreestanding -Os -Wall -Wextra
LDFLAGS ?= -Wl,-gc-sections

OUT ?= which-southbridge

all: $(OUT).bin

$(OUT).elf: src/start.o src/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/start.o: src/start.S
	$(CC) $(CFLAGS) -c -o $@ $<

# Raw shellcode: the loader executes the .bin from its first byte.
$(OUT).bin: $(OUT).elf
	$(OBJCOPY) \
	    --only-section .text \
	    --only-section .data \
	    --only-section .bss \
	    --only-section .rodata \
	    -O binary $< $@

clean:
	rm -f $(OUT).elf $(OUT).bin src/*.o

.PHONY: all clean
