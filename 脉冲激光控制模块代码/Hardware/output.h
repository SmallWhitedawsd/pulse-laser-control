/* output.h — consumer: clones pulse width, gap depends on mode */

#ifndef __OUTPUT_H
#define __OUTPUT_H

#include "stm32f10x.h"

extern volatile uint32_t gap_ms;   /* 0=passthrough, >0=fixed-gap in ms, settable via UART */

void Output_Init(void);
void Output_Poll(void);

#endif
