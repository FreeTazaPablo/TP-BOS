#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "vga.h"

/* Códigos especiales (> 127, no son ASCII) */
#define KEY_UP    0x80
#define KEY_DOWN  0x81
#define KEY_LEFT  0x82
#define KEY_RIGHT 0x83
#define KEY_HOME  0x84
#define KEY_END   0x85
#define KEY_TAB   0x09   /* ASCII normal */

void kb_init(void);
/* Devuelve ASCII normal, carácter de control (Ctrl+X),
   o uno de los KEY_* de arriba para teclas especiales */
unsigned char kb_getchar(void);

#endif
