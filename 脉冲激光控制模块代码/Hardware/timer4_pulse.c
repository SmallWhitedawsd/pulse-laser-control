#include "timer4_pulse.h"

volatile uint8_t timer4_expired = 0;

void Timer4_Pulse_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/* TIM4 时基: 1MHz (72MHz/72=1MHz), 分辨率 1μs */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);
}

void Timer4_Start_us(uint16_t width_us)
{
	TIM4->CNT = 0;
	TIM4->ARR = width_us;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	TIM_Cmd(TIM4, ENABLE);
}

void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		TIM_Cmd(TIM4, DISABLE);
		timer4_expired = 1;
	}
}
