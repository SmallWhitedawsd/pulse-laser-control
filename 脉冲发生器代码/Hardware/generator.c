#include "generator.h"
#include "Serial.h"

/* ── 参数默认值 ── */
volatile uint32_t gen_freq_hz      = 10000;   /* 10KHz */
volatile uint32_t gen_duty_pct     = 50;      /* 50% */
volatile uint32_t gen_pulse_count  = 5;       /* 每轮5个脉冲 */
volatile uint8_t  gen_running      = 0;

volatile uint32_t gen_current_pulse = 0;
volatile uint32_t gen_total_rounds  = 0;

/* ── 内部变量 ── */
static volatile uint32_t pulse_cnt = 0;       /* 当前轮已发脉冲计数 */

/**
  * 初始化 TIM2: 5 秒间隔定时器
  *   72MHz / 7200 = 10KHz → 周期 0.1ms
  *   ARR = 50000 → 50000×0.1ms = 5000ms = 5s
  */
static void Generator_Timer2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 50000 - 1;          /* 5s */
	TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;        /* 10KHz */
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);   /* 自由运行 */
}

/**
  * 初始化 TIM3: PWM 输出, 通道1 → PA6
  *   频率和占空比在 Generator_UpdateParams() 中动态计算
  */
static void Generator_Timer3_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* PA6: TIM3_CH1 复用推挽输出 */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* TIM3 时基: 先设一个默认值, 后面更新 */
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 7200 - 1;        /* 先默认 10KHz */
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1;         /* 不分频 */
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* CH1 PWM 模式 1 */
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 3600;                /* 默认 50% */
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

	/* 使能 TIM3 溢出中断（用于计数脉冲） */
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&NVIC_InitStructure);

	/* 注意: 初始不启动 TIM3, 等 TIM2 触发 */
}

/**
  * 根据 freq 和 duty 更新 TIM3 的 PSC/ARR/CCR1
  */
static void Generator_UpdateParams(void)
{
	uint32_t freq = gen_freq_hz;
	uint32_t duty = gen_duty_pct;

	if (freq < 1000)  freq = 1000;
	if (freq > 100000) freq = 100000;
	if (duty < 1)    duty = 1;
	if (duty > 99)   duty = 99;

	/* 找合适的 PSC, 使 ARR 尽量大以提高占空比精度 */
	/* 72MHz / (PSC+1) = TIM3_CLK */
	/* 目标: ARR ≈ 1000（1/1000 分辨率好） */
	/* freq = 72000000 / ((PSC+1) * (ARR+1)) */
	/* 取 ARR = 999 (1000 等分), 则 PSC+1 = 72000000 / (freq * 1000) */

	uint32_t arr = 999;  /* 0.1% 占空比精度 */
	uint32_t psc = 72000000 / (freq * (arr + 1));

	if (psc == 0) {
		/* 频率太高, 减小 ARR */
		psc = 1;
		arr = 72000000 / (freq * psc) - 1;
	}

	if (arr > 65535) {
		arr = 65535;
		psc = 72000000 / (freq * (arr + 1));
		if (psc == 0) psc = 1;
	}

	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * duty / 100);
	if (ccr > arr) ccr = arr;

	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC = psc - 1;
	TIM3->ARR = arr;
	TIM3->CCR1 = ccr;
}

/**
  * 总初始化
  */
void Generator_Init(void)
{
	/* PA6 已由 TIM3 初始化 */
	/* PC13 LED 用于指示 */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);   /* LED 灭 */

	Generator_Timer3_Init();
	Generator_Timer2_Init();

	Generator_UpdateParams();
}

/**
  * 启动发生
  */
void Generator_Start(void)
{
	gen_running = 1;
	pulse_cnt = 0;
	gen_current_pulse = 0;

	Generator_UpdateParams();
	TIM_Cmd(TIM3, ENABLE);               /* 立即开始第一轮 */
}

/**
  * 停止发生
  */
void Generator_Stop(void)
{
	gen_running = 0;
	TIM_Cmd(TIM3, DISABLE);
}

/* ──── ISR ──── */

/**
  * TIM2: 间隔 5 秒 → 启动新一轮脉冲
  */
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		if (gen_running) {
			pulse_cnt = 0;
			gen_current_pulse = 0;

			Generator_UpdateParams();
			TIM_Cmd(TIM3, ENABLE);               /* 启动 PWM */
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);  /* LED 亮 */
		}
	}
}

/**
  * TIM3: PWM 溢出 → 一个脉冲完成
  */
void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

		pulse_cnt++;
		gen_current_pulse = pulse_cnt;

		if (pulse_cnt >= gen_pulse_count) {
			/* 本轮结束, 关闭 PWM 输出, 等待下轮 TIM2 触发 */
			TIM_Cmd(TIM3, DISABLE);
			GPIO_SetBits(GPIOC, GPIO_Pin_13);   /* LED 灭 */
			gen_total_rounds++;
			pulse_cnt = 0;
		}
	}
}
