/* capture.c - TIM2 CH1 double-edge capture on PA0 (pull-down)
 *
 * Producer: ISR measures input high-pulse width (μs), pushes to fifo.
 * Does NOT touch PA1 or state machine — fully decoupled.
 */

#include "capture.h"
#include "fifo.h"

static volatile uint16_t start = 0;

void Capture_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA0 pull-down → prevent floating noise */
	GPIO_InitTypeDef g = {0};
	g.GPIO_Mode  = GPIO_Mode_IPD;
	g.GPIO_Pin   = GPIO_Pin_0;
	GPIO_Init(GPIOA, &g);

	/* TIM2: 72 MHz, /1, up-count */
	TIM_TimeBaseInitTypeDef t = {0};
	t.TIM_Period    = 65535;
	t.TIM_Prescaler = 0;
	t.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &t);

	/* CH1: direct input capture, start on rising edge */
	TIM_ICInitTypeDef ic = {0};
	ic.TIM_Channel     = TIM_Channel_1;
	ic.TIM_ICPolarity  = TIM_ICPolarity_Rising;
	ic.TIM_ICSelection = TIM_ICSelection_DirectTI;
	ic.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInit(TIM2, &ic);

	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

	NVIC_InitTypeDef n = {0};
	n.NVIC_IRQChannel    = TIM2_IRQn;
	n.NVIC_IRQChannelCmd = ENABLE;
	n.NVIC_IRQChannelPreemptionPriority = 0;
	n.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&n);
}

void Capture_Start(void)
{
	fifo_reset();
	TIM_SetCounter(TIM2, 0);
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
	TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) == SET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
		uint16_t now = TIM_GetCapture1(TIM2);

		if (TIM2->CCER & TIM_CCER_CC1P) {
			/* falling edge → width μs, force unsigned to survive TIM2 16-bit wrap */
			uint16_t w = (uint16_t)(now - start) / 72U;
			if (w > 0 && !fifo_full()) {
				fifo_push(w);
			}
		} else {
			/* rising edge → record start */
			start = now;
		}
		TIM2->CCER ^= TIM_CCER_CC1P;
	}
}
