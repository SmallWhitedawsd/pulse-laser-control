#include "generator.h"
#include "Serial.h"

volatile uint32_t gen_freq_hz      = 10000;
volatile uint32_t gen_duty_pct     = 50;
volatile uint32_t gen_pulse_count  = 5;
volatile uint8_t  gen_running      = 0;
volatile uint32_t gen_current_pulse = 0;
volatile uint32_t gen_total_rounds  = 0;

static volatile uint32_t pulse_cnt = 0;
static uint8_t timers_ready = 0;   /* 0=未初始化, 1=已就绪 */

void Generator_Init(void)
{
	/* 先什么都不做，保证串口能跑 */
}

static void Generator_Hardware_Init(void)
{
	/* ── PA6: TIM3_CH1 PWM 输出 ── */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 7200 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 3600;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	/* ── TIM2: 5 秒间隔 ── */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseStructure.TIM_Period = 50000 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	timers_ready = 1;
}

static void Generator_UpdateParams(void)
{
	uint32_t freq = gen_freq_hz;
	uint32_t duty = gen_duty_pct;

	if (freq < 1000)  freq = 1000;
	if (freq > 100000) freq = 100000;
	if (duty < 1)    duty = 1;
	if (duty > 99)   duty = 99;

	uint32_t arr = 999;
	uint32_t psc = 72000000UL / (freq * (arr + 1));

	if (psc == 0) {
		psc = 1;
		arr = 72000000UL / freq - 1;
	}
	if (arr > 65535) {
		arr = 65535;
		psc = 72000000UL / (freq * (arr + 1));
		if (psc == 0) psc = 1;
	}

	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * duty / 100);
	if (ccr > arr) ccr = arr;

	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC = psc - 1;
	TIM3->ARR = arr;
	TIM3->CCR1 = ccr;
}

void Generator_Start(void)
{
	if (!timers_ready) {
		Generator_Hardware_Init();
	}
	gen_running = 1;
	pulse_cnt = 0;
	gen_current_pulse = 0;

	Generator_UpdateParams();
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);       /* TIM2 开始周期性 5s 计时 */
	TIM_Cmd(TIM3, ENABLE);       /* 立即输出第一轮 */
}

void Generator_Stop(void)
{
	gen_running = 0;
	TIM_Cmd(TIM2, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
}

/* ──── ISR ──── */

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		if (gen_running) {
			pulse_cnt = 0;
			gen_current_pulse = 0;
			Generator_UpdateParams();
			TIM_Cmd(TIM3, ENABLE);
		}
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		pulse_cnt++;
		gen_current_pulse = pulse_cnt;
		if (pulse_cnt >= gen_pulse_count) {
			TIM_Cmd(TIM3, DISABLE);
			gen_total_rounds++;
		}
	}
}
