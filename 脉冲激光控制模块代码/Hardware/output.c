/* output.c — TIM4 pulse-width (1 μs), TIM3 gap (1 ms), state machine
 *
 *    IDLE → (fifo non-empty) → pop width, PA1=1, TIM4 start → PULSE
 *    PULSE → (TIM4 expires) → PA1=0, TIM3 start(gap_ms) → GAP
 *    GAP   → (TIM3 expires) → IDLE
 */

#include "output.h"
#include "fifo.h"

volatile uint32_t gap_ms = 100;          /* default 100 ms */

static volatile uint8_t  tim4_done;
static volatile uint32_t tim3_cnt;       /* remaining ms */

typedef enum { S_IDLE, S_PULSE, S_GAP } state_t;
static volatile state_t state = S_IDLE;

void Output_Init(void)
{
	/* PA1 push-pull output, initial LOW */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef g = {0};
	g.GPIO_Mode  = GPIO_Mode_Out_PP;
	g.GPIO_Pin   = GPIO_Pin_1;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &g);
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);

	/* === TIM4: 1 MHz (1 μs tick), 16-bit auto-reload === */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseInitTypeDef t = {0};
	t.TIM_Period    = 65535;
	t.TIM_Prescaler = 72 - 1;
	t.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &t);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef n = {0};
	n.NVIC_IRQChannel    = TIM4_IRQn;
	n.NVIC_IRQChannelCmd = ENABLE;
	n.NVIC_IRQChannelPreemptionPriority = 1;
	n.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&n);

	/* === TIM3: 1 kHz (1 ms tick) === */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	t.TIM_Period    = 1000 - 1;
	t.TIM_Prescaler = 72000 - 1;
	t.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &t);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	n.NVIC_IRQChannel    = TIM3_IRQn;
	n.NVIC_IRQChannelPreemptionPriority = 1;
	n.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&n);
}

void Output_Poll(void)
{
	switch (state) {

	case S_IDLE:
		if (!fifo_empty()) {
			uint16_t w = fifo_pop();
			GPIO_SetBits(GPIOA, GPIO_Pin_1);        /* PA1 high */
			/* fire TIM4 for w μs */
			TIM4->CNT = 0;
			TIM4->ARR = w;
			tim4_done = 0;
			TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
			TIM_Cmd(TIM4, ENABLE);
			state = S_PULSE;
		}
		break;

	case S_PULSE:
		if (tim4_done) {
			tim4_done = 0;
			GPIO_ResetBits(GPIOA, GPIO_Pin_1);      /* PA1 low */
			/* fire TIM3 for gap_ms */
			tim3_cnt = gap_ms;
			TIM3->CNT = 0;
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
			TIM_Cmd(TIM3, ENABLE);
			state = S_GAP;
		}
		break;

	case S_GAP:
		if (tim3_cnt == 0) {
			state = S_IDLE;
		}
		break;
	}
}

/* === ISRs ================================================================ */

void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		TIM_Cmd(TIM4, DISABLE);
		tim4_done = 1;
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		if (tim3_cnt) {
			tim3_cnt--;
			if (tim3_cnt == 0) {
				TIM_Cmd(TIM3, DISABLE);
			}
		}
	}
}
