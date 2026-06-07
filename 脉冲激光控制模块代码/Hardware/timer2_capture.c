#include "timer2_capture.h"
#include "fifo.h"
#include "state_machine.h"

volatile uint8_t timer2_first_pulse_bypassed = 0;

static volatile uint16_t capture_start = 0;

void Timer2_Capture_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA0: TIM2_CH1, 浮空输入 */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* TIM2 时基: 72MHz, 不分频, 向上计数 */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* TIM2 CH1 输入捕获: 双沿, 无滤波 */
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	/* 使能 CC1 中断 */
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

	/* NVIC */
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	/* TIM2 counter enabled later (after welcome message) to avoid floating PA0 triggering ISRs */
}

void Timer2_Capture_Enable(void)
{
	TIM_Cmd(TIM2, ENABLE);
}

/* ISR 放在 stm32f10x_it.c */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) == SET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

		if (test_mode) return;  /* ignore all input during TEST */

		uint16_t capture = TIM_GetCapture1(TIM2);

		if (TIM2->CCER & TIM_CCER_CC1P) {
			/* 下降沿 */
			uint16_t width = capture - capture_start;

			if (timer2_first_pulse_bypassed) {
				/* 首脉冲已直通输出; 拉低PA1, 进入间隙 */
				GPIO_ResetBits(GPIOA, GPIO_Pin_1);
				deadtime_start(g_gap_ms);
				g_out_state = GAP_WAIT;
				timer2_first_pulse_bypassed = 0;
			} else {
				if (!FIFO_IS_FULL()) {
					fifo_push(width);
				}
			}
		} else {
			/* 上升沿 */
			capture_start = capture;

			if (g_out_state == IDLE && FIFO_IS_EMPTY()) {
				GPIO_SetBits(GPIOA, GPIO_Pin_1);
				g_out_state = PULSE_OUT;
				timer2_first_pulse_bypassed = 1;
			}
		}

		TIM2->CCER ^= TIM_CCER_CC1P;
	}
}
