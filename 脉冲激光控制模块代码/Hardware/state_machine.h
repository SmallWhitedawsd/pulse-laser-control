#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include "stm32f10x.h"

typedef enum {
	IDLE,
	PULSE_OUT,
	GAP_WAIT
} OutState;

extern volatile OutState g_out_state;
extern volatile uint32_t g_gap_ms;
extern volatile uint32_t g_out_count;

void process_pulse_output(void);

#endif
