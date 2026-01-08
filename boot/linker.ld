/* AgentOS Linker Script
 * Links kernel at 1MiB (0x100000) for Multiboot2 compatibility
 */

ENTRY(_start)

SECTIONS
{
    /* Load kernel at 1MiB - standard location for Multiboot kernels */
    . = 1M;

    /* Multiboot2 header must be at the start */
    .multiboot2 : ALIGN(8) {
        *(.multiboot2)
    }

    /* Code section */
    .text : ALIGN(4K) {
        *(.text)
    }

    /* Read-only data */
    .rodata : ALIGN(4K) {
        *(.rodata)
    }

    /* Initialized data */
    .data : ALIGN(4K) {
        *(.data)
    }

    /* Uninitialized data (BSS) */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    /* Discard everything else */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
    }
}
