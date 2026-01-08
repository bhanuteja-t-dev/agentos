// AgentOS Kernel Main Entry Point

void kernel_main(void) {
    // Simple infinite loop - halt the CPU
    // This will be replaced with actual kernel logic in future iterations
    while (1) {
        __asm__ volatile ("hlt");
    }
}
