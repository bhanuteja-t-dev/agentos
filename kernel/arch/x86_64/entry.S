# AgentOS Kernel Entry Point
# Note: Despite folder name (x86_64), this contains i386 assembly

.section .multiboot2
.align 8

# Multiboot2 Header
multiboot2_header_start:
    .long 0xe85250d6                # Multiboot2 magic number
    .long 0                          # Architecture: i386 (0)
    .long multiboot2_header_end - multiboot2_header_start  # Header length
    .long -(0xe85250d6 + 0 + (multiboot2_header_end - multiboot2_header_start))  # Checksum

    # End tag
    .short 0                        # Type: end tag (0)
    .short 0                        # Flags: none
    .long 8                         # Size: 8 bytes
multiboot2_header_end:

.section .text
.global _start
_start:
    # Set up stack pointer
    # Stack grows downward from stack_top
    movl $stack_top, %esp
    movl %esp, %ebp

    # Call kernel_main
    # i386 calling convention: parameters on stack, caller cleans up
    # kernel_main takes no parameters
    call kernel_main

    # If kernel_main ever returns (shouldn't), halt
halt_loop:
    hlt
    jmp halt_loop

.section .bss
.align 16
# Stack space: 16KB
stack_bottom:
    .skip 16384
stack_top:
