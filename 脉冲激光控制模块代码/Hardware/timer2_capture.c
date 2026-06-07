#include "timer2_capture.h"
#include "fifo.h"
#include "state_machine.h"

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

	/* TIM2 时基: 72MHz, 不分频 */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* TIM2 CH1 输入捕获: 双沿 */
	TIM_ICInitTypeDef TIM_ICInitStructure;
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInitStructure.TIM_ICFilter = 0x0;
	TIM_ICInit(TIM2, &TIM_ICInitStructure);

	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	/* TIM2 enable delayed to Timer2_Capture_Enable() */
}

void Timer2_Capture_Enable(void)
{
	TIM_Cmd(TIM2, ENABLE);
}

/* ISR: record width on falling edge → push to FIFO */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_CC1) == SET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

		uint16_t capture = TIM_GetCapture1(TIM2);

		if (TIM2->CCER & TIM_CCER_CC1P) {
			/* 下降沿 → 算脉宽(μs) → 入队 */
			uint16_t width = (capture - capture_start) / 72;  /* 72MHz→μs */
			if (!FIFO_IS_FULL()) {
				fifo_push(width);
			}
		} else {
			/* 上升沿 → 记录起点 */
			capture_start = capture;
		}

		TIM2->CCER ^= TIM_CCER_CC1P;  /* toggle edge */
	}
}
