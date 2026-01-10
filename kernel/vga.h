// AgentOS VGA Text-Mode Output
// Week 2 Day 1: Cursor-based console output

#ifndef VGA_H
#define VGA_H

// VGA dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Clear the VGA screen and reset cursor to top-left
void vga_clear(void);

// Write a null-terminated ASCII string to VGA buffer
// Supports '\n' for newlines - automatically advances to next row
// Cursor advances on each character and wraps to next line on '\n'
void vga_write(const char* s);

#endif // VGA_H
