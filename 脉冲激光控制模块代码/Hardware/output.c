/* output.c — real-time pulse gate
 *
 * PA0 ← input signal, PA1 → output. TIM2 ISR directly mirrors PA0 → PA1.
 *
 * Gate: S_FORWARD → follow edges. After burst_count pulses → S_SILENCE.
 *       S_SILENCE → hold PA1 LOW, TIM3 counts burst_delay ms, then → S_FORWARD.
 */

#include "output.h"

volatile uint32_t burst_count = 2;
volatile uint32_t burst_delay = 2;
volatile uint32_t input_freq_hz = 0;      /* measured from TIM2 rising edges */

static volatile uint32_t pulse_cnt;
static volatile uint32_t tim3_ms;

/* frequency measurement */
static volatile uint16_t prev_rise_ccr;
static volatile uint8_t  prev_rise_valid;

typedef enum { S_FORWARD, S_SILENCE } gate_t;
static volatile gate_t gate = S_FORWARD;

void Output_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef g = {0};
	g.GPIO_Mode  = GPIO_Mode_IPD;
	g.GPIO_Pin   = GPIO_Pin_0;
	GPIO_Init(GPIOA, &g);

	g.GPIO_Mode  = GPIO_Mode_Out_PP;
	g.GPIO_Pin   = GPIO_Pin_1;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &g);
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef t = {0};
	t.TIM_Period    = 65535;
	t.TIM_Prescaler = 0;
	t.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &t);

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

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	t.TIM_Period    = 1000 - 1;
	t.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseInit(TIM3, &t);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	n.NVIC_IRQChannel    = TIM3_IRQn;
	n.NVIC_IRQChannelPreemptionPriority = 1;
	n.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&n);

	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
	TIM_Cmd(TIM2, ENABLE);
}

void Output_Poll(void) {}

void Output_Resync(void)
{
	TIM_Cmd(TIM3, DISABLE);          /* stop gap timer if running */
	gate = S_FORWARD;
	pulse_cnt = 0;
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
		TIM2->CCER |= TIM_CCER_CC1P;
	else
		TIM2->CCER &= ~TIM_CCER_CC1P;
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
}

/* ========== ISRs ========== */

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);
		uint16_t ccr = TIM_GetCapture1(TIM2);

		if (TIM2->CCER & TIM_CCER_CC1P) {
			if (gate == S_FORWARD && pulse_cnt > 0) {
				GPIO_ResetBits(GPIOA, GPIO_Pin_1);
				if (pulse_cnt >= burst_count) {
					gate = S_SILENCE;
					pulse_cnt = 0;
					tim3_ms = burst_delay;
					TIM3->CNT = 0;
					TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
					TIM_Cmd(TIM3, ENABLE);
				}
			}
		} else {
			/* measure frequency on rising edges */
			if (prev_rise_valid) {
				uint32_t period = (ccr >= prev_rise_ccr)
					? (ccr - prev_rise_ccr)
					: (ccr + 65536UL - prev_rise_ccr);
				if (period > 0) {
					input_freq_hz = 72000000UL / period;
				}
			}
			prev_rise_ccr = ccr;
			prev_rise_valid = 1;

			if (gate == S_FORWARD) {
				pulse_cnt++;
				GPIO_SetBits(GPIOA, GPIO_Pin_1);
			}
		}
		TIM2->CCER ^= TIM_CCER_CC1P;
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		if (tim3_ms) {
			tim3_ms--;
			if (tim3_ms == 0) {
				TIM_Cmd(TIM3, DISABLE);
				gate = S_FORWARD;
				/* resync edge polarity with current input level */
				if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0))
					TIM2->CCER |= TIM_CCER_CC1P;
				else
					TIM2->CCER &= ~TIM_CCER_CC1P;
			}
		}
	}
}
