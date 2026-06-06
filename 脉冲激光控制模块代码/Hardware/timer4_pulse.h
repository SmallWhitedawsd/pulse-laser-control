#ifndef __TIMER4_PULSE_H
#define __TIMER4_PULSE_H

#include "stm32f10x.h"

void Timer4_Pulse_Init(void);
void Timer4_Start_us(uint16_t width_us);

extern volatile uint8_t timer4_expired;

#endif
