#include "vga.h"

/* Buffer de video en memoria — dirección fija en x86 */
#define VGA_BUFFER ((uint16_t *)0xB8000)

static uint8_t  vga_col;
static uint8_t  vga_row;
static uint8_t  vga_color;

static uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static uint16_t make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void update_cursor(void) {
    uint16_t pos = vga_row * VGA_COLS + vga_col;
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0F), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)(pos & 0xFF)), "Nd"((uint16_t)0x3D5));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x0E), "Nd"((uint16_t)0x3D4));
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)((pos >> 8) & 0xFF)), "Nd"((uint16_t)0x3D5));
}

static void scroll(void) {
    for (int row = 1; row < VGA_ROWS; row++)
        for (int col = 0; col < VGA_COLS; col++)
            VGA_BUFFER[(row - 1) * VGA_COLS + col] = VGA_BUFFER[row * VGA_COLS + col];
    for (int col = 0; col < VGA_COLS; col++)
        VGA_BUFFER[(VGA_ROWS - 1) * VGA_COLS + col] = make_entry(' ', vga_color);
    vga_row = VGA_ROWS - 1;
}

void vga_init(void) {
    vga_col   = 0;
    vga_row   = 0;
    vga_color = make_color(VGA_LIGHT_GRAY, VGA_BLACK);
    vga_clear();
}

void vga_clear(void) {
    for (int row = 0; row < VGA_ROWS; row++)
        for (int col = 0; col < VGA_COLS; col++)
            VGA_BUFFER[row * VGA_COLS + col] = make_entry(' ', vga_color);
    vga_col = 0;
    vga_row = 0;
    update_cursor();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = make_color(fg, bg);
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            /* Solo mover el cursor, NO borrar el caracter que hay ahi */
        }
    } else {
        VGA_BUFFER[vga_row * VGA_COLS + vga_col] = make_entry(c, vga_color);
        vga_col++;
        if (vga_col >= VGA_COLS) {
            vga_col = 0;
            vga_row++;
        }
    }
    if (vga_row >= VGA_ROWS)
        scroll();
    update_cursor();
}

void vga_print(const char *str) {
    while (*str) vga_putchar(*str++);
}

void vga_println(const char *str) {
    vga_print(str);
    vga_putchar('\n');
}

void vga_print_color(const char *str, uint8_t fg, uint8_t bg) {
    uint8_t prev = vga_color;
    vga_color = make_color(fg, bg);
    vga_print(str);
    vga_color = prev;
}

void vga_print_int(int n) {
    if (n < 0) { vga_putchar('-'); n = -n; }
    if (n == 0) { vga_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i - 1; j >= 0; j--) vga_putchar(buf[j]);
}
