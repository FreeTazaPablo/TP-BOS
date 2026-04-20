#include "keyboard.h"

/* ── Ports PS/2 ────────────────────────────────────────────────────────────── */
#define KB_DATA   0x60
#define KB_STATUS 0x64

/* ── Helpers I/O ─────────────────────────────────────────────────────────────*/
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ── Mapa de scancodes (Set 1) → ASCII, layout ES España ─────────────────── */
/*
   Distribución física ES:
   Fila números: º 1 2 3 4 5 6 7 8 9 0 ' ¡
   Fila Q:       q w e r t y u i o p ` +
   Fila A:       a s d f g h j k l ñ ´ ç
   Fila Z:       < z x c v b n m , . -
*/
static const char sc_normal[128] = {
/*00*/  0,
/*01*/  0,        /* Esc        */
/*02*/  '1',
/*03*/  '2',
/*04*/  '3',
/*05*/  '4',
/*06*/  '5',
/*07*/  '6',
/*08*/  '7',
/*09*/  '8',
/*0A*/  '9',
/*0B*/  '0',
/*0C*/  '\'',     /* ' (apóstrofo, tecla entre 0 y ¡) */
/*0D*/  0,        /* ¡ — sin ASCII directo, ignorar */
/*0E*/  '\b',
/*0F*/  '\t',
/*10*/  'q',
/*11*/  'w',
/*12*/  'e',
/*13*/  'r',
/*14*/  't',
/*15*/  'y',
/*16*/  'u',
/*17*/  'i',
/*18*/  'o',
/*19*/  'p',
/*1A*/  '`',      /* ` (tecla entre p y +) */
/*1B*/  '+',
/*1C*/  '\n',
/*1D*/  0,        /* LCtrl */
/*1E*/  'a',
/*1F*/  's',
/*20*/  'd',
/*21*/  'f',
/*22*/  'g',
/*23*/  'h',
/*24*/  'j',
/*25*/  'k',
/*26*/  'l',
/*27*/  0,        /* ñ — sin ASCII, ignorar por ahora */
/*28*/  0,        /* ´ (acento agudo muerto) */
/*29*/  0,        /* º */
/*2A*/  0,        /* LShift */
/*2B*/  0,        /* ç — sin ASCII directo */
/*2C*/  'z',
/*2D*/  'x',
/*2E*/  'c',
/*2F*/  'v',
/*30*/  'b',
/*31*/  'n',
/*32*/  'm',
/*33*/  ',',
/*34*/  '.',
/*35*/  '-',
/*36*/  0,        /* RShift */
/*37*/  '*',      /* Numpad * */
/*38*/  0,        /* LAlt */
/*39*/  ' ',
/*3A*/  0,        /* CapsLock */
/*3B-44*/ 0,0,0,0,0,0,0,0,0,0,  /* F1-F10 */
/*45*/  0,        /* NumLock */
/*46*/  0,        /* ScrollLock */
/*47*/  '7','8','9','-',
/*4B*/  '4','5','6','+',
/*4F*/  '1','2','3','0','.', 0,0,
/*56*/  '<',      /* Tecla < > entre LShift y Z (ES tiene esta tecla extra) */
/*57*/  0,        /* F11 */
/*58*/  0,        /* F12 */
        0,0,0,0,0,0,0  /* padding hasta 128 */
};

static const char sc_shifted[128] = {
/*00*/  0,
/*01*/  0,
/*02*/  '!',
/*03*/  '"',      /* Shift+2 = " en ES */
/*04*/  0,        /* Shift+3 = · en ES (sin ASCII simple) */
/*05*/  '$',
/*06*/  '%',
/*07*/  '&',
/*08*/  '/',
/*09*/  '(',
/*0A*/  ')',
/*0B*/  '=',
/*0C*/  '?',      /* Shift+' = ? */
/*0D*/  0,
/*0E*/  '\b',
/*0F*/  '\t',
/*10*/  'Q',
/*11*/  'W',
/*12*/  'E',
/*13*/  'R',
/*14*/  'T',
/*15*/  'Y',
/*16*/  'U',
/*17*/  'I',
/*18*/  'O',
/*19*/  'P',
/*1A*/  '^',      /* Shift+` = ^ */
/*1B*/  '*',      /* Shift++ = * */
/*1C*/  '\n',
/*1D*/  0,
/*1E*/  'A',
/*1F*/  'S',
/*20*/  'D',
/*21*/  'F',
/*22*/  'G',
/*23*/  'H',
/*24*/  'J',
/*25*/  'K',
/*26*/  'L',
/*27*/  0,        /* Shift+ñ */
/*28*/  0,        /* Shift+´ = ¨ */
/*29*/  0,
/*2A*/  0,
/*2B*/  0,        /* Shift+ç */
/*2C*/  'Z',
/*2D*/  'X',
/*2E*/  'C',
/*2F*/  'V',
/*30*/  'B',
/*31*/  'N',
/*32*/  'M',
/*33*/  ';',      /* Shift+, = ; */
/*34*/  ':',      /* Shift+. = : */
/*35*/  '_',      /* Shift+- = _ */
/*36*/  0,
/*37*/  '*',
/*38*/  0,
/*39*/  ' ',
/*3A*/  0,
/*3B-44*/ 0,0,0,0,0,0,0,0,0,0,
/*45*/  0,
/*46*/  0,
/*47*/  '7','8','9','-',
/*4B*/  '4','5','6','+',
/*4F*/  '1','2','3','0','.', 0,0,
/*56*/  '>',      /* Shift+< = > */
/*57*/  0,
/*58*/  0,
        0,0,0,0,0,0,0
};

