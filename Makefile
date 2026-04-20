# ── Toolchain ─────────────────────────────────────────────────────────────────
CC        = gcc
CFLAGS    = -m32 -ffreestanding -O2 -Wall -Wextra \
            -fno-stack-protector -fno-builtin \
            -nostdlib -nostdinc \
            -fno-pic -fno-pie \
            -mno-sse -mno-sse2 -mno-mmx \
            -Ikernel

NASM      = nasm
NASMFLAGS = -f elf32

# ld 32-bit
LDFLAGS   = -m elf_i386 -T linker.ld -nostdlib -z max-page-size=0x1000

OBJ = kernel/boot.o   \
      kernel/vga.o    \
      kernel/keyboard.o \
      kernel/fs.o     \
      kernel/shell.o  \
      kernel/shellcommands/cmd_system.o \
      kernel/shellcommands/cmd_fs.o     \
      kernel/shellcommands/cmd_math.o   \
      kernel/shellcommands/cmd_misc.o   \
      kernel/shellcommands/cmd_bf.o     \
      kernel/kernel.o

ISO = tpbos.iso
BIN = tpbos.bin

.PHONY: all iso clean run

all: $(BIN)

kernel/boot.o: kernel/boot.asm
	$(NASM) $(NASMFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/shellcommands/%.o: kernel/shellcommands/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OBJ)
	ld $(LDFLAGS) -o $@ $^

iso: $(BIN)
	mkdir -p iso/boot/grub
	cp $(BIN)             iso/boot/tpbos.bin
	cp boot/grub/grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso
	@echo ""
	@echo "  ISO lista: $(ISO)"

run: iso
	qemu-system-i386 -cdrom $(ISO) -m 64M -no-reboot -no-shutdown

clean:
	rm -f kernel/*.o kernel/shellcommands/*.o $(BIN) $(ISO)
	rm -rf iso
