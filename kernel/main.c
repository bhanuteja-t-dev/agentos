// AgentOS Kernel Main Entry Point
// Week 1 Day 4: Minimal freestanding kernel

#include "vga.h"

void kernel_main(void) {
    // Write message to VGA text buffer
    vga_write("Hello AgentOS");
    
    // Halt the CPU in infinite loop
    while (1) {
        __asm__ volatile ("hlt");
    }
}
