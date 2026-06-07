#include "state_machine.h"
#include "fifo.h"
#include "timer4_pulse.h"
#include "timer3_gap.h"

volatile OutState g_out_state = IDLE;
volatile uint32_t g_gap_ms = 100;	/* 默认间隙 100ms */
volatile uint32_t g_out_count = 0;	/* 已输出脉冲计数 */

void process_pulse_output(void)
{
	switch (g_out_state) {

	case IDLE:
		if (!FIFO_IS_EMPTY()) {
			uint16_t current_width = fifo_pop();

			GPIO_SetBits(GPIOA, GPIO_Pin_1);		/* PA1 置高 */
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);		/* LED亮 */
			Timer4_Start_us(current_width);			/* TIM4 计时脉宽 */
			g_out_state = PULSE_OUT;
		}
		break;

	case PULSE_OUT:
		if (timer4_expired) {
			timer4_expired = 0;

			GPIO_ResetBits(GPIOA, GPIO_Pin_1);		/* PA1 置低 */
			GPIO_SetBits(GPIOC, GPIO_Pin_13);		/* LED灭 */
			g_out_count++;					/* 脉冲计数+1 */
			deadtime_start(g_gap_ms);			/* 启动间隙等待 */
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
