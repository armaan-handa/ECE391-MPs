#include "types.h"
#include "paging.h"
#include "lib.h"
#ifndef _ASM
#define VMEM_BUFFERS (VIDEO + FOUR_KI_B)


extern int current_terminal;
extern int terminal_processes[3];
extern uint8_t* vmem_buffers[3];
extern uint8_t* map_loc;
extern void swap_terminal (int terminal);
extern int32_t halt_syscall (uint8_t status);
extern int32_t execute_syscall (const uint8_t* command);
extern int32_t read_syscall (int32_t fd, void* buf, int32_t nbytes);
extern int32_t write_syscall (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open_syscall (const uint8_t* filename);
extern int32_t close_syscall (int32_t fd);
extern int32_t getargs_syscall (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap_syscall (uint8_t** screen_start);
extern int32_t set_handler_syscall (int32_t signum, void* handler);
extern int32_t sigreturn_syscall (void);
#endif
