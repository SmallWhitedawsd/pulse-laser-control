/* output.h — consumer: clones pulse width, inserts user-defined gap */

#ifndef __OUTPUT_H
#define __OUTPUT_H

#include "stm32f10x.h"

extern volatile uint32_t gap_ms;   /* inter-pulse gap in ms, settable via UART */

void Output_Init(void);
void Output_Poll(void);

#endif
