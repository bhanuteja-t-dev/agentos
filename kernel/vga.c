// AgentOS VGA Text-Mode Output Implementation
// Week 1 Day 4: Minimal freestanding VGA driver

// VGA text buffer starts at physical address 0xB8000
// Each character is 2 bytes: [character] [attribute]
// Attribute format: [blink|bg|fg] where bg/fg are 4-bit color codes
// 80 columns x 25 rows = 2000 characters total

#define VGA_BUFFER_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

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

void vga_write(const char* s) {
    // Cast VGA buffer address to volatile pointer
    // Volatile prevents compiler optimizations that might skip writes
    volatile unsigned short* vga_buffer = (volatile unsigned short*)VGA_BUFFER_ADDR;
    
    // Write to first line (row 0, columns 0-79)
    unsigned int col = 0;
    while (s[col] != '\0' && col < VGA_WIDTH) {
        // Each VGA entry is: character byte | (attribute byte << 8)
        vga_buffer[col] = VGA_ENTRY(s[col], VGA_DEFAULT_ATTR);
        col++;
    }
    
    // Clear remaining characters on first line if string is shorter than 80 chars
    while (col < VGA_WIDTH) {
        vga_buffer[col] = VGA_ENTRY(' ', VGA_DEFAULT_ATTR);
        col++;
    }
}
