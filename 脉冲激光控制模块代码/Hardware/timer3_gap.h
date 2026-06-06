#ifndef __TIMER3_GAP_H
#define __TIMER3_GAP_H

#include "stm32f10x.h"

void Timer3_Gap_Init(void);
void deadtime_start(uint32_t gap_ms);

extern volatile uint32_t deadtime_remaining_ms;

#endif
