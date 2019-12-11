#ifndef _PARSING_H
#define _PARSING_H

#include "types.h"
#include "pcb.h"


#define SPACE           ' '
#define NULL_CHAR       '\0'
void parseString(const char* input, char* command, uint8_t args[ARG_LIMIT]);
#endif




