#ifndef _KEYBOARD_H
#define _KEYBOARD_H
#endif

#include "types.h"

#define KEYBOARD_IRQ_NUM    1
#define KEYBOARD_PORT       0x60
#define BUF_LIMIT           127

// define common scancodes
#define BACKSPACE   0x0E
#define TAB         0x0F
#define ENTER       0x1C
#define CTRL        0x1D
#define CTRL_REL    0x9D
#define L_SHIFT     0x2A
#define L_SHIFT_REL 0xAA
#define R_SHIFT     0x36
#define R_SHIFT_REL 0xB6
#define ALT         0x38
#define ALT_REL     0xB8
#define CAPS_LOCK   0x3A
#define F1          0x3B
#define F2          0x3C
#define F3          0x3D

#define RELEASED      0
#define PRESSED       1

#define NUM_KEYS      62

extern void init_keyboard();
extern void cp1_keyboard_handler();
extern void keyboard_handler();
extern void keypress(uint8_t);
extern void add_to_key_buf(char);
extern void enter();
extern void backspace();
