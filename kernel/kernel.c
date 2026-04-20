#include "vga.h"
#include "keyboard.h"
#include "fs.h"
#include "shell.h"

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void pit_disable(void) {
    outb(0x43, 0x30);
    outb(0x40, 0x00);
    outb(0x40, 0x00);
}

static void pic_disable(void) {
    outb(0xA1, 0xFF);
    outb(0x21, 0xFF);
}

void kernel_main(unsigned long mb_magic, unsigned long mb_info) {
    (void)mb_magic;
    (void)mb_info;

    __asm__ volatile ("cli");
    pic_disable();
    pit_disable();

    vga_init();
    kb_init();
    fs_init();

    /* ── Banner de arranque ──────────────────────────────────────────────── */
    vga_print_color(
        "TP-BOS v0.1 [Taza Pablo Boot Operating System]\n",
        VGA_WHITE, VGA_BLACK
    );
    vga_print_color(
        "Copyright (C) 2026. All rights reserved.\n",
        VGA_LIGHT_GRAY, VGA_BLACK
    );
    vga_putchar('\n');
    vga_print_color(
        "Escribe 'help' para ver los comandos disponibles.\n",
        VGA_DARK_GRAY, VGA_BLACK
    );
    vga_putchar('\n');

    /* ── Shell — nunca retorna ───────────────────────────────────────────── */
    shell_run();

    /* Por si acaso */
    while (1) __asm__ volatile ("pause");
}
