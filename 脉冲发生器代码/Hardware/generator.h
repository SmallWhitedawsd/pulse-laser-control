#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "stm32f10x.h"

extern volatile uint32_t gen_freq_hz;
extern volatile uint32_t gen_duty_pct;
extern volatile uint32_t gen_pulse_count;
extern volatile uint8_t  gen_running;
extern volatile uint32_t gen_current_pulse;
extern volatile uint32_t gen_total_rounds;

void Generator_Init(void);
void Generator_UpdateParams(void);

#endif