static int shift_held  = 0;
static int ctrl_held   = 0;
static int altgr_held  = 0;   /* AltGr = RAlt = 0xE0 0x38 */
static int caps_lock   = 0;
static int extended    = 0;

/* Mapa AltGr ES España:
   AltGr+2=@  AltGr+3=#  AltGr+4=~  AltGr+5=€(usamos~alt)
   AltGr+`=[  AltGr++=]  AltGr+ç=}  AltGr+`(0x29)=\
   en ES: AltGr+1=|  AltGr+q=@(no, eso es AltGr+2)
   Mapa real ES:
   0x02(1)→|   0x03(2)→@   0x04(3)→#   0x05(4)→~
   0x1A(`)→[   0x1B(+)→]
   0x2B(ç)→}   0x28(´)→{   (en algunas variantes)
   0x29(º)→\
*/
static const char sc_altgr[128] = {
/*00*/  0,
/*01*/  0,
/*02*/  '|',      /* AltGr+1 = | */
/*03*/  '@',      /* AltGr+2 = @ */
/*04*/  '#',      /* AltGr+3 = # */
/*05*/  '~',      /* AltGr+4 = ~ */
/*06*/  0,
/*07*/  0,
/*08*/  0,
/*09*/  '[',      /* AltGr+8 = [ */
/*0A*/  ']',      /* AltGr+9 = ] */
/*0B*/  0,
/*0C*/  '\\',     /* AltGr+' = \ */
/*0D*/  0,
/*0E*/  0,
/*0F*/  0,
/*10*/  0,
/*11*/  0,
/*12*/  0,
/*13*/  0,
/*14*/  0,
/*15*/  0,
/*16*/  0,
/*17*/  0,
/*18*/  0,
/*19*/  0,
/*1A*/  0,
/*1B*/  0,
/*1C*/  0,
/*1D*/  0,
/*1E*/  0,
/*1F*/  0,
/*20*/  0,
/*21*/  0,
/*22*/  0,
/*23*/  0,
/*24*/  0,
/*25*/  0,
/*26*/  0,
/*27*/  0,
/*28*/  '{',      /* AltGr+´ = { */
/*29*/  0,
/*2A*/  0,
/*2B*/  '}',      /* AltGr+ç = } */
/*2C*/  0,
/*2D*/  0,
/*2E*/  0,
/*2F*/  0,
/*30*/  0,
/*31*/  0,
/*32*/  0,
/*33*/  0,
/*34*/  0,
/*35*/  0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0
};

void kb_init(void) {
    while (inb(KB_STATUS) & 0x01)
        inb(KB_DATA);
}

unsigned char kb_getchar(void) {
    while (1) {
        while (!(inb(KB_STATUS) & 0x01))
            __asm__ volatile ("pause");

        uint8_t sc = inb(KB_DATA);

        /* Prefijo de tecla extendida */
        if (sc == 0xE0) { extended = 1; continue; }

        /* Key-release: bit 7 set */
        if (sc & 0x80) {
            uint8_t rel = sc & 0x7F;
            if (extended) {
                if (rel == 0x38) altgr_held = 0;  /* RAlt release */
                if (rel == 0x1D) ctrl_held  = 0;  /* RCtrl release */
            } else {
                if (rel == 0x2A || rel == 0x36) shift_held = 0;
                if (rel == 0x1D)                ctrl_held  = 0;
            }
            extended = 0;
            continue;
        }

        if (extended) {
            extended = 0;
            switch (sc) {
                case 0x38: altgr_held = 1; continue;  /* RAlt = AltGr */
                case 0x1D: ctrl_held  = 1; continue;  /* RCtrl */
                case 0x48: return KEY_UP;
                case 0x50: return KEY_DOWN;
                case 0x4B: return KEY_LEFT;
                case 0x4D: return KEY_RIGHT;
                case 0x47: return KEY_HOME;
                case 0x4F: return KEY_END;
                default:   continue;
            }
        }

        /* Modificadores normales */
        if (sc == 0x2A || sc == 0x36) { shift_held = 1; continue; }
        if (sc == 0x1D)               { ctrl_held  = 1; continue; }
        if (sc == 0x3A)               { caps_lock  = !caps_lock; continue; }

        /* Tab */
        if (sc == 0x0F) return KEY_TAB;

        if (sc >= 128) continue;

        /* AltGr tiene prioridad sobre shift */
        if (altgr_held) {
            unsigned char c = sc_altgr[sc];
            if (c != 0) return c;
            continue;
        }

        /* Ctrl+letra → carácter de control (ASCII 1-26) */
        if (ctrl_held) {
            char base = sc_normal[sc];
            if (base >= 'a' && base <= 'z') return base - 'a' + 1;
            if (base >= 'A' && base <= 'Z') return base - 'A' + 1;
            continue;
        }

        unsigned char c;
        if (shift_held) {
            c = sc_shifted[sc];
        } else {
            c = sc_normal[sc];
            if (caps_lock && c >= 'a' && c <= 'z') c -= 32;
        }

        if (c == 0) continue;
        return c;
    }
}
