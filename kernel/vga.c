// AgentOS VGA Text-Mode Output Implementation
// Week 2 Day 1: Cursor-based console output

#include "vga.h"

// VGA text buffer starts at physical address 0xB8000
// Each character is 2 bytes: [character] [attribute]
// Attribute format: [blink|bg|fg] where bg/fg are 4-bit color codes
// 80 columns x 25 rows = 2000 characters total

#define VGA_BUFFER_ADDR 0xB8000

// Color attributes (4-bit foreground/background)
#define VGA_COLOR_BLACK   0
#define VGA_COLOR_BLUE    1
#define VGA_COLOR_GREEN   2
#define VGA_COLOR_CYAN    3
#define VGA_COLOR_RED     4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN   6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY  8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN  11
#define VGA_COLOR_LIGHT_RED  12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_YELLOW  14
#define VGA_COLOR_WHITE   15

// Create attribute byte: foreground | (background << 4)
#define VGA_ENTRY(ch, attr) ((unsigned short)(ch) | ((unsigned short)(attr) << 8))

// Default attribute: light grey on black
#define VGA_DEFAULT_ATTR (VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4))

// Global cursor position (row, col)
static unsigned int vga_cursor_row = 0;
static unsigned int vga_cursor_col = 0;

// Get VGA buffer pointer
static volatile unsigned short* vga_get_buffer(void) {
    return (volatile unsigned short*)VGA_BUFFER_ADDR;
}

// Clear a single row
static void vga_clear_row(unsigned int row) {
    volatile unsigned short* vga_buffer = vga_get_buffer();
    unsigned int start_pos = row * VGA_WIDTH;
    for (unsigned int col = 0; col < VGA_WIDTH; col++) {
        vga_buffer[start_pos + col] = VGA_ENTRY(' ', VGA_DEFAULT_ATTR);
    }
}

// Clear the entire VGA screen and reset cursor
void vga_clear(void) {
    for (unsigned int row = 0; row < VGA_HEIGHT; row++) {
        vga_clear_row(row);
    }
    vga_cursor_row = 0;
    vga_cursor_col = 0;
}

// Write a single character at current cursor position and advance cursor
static void vga_putchar(char c) {
    volatile unsigned short* vga_buffer = vga_get_buffer();
    
    // Handle newline
    if (c == '\n') {
        // Move to next row, column 0
        vga_cursor_row++;
        vga_cursor_col = 0;
        
        // If we've scrolled past the bottom, wrap to top (or scroll up)
        // For simplicity, wrap to top row
        if (vga_cursor_row >= VGA_HEIGHT) {
            vga_cursor_row = 0;
        }
        return;
    }
    
    // Check if we're past screen bounds
    if (vga_cursor_row >= VGA_HEIGHT) {
        // Wrap to top row if past bottom
        vga_cursor_row = 0;
        vga_cursor_col = 0;
    }
    
    // Calculate position in buffer
    unsigned int pos = vga_cursor_row * VGA_WIDTH + vga_cursor_col;
    
    // Write character
    vga_buffer[pos] = VGA_ENTRY(c, VGA_DEFAULT_ATTR);
    
    // Advance cursor column
    vga_cursor_col++;
    
    // Handle line wrap (if column exceeds width, move to next row)
    if (vga_cursor_col >= VGA_WIDTH) {
        vga_cursor_col = 0;
        vga_cursor_row++;
        
        // Wrap to top if past bottom
        if (vga_cursor_row >= VGA_HEIGHT) {
            vga_cursor_row = 0;
        }
    }
}

// Write a null-terminated string using cursor-based output
// Supports '\n' for newlines
void vga_write(const char* s) {
    unsigned int i = 0;
    while (s[i] != '\0') {
        vga_putchar(s[i]);
        i++;
    }
}
