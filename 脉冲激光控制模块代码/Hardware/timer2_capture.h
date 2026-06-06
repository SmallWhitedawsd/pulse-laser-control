#ifndef __TIMER2_CAPTURE_H
#define __TIMER2_CAPTURE_H

#include "stm32f10x.h"

void Timer2_Capture_Init(void);

extern volatile uint8_t timer2_first_pulse_bypassed;

#endif
