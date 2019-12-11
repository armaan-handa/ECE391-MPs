#include "types.h"

#define PIT_COMMAND	0x43
#define PIT_CHAN0	  0x40
#define PIT_IRQ_NUM 0
#define SQUARE_WAVE 0x36
#define _20HZ       59659
#define _100HZ      11931

extern void switch_task (int32_t new_pid);
extern void init_pit();
extern void pit_handler();
