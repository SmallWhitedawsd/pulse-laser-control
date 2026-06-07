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
extern volatile uint8_t  test_mode;     /* 1=测试模式, 0=正常 */

void process_pulse_output(void);
void start_test_output(void);
void stop_test_output(void);

#endif
