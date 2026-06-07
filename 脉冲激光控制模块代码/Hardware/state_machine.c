#include "state_machine.h"
#include "fifo.h"
#include "timer4_pulse.h"
#include "timer3_gap.h"

volatile OutState g_out_state = IDLE;
volatile uint32_t g_gap_ms = 100;
volatile uint8_t  test_mode = 0;

void process_pulse_output(void)
{
	if (test_mode) return;  /* test mode handles PA1 on its own */

	switch (g_out_state) {

	case IDLE:
		if (!FIFO_IS_EMPTY()) {
			uint16_t current_width = fifo_pop();

			GPIO_SetBits(GPIOA, GPIO_Pin_1);
			Timer4_Start_us(current_width);
			g_out_state = PULSE_OUT;
		}
		break;

	case PULSE_OUT:
		if (timer4_expired) {
			timer4_expired = 0;

			GPIO_ResetBits(GPIOA, GPIO_Pin_1);
			deadtime_start(g_gap_ms);
			g_out_state = GAP_WAIT;
		}
		break;

	case GAP_WAIT:
		if (deadtime_remaining_ms == 0) {
			g_out_state = IDLE;
		}
		break;
	}
}

/* ── test mode: 1Hz @ 50% duty direct toggle via TIM3 ── */

void start_test_output(void)
{
	test_mode = 1;
	/* stop any ongoing normal output */
	TIM_Cmd(TIM4, DISABLE);
	timer4_expired = 0;
	g_out_state = IDLE;

	/* reprogram TIM3 for 500ms period → 1Hz square wave via ISR */
	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC = 7200 - 1;        /* 10KHz tick */
	TIM3->ARR = 5000 - 1;        /* 5000 * 0.1ms = 500ms */
	TIM_SetCounter(TIM3, 0);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
	TIM_Cmd(TIM3, ENABLE);

	GPIO_SetBits(GPIOA, GPIO_Pin_1);   /* start HIGH */
}

void stop_test_output(void)
{
	test_mode = 0;
	TIM_Cmd(TIM3, DISABLE);
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);

	/* restore TIM3 for gap duty */
	TIM3->PSC = 72000 - 1;
	TIM3->ARR = 1000 - 1;
	deadtime_remaining_ms = 0;
}
