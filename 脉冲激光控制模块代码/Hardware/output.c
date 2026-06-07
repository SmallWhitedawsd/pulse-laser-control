/* output.c — TIM4 pulse-width (1 μs) for HIGH, TIM3 gap (1 ms) for LOW
 *
 * FIFO encoding:  width  → value (< 0x8000)
 *                 gap    → value | FIFO_TAG_GAP  (0x8000)
 *
 * HIGH width copied from input; LOW gap replaced by user-configured gap_ms.
 * Stray gap entries (from input) are discarded in S_IDLE and S_HIGH.
 *
 *    IDLE → (fifo has width) → pop, PA1=1, TIM4(width) → HIGH
 *    HIGH → (TIM4 expires)   → PA1=0, discard fifo gap, TIM3(gap_ms) → GAP
 *    GAP  → (TIM3 expires)   → IDLE
 */

#include "output.h"
#include "fifo.h"

volatile uint32_t gap_ms = 100;          /* user-configurable gap in ms */

static volatile uint8_t  tim4_done;
static volatile uint32_t tim3_cnt;       /* remaining ms for TIM3 */

typedef enum { S_IDLE, S_HIGH, S_GAP } state_t;
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

	/* === TIM3: 1 kHz (1 ms tick) — user-configured gap === */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	t.TIM_Period    = 1000 - 1;     /* 1 MHz / 1000 = 1 kHz = 1 ms tick */
	t.TIM_Prescaler = 72 - 1;       /* 72 MHz / 72 = 1 MHz  (fits 16-bit!) */
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
		/* discard any stray gap entries, wait for a width */
		while (!fifo_empty()) {
			uint16_t v = fifo[fifo_rd];          /* peek */
			if (v & FIFO_TAG_GAP) {
				fifo_pop();                       /* discard stray gap */
			} else {
				(void)fifo_pop();                 /* consume width */
				GPIO_SetBits(GPIOA, GPIO_Pin_1);  /* PA1 high */
				TIM4->CNT = 0;
				TIM4->ARR = v;
				tim4_done = 0;
				TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
				TIM_Cmd(TIM4, ENABLE);
				state = S_HIGH;
				break;
			}
		}
		break;

	case S_HIGH:
		if (tim4_done) {
			tim4_done = 0;
			GPIO_ResetBits(GPIOA, GPIO_Pin_1);    /* PA1 low */
			/* discard the gap entry if already in fifo */
			if (!fifo_empty() && (fifo[fifo_rd] & FIFO_TAG_GAP)) {
				fifo_pop();
			}
			/* fire TIM3 for user-configured gap_ms */
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
