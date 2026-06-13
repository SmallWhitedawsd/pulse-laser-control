/* output.h — real-time pulse gate */

#ifndef __OUTPUT_H
#define __OUTPUT_H

#include "stm32f10x.h"

extern volatile uint32_t burst_count;
extern volatile uint32_t burst_delay;
extern volatile uint32_t input_freq_hz;   /* measured input frequency, 0 = unknown */

void Output_Init(void);
void Output_Poll(void);
void Output_Resync(void);

#endif
