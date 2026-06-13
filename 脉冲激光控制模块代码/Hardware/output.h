/* output.h — real-time pulse gate */

#ifndef __OUTPUT_H
#define __OUTPUT_H

#include "stm32f10x.h"

extern volatile uint32_t burst_count;
extern volatile uint32_t burst_delay;
extern volatile uint32_t input_freq_hz;   /* computed in Output_Poll, 0=unknown */
extern volatile uint32_t raw_period_ticks;/* ISR stores raw rising-edge period */

void Output_Init(void);
void Output_Poll(void);
void Output_Resync(void);

#endif
