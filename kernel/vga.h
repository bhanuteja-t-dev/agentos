// AgentOS VGA Text-Mode Output
// Week 1 Day 4: Minimal freestanding VGA driver

#ifndef VGA_H
#define VGA_H

// Write a null-terminated ASCII string to the first line of VGA text buffer
void vga_write(const char* s);

#endif // VGA_H
