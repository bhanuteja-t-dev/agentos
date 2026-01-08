# AgentOS

An agent-first operating system focused on running AI agents safely with strict capability controls and auditability.

## Overview

AgentOS is a minimal OS/runtime designed for running long-lived AI agents with strict capability controls and auditability. The OS treats agents as first-class citizens, not just processes.

**Current Status (Week 1 Complete):**
- Bootable i386 Multiboot2 kernel
- VGA text-mode output (80x25)
- Prints "Hello AgentOS" on boot
- GRUB bootloader support
- QEMU emulation support

## Prerequisites

- **macOS** (Apple Silicon or Intel)
- **clang** - C compiler (comes with Xcode Command Line Tools)
- **lld** - Linker (comes with clang)
- **i686-elf-grub** - GRUB for cross-compilation (install via Homebrew: `brew install i686-elf-grub`)
- **qemu-system-i386** - QEMU emulator (install via Homebrew: `brew install qemu`)
- **xorriso** - ISO creation tool (install via Homebrew: `brew install xorriso`)

## Quick Start

### Build and Run

```bash
make kernel    # Build kernel ELF (build/kernel.elf)
make iso       # Generate bootable ISO (build/agentos.iso)
make run       # Boot in QEMU
```

When you run `make run`, the kernel will:
1. Boot via GRUB
2. Display "Hello AgentOS" on the first line of the VGA text buffer
3. Halt in an infinite loop

### Debugging

```bash
make debug     # Start QEMU in debug mode (paused, waiting for GDB)
# In another terminal:
gdb build/kernel.elf -x tools/gdb-init
```

See [docs/dev-setup.md](docs/dev-setup.md) for detailed debugging instructions.

## Make Targets

- `make kernel` - Build the kernel ELF (`build/kernel.elf`)
- `make iso` - Build kernel and generate bootable ISO (`build/agentos.iso`)
- `make run` - Build ISO and boot in QEMU
- `make debug` - Build ISO and start QEMU in debug mode (GDB server on port 1234)
- `make clean` - Remove all build artifacts

## Project Structure

```
agentos/
├── boot/
│   ├── grub/
│   │   └── grub.cfg          # GRUB bootloader configuration
│   └── linker.ld             # Kernel linker script
├── build/                    # Build artifacts (git-ignored)
│   ├── kernel.elf            # Compiled kernel
│   └── agentos.iso          # Bootable ISO image
├── docs/                     # Documentation
│   ├── dev-setup.md          # Development setup and debugging
│   ├── philosophy.md         # Project philosophy
│   └── roadmap.md            # Development roadmap
├── kernel/                   # Kernel source code
│   ├── arch/
│   │   └── x86_64/
│   │       └── entry.S       # Multiboot2 entry point (i386 assembly)
│   ├── main.c                # Kernel main entry point
│   ├── vga.c                 # VGA text-mode driver
│   └── vga.h                 # VGA driver header
├── tools/                    # Development tools
│   └── gdb-init              # GDB initialization script
└── Makefile                  # Build system
```

## Technical Details

- **Architecture**: i386 (32-bit)
- **Boot Protocol**: Multiboot2
- **Bootloader**: GRUB
- **Toolchain**: clang + lld
- **Target**: freestanding (no libc)
- **Emulation**: QEMU i386

## Documentation

- [Development Setup](docs/dev-setup.md) - Setup, building, and debugging
- [Philosophy](docs/philosophy.md) - Project design principles
- [Roadmap](docs/roadmap.md) - Development roadmap

## License

See [LICENSE](LICENSE) file for details.
