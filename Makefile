# AgentOS Makefile
# Week 1 Day 4: Build i386 Multiboot2 kernel

# Toolchain
CC = clang
AS = clang
LD = ld.lld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386

# Target architecture
TARGET = i386-elf

# Directories
KERNEL_DIR = kernel
BOOT_DIR = boot
BUILD_DIR = build

# Output
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
ISO = $(BUILD_DIR)/agentos.iso
ISO_DIR = $(BUILD_DIR)/iso
ISO_BOOT_DIR = $(ISO_DIR)/boot
ISO_GRUB_DIR = $(ISO_BOOT_DIR)/grub
ISO_KERNEL = $(ISO_BOOT_DIR)/kernel.elf
ISO_GRUB_CFG = $(ISO_GRUB_DIR)/grub.cfg

# Source files
ENTRY_S = $(KERNEL_DIR)/arch/x86_64/entry.S
MAIN_C = $(KERNEL_DIR)/main.c
VGA_C = $(KERNEL_DIR)/vga.c

# Object files
ENTRY_O = $(BUILD_DIR)/entry.o
MAIN_O = $(BUILD_DIR)/main.o
VGA_O = $(BUILD_DIR)/vga.o

# Compiler flags
CFLAGS = -target $(TARGET) \
         -ffreestanding \
         -fno-builtin \
         -nostdlib \
         -fno-stack-protector \
         -Wall \
         -Wextra \
         -Werror \
         -g

# Assembler flags
ASFLAGS = -target $(TARGET) \
          -g

# Linker flags
LDFLAGS = -m elf_i386 \
          -T $(BOOT_DIR)/linker.ld \
          -static

.PHONY: all kernel iso run clean

all: kernel

kernel: $(KERNEL_ELF)

iso: $(ISO)

run: $(ISO)
	$(QEMU) -cdrom $(ISO) -m 128M -serial stdio -boot d -no-reboot -no-shutdown

$(KERNEL_ELF): $(ENTRY_O) $(MAIN_O) $(VGA_O) $(BOOT_DIR)/linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ENTRY_O) $(MAIN_O) $(VGA_O)

$(ENTRY_O): $(ENTRY_S) | $(BUILD_DIR)
	$(AS) $(ASFLAGS) -c $< -o $@

$(MAIN_O): $(MAIN_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(VGA_O): $(VGA_C) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ISO generation
$(ISO): $(KERNEL_ELF) $(BOOT_DIR)/grub/grub.cfg | $(ISO_GRUB_DIR)
	@echo "Copying kernel to ISO staging directory..."
	cp $(KERNEL_ELF) $(ISO_KERNEL)
	cp $(BOOT_DIR)/grub/grub.cfg $(ISO_GRUB_CFG)
	@echo "Generating bootable ISO..."
	$(GRUB_MKRESCUE) -o $@ $(ISO_DIR)
	@echo "ISO created: $@"

$(ISO_GRUB_DIR): | $(BUILD_DIR)
	mkdir -p $(ISO_GRUB_DIR)

clean:
	rm -rf $(BUILD_DIR)
