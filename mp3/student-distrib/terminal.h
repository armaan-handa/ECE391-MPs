#ifndef _TERMINAL_H
#define _TERMINAL_H
#endif

#include "types.h"

#define VGA_PORT            0x3C0
#define RESET_INDEX_MODE    0x3DA

extern int32_t open_terminal(const uint8_t* filename);
extern int32_t close_terminal(uint32_t fd);
extern int32_t write_terminal(uint32_t fd, const void* buf, uint32_t nbytes);
extern int32_t read_terminal(uint32_t fd, void* buf, uint32_t nbytes);
