# AgentOS Development Setup

## Prerequisites

### Required Tools

Install the following tools on macOS:

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required packages
brew install i686-elf-grub qemu xorriso
```

### Verify Installation

```bash
# Check that all tools are available
which clang          # Should be in /usr/bin/clang or /opt/homebrew/bin/clang
which ld.lld         # Should be in /usr/bin/ld.lld or /opt/homebrew/bin/ld.lld
which i686-elf-grub-mkrescue  # Should be in /opt/homebrew/bin/i686-elf-grub-mkrescue
which qemu-system-i386        # Should be in /opt/homebrew/bin/qemu-system-i386
which xorriso                 # Should be in /opt/homebrew/bin/xorriso
```

## Build System

### Compilation Flags

The kernel is built with freestanding flags (no libc):

- `-target i386-elf` - Target i386 architecture
- `-ffreestanding` - Freestanding environment (no standard library)
- `-fno-builtin` - Disable built-in functions
- `-nostdlib` - Don't link standard library
- `-fno-stack-protector` - Disable stack protection
- `-Wall -Wextra -Werror` - Enable all warnings as errors
- `-g` - Include debug symbols

### Build Process

1. **Assemble entry point** (`kernel/arch/x86_64/entry.S`)
   - Creates Multiboot2 header
   - Sets up stack
   - Calls `kernel_main`

2. **Compile C sources** (`kernel/main.c`, `kernel/vga.c`)
   - Compiles with freestanding flags
   - No standard library dependencies

3. **Link kernel** (`boot/linker.ld`)
   - Links at 1MiB (0x100000)
   - Preserves Multiboot2 header section
   - Creates `build/kernel.elf`

4. **Generate ISO** (`i686-elf-grub-mkrescue`)
   - Copies kernel to ISO staging directory
   - Copies GRUB configuration
   - Creates bootable ISO image

## Development Workflow

### Building the Kernel

```bash
# Clean previous build
make clean

# Build kernel ELF
make kernel

# Verify the output
file build/kernel.elf
# Expected: ELF 32-bit LSB executable, Intel 80386

# Check symbols
nm build/kernel.elf | grep -E "(kernel_main|vga_write)"
# Should show both symbols
```

### Creating Bootable ISO

```bash
# Build ISO (automatically builds kernel if needed)
make iso

# Verify ISO was created
ls -lh build/agentos.iso
```

### Running in QEMU

```bash
# Build and run
make run
```

**Expected Behavior:**
- QEMU window opens (or runs in terminal if using `-nographic`)
- GRUB menu appears briefly
- Kernel boots
- "Hello AgentOS" appears on the first line of the VGA text buffer
- Kernel halts in infinite loop

**To exit QEMU:**
- Press `Ctrl+A` then `X`, or
- Close the QEMU window

### QEMU Run Flags

The `make run` target uses:
- `-cdrom build/agentos.iso` - Boot from ISO
- `-m 128M` - 128MB RAM
- `-serial stdio` - Serial output to terminal
- `-boot d` - Boot from CD/DVD
- `-no-reboot` - Exit on shutdown
- `-no-shutdown` - Keep running on halt

## Debugging with GDB

### Quick Start

1. **Start QEMU in debug mode:**
   ```bash
   make debug
   ```
   This will:
   - Build the ISO (if needed)
   - Start QEMU paused
   - Wait for GDB connection on port 1234

2. **In another terminal, connect with GDB:**
   ```bash
   gdb build/kernel.elf -x tools/gdb-init
   ```
   This will:
   - Load kernel symbols from `build/kernel.elf`
   - Execute commands from `tools/gdb-init`:
     - Connect to QEMU at `:1234`
     - Set a breakpoint at `kernel_main`
     - Continue execution

3. **GDB will pause at `kernel_main`** - You can now:
   - Inspect registers: `info registers`
   - Step through code: `step` or `next`
   - Inspect memory: `x/10x $esp`
   - Continue: `continue`
   - Quit: `quit`

### Manual GDB Commands

If you prefer to run GDB manually:

```bash
gdb build/kernel.elf
```

Then in GDB:
```
(gdb) target remote :1234
(gdb) break kernel_main
(gdb) continue
```

### GDB Command File

The `tools/gdb-init` file contains:
- `target remote :1234` - Connect to QEMU's GDB server
- `break kernel_main` - Set breakpoint at kernel entry point
- `continue` - Resume execution until breakpoint

You can modify `tools/gdb-init` to add additional breakpoints or commands as needed.

### Debug Flags

The `make debug` target uses the same flags as `make run`, plus:
- `-S` - Freeze CPU at startup (start paused)
- `-s` - Shorthand for `-gdb tcp::1234` (GDB server on port 1234)

## Troubleshooting

### Build Issues

**Error: "clang: error: unsupported option '-m elf_i386'"**
- This is normal - the linker flag is for `ld.lld`, not clang
- Make sure `ld.lld` is available: `which ld.lld`

**Error: "i686-elf-grub-mkrescue: command not found"**
- Install GRUB: `brew install i686-elf-grub`
- Verify: `which i686-elf-grub-mkrescue`

### Runtime Issues

**QEMU doesn't boot**
- Verify ISO was created: `ls -lh build/agentos.iso`
- Check QEMU version: `qemu-system-i386 --version` (may fail on Apple Silicon, but binary should work)
- Try rebuilding: `make clean && make iso && make run`

**No VGA output visible**
- QEMU may be using a different display mode
- Try adding `-nographic` to see serial output
- Check that VGA buffer is being written (use GDB to inspect memory at 0xB8000)

### Debugging Issues

**GDB can't connect**
- Make sure QEMU is running in debug mode: `make debug`
- Check that QEMU is waiting (should say "Waiting for gdb connection")
- Verify port 1234 is not in use: `lsof -i :1234`

**Breakpoints not working**
- Make sure kernel was built with `-g` flag (included in Makefile)
- Verify symbols exist: `nm build/kernel.elf | grep kernel_main`
- Try setting breakpoint after connecting: `(gdb) break kernel_main`

## Current Kernel Features

### Week 1 Day 4

- **Multiboot2 Boot**: Kernel boots via GRUB Multiboot2 protocol
- **VGA Text Output**: Writes to VGA text buffer at 0xB8000
- **Minimal C Runtime**: Freestanding C with no libc dependencies
- **Stack Setup**: Assembly entry point sets up stack before calling C code
- **Halt Loop**: Kernel halts CPU in infinite loop after initialization

### Kernel Components

- **`kernel/arch/x86_64/entry.S`**: Assembly entry point with Multiboot2 header
- **`kernel/main.c`**: Main kernel entry point, calls VGA writer
- **`kernel/vga.c`**: VGA text-mode driver (writes to 0xB8000)
- **`kernel/vga.h`**: VGA driver interface

## Next Steps

See [roadmap.md](roadmap.md) for planned features and development timeline.
