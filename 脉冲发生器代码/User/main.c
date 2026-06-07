#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* ========== UART ========== */
uint8_t  uart_rx_data;
uint8_t  uart_rx_flag;
char     cmd_buf[32];
uint8_t  cmd_idx;

void UART_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin   = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Pin   = GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpio);

	USART_InitTypeDef usart = {0};
	usart.USART_BaudRate            = 115200;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
	usart.USART_Parity              = USART_Parity_No;
	usart.USART_StopBits            = USART_StopBits_1;
	usart.USART_WordLength          = USART_WordLength_8b;
	USART_Init(USART1, &usart);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef nvic = {0};
	nvic.NVIC_IRQChannel    = USART1_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	USART_Cmd(USART1, ENABLE);
}

void UART_SendByte(uint8_t byte)
{
	USART_SendData(USART1, byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendStr(const char *s) { while (*s) UART_SendByte(*s++); }

int fputc(int ch, FILE *f) { UART_SendByte((uint8_t)ch); return ch; }

void UART_Printf(const char *fmt, ...)
{
	char buf[100];
	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	UART_SendStr(buf);
}

/* ========== Generator Params ========== */
uint32_t gen_freq      = 10000;
uint32_t gen_duty      = 50;
uint32_t gen_count     = 5;
uint32_t gen_interval  = 1;       /* 间隔秒数, 默认1秒 */
uint8_t  gen_running   = 0;
uint32_t gen_cur_pulse = 0;
uint32_t gen_rounds    = 0;

uint32_t pulse_cnt      = 0;
uint32_t interval_cnt   = 0;   /* TIM2 秒计数器 */
uint8_t  timer_inited   = 0;

/* ========== Timer Hardware Init (delayed until START) ========== */
void Timer_Hardware_Init(void)
{
	/* PA6 -> TIM3_CH1 PWM */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin   = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	/* TIM3: PWM */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef tim = {0};
	tim.TIM_Period        = 7200 - 1;
	tim.TIM_Prescaler     = 0;
	tim.TIM_CounterMode   = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &tim);

	TIM_OCInitTypeDef oc = {0};
	oc.TIM_OCMode       = TIM_OCMode_PWM1;
	oc.TIM_OutputState  = TIM_OutputState_Enable;
	oc.TIM_Pulse        = 3600;
	oc.TIM_OCPolarity   = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &oc);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ITConfig(TIM3, TIM_IT_Update | TIM_IT_CC1, ENABLE);

	NVIC_InitTypeDef nvic = {0};
	nvic.NVIC_IRQChannel    = TIM3_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	/* TIM2: 1s base interval (software counter extends) */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	tim.TIM_Period    = 10000 - 1;   /* 1s per overflow */
	tim.TIM_Prescaler = 7200 - 1;    /* 10KHz tick */
	TIM_TimeBaseInit(TIM2, &tim);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	nvic.NVIC_IRQChannel    = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_Init(&nvic);

	timer_inited = 1;
}

void Update_PWM_Params(void)
{
	uint32_t f = gen_freq, d = gen_duty;
	if (f < 1000) f = 1000; if (f > 100000) f = 100000;
	if (d < 1)    d = 1;    if (d > 99)       d = 99;

	/* target ARR >= 200 for good PWM resolution; fallback to ARR>=100 */
	uint32_t arr = 999;
	uint32_t psc = 72000000UL / (f * (arr + 1));
	if (psc == 0) {
		arr = 199;
		psc = 72000000UL / (f * (arr + 1));
		if (psc == 0) {
			arr = 99;
			psc = 72000000UL / (f * (arr + 1));
			if (psc == 0) {
				psc = 1;
				arr = 72000000UL / f - 1;
			}
		}
	}
	if (arr > 65535) {
		arr = 65535;
		psc = 72000000UL / (f * (arr + 1));
		if (psc == 0) psc = 1;
	}
	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * d / 100);
	if (ccr == 0 || ccr >= arr) ccr = arr / 2; /* safety: 50% */
	if (ccr > arr) ccr = arr;

	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC  = psc - 1;
	TIM3->ARR  = arr;
	TIM3->CCR1 = ccr;
}

void Generator_Start(void)
{
	if (!timer_inited) Timer_Hardware_Init();
	gen_running   = 1;
	pulse_cnt     = 0;
	gen_cur_pulse = 0;
	interval_cnt  = gen_interval - 1; /* first round fires immediately */
	Update_PWM_Params();
	TIM_SetCounter(TIM3, 0);
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	UART_SendStr("STARTED\r\n");
}

void Generator_Stop(void)
{
	gen_running = 0;
	interval_cnt = 0;
	TIM_Cmd(TIM2, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
	/* 强制 PA6 输出低电平 */
	TIM_ForcedOC1Config(TIM3, TIM_ForcedAction_InActive);
	UART_SendStr("STOPPED\r\n");
}

/* ========== Command Parser ========== */
int extract_num(const char *s, const char *key)
{
	char *p = strstr(s, key);
	return p ? atoi(p + strlen(key)) : 0;
}

void ParseCommand(void)
{
	uint32_t fv = 0, dv = 0, nv = 0, tv = 0;
	int has_start = (strstr(cmd_buf, "START") != NULL);
	int has_f = (strstr(cmd_buf, "F=") != NULL);
	int has_d = (strstr(cmd_buf, "D=") != NULL);
	int has_n = (strstr(cmd_buf, "N=") != NULL);
	int has_t = (strstr(cmd_buf, "T=") != NULL);
	int has_params = (has_f && has_d && has_n);

	if (has_params) {
		fv = extract_num(cmd_buf, "F=");
		dv = extract_num(cmd_buf, "D=");
		nv = extract_num(cmd_buf, "N=");
		if (has_t) {
			tv = extract_num(cmd_buf, "T=");
			if (tv >= 1 && tv <= 3600) gen_interval = tv;
		}
	}

	/* ── START F=10000 D=50 N=10 ── */
	if (has_start && has_params) {
		if (fv >= 1000 && fv <= 100000 && dv >= 1 && dv <= 99 && nv >= 1 && nv <= 100000) {
			gen_freq  = fv;
			gen_duty  = dv;
			gen_count = nv;
			if (!gen_running) {
				Generator_Start();
			} else {
				if (timer_inited) Update_PWM_Params();
				UART_SendStr("ALREADY RUNNING\r\n");
			}
			return;
		} else {
			UART_SendStr("ERR RANGE\r\n");
			return;
		}
	}

	/* ── F=10000 D=50 N=10 (set only) ── */
	if (has_params) {
		if (fv >= 1000 && fv <= 100000 && dv >= 1 && dv <= 99 && nv >= 1 && nv <= 100000) {
			gen_freq  = fv;
			gen_duty  = dv;
			gen_count = nv;
			if (timer_inited) Update_PWM_Params();
			UART_Printf("OK F=%luHz D=%lu%% N=%lu T=%lus\r\n", fv, dv, nv, gen_interval);
		} else {
			UART_SendStr("ERR RANGE\r\n");
		}
		return;
	}

	/* ── T= parameter only ── */
	if (has_t) {
		tv = extract_num(cmd_buf, "T=");
		if (tv >= 1 && tv <= 3600) {
			gen_interval = tv;
			UART_Printf("OK T=%lus\r\n", tv);
		} else UART_SendStr("ERR RANGE (1~3600)\r\n");
		return;
	}

	/* ── legacy single-param commands ── */
	if (strncmp(cmd_buf, "FREQ=", 5) == 0) {
		int v = atoi(cmd_buf + 5);
		if (v >= 1000 && v <= 100000) {
			gen_freq = v;
			if (timer_inited) Update_PWM_Params();
			UART_Printf("OK FREQ=%dHz\r\n", v);
		} else UART_SendStr("ERR RANGE\r\n");
	}
	else if (strncmp(cmd_buf, "DUTY=", 5) == 0) {
		int v = atoi(cmd_buf + 5);
		if (v >= 1 && v <= 99) {
			gen_duty = v;
			if (timer_inited) Update_PWM_Params();
			UART_Printf("OK DUTY=%d%%\r\n", v);
		} else UART_SendStr("ERR RANGE\r\n");
	}
	else if (strncmp(cmd_buf, "PULSES=", 7) == 0) {
		int v = atoi(cmd_buf + 7);
		if (v >= 1 && v <= 100000) {
			gen_count = v;
			UART_Printf("OK PULSES=%d\r\n", v);
		} else UART_SendStr("ERR RANGE\r\n");
	}
	else if (strcmp(cmd_buf, "START") == 0) {
		if (!gen_running) Generator_Start();
		else UART_SendStr("ALREADY RUNNING\r\n");
	}
	else if (strcmp(cmd_buf, "STOP") == 0) {
		Generator_Stop();
	}
	else if (strcmp(cmd_buf, "STATUS?") == 0) {
		UART_Printf(
			"S=%s F=%luHz D=%lu%% N=%lu C=%lu R=%lu T=%lus\r\n",
			gen_running ? "RUN" : "STOP",
			gen_freq, gen_duty, gen_count, gen_cur_pulse, gen_rounds, gen_interval);
	}
	else if (strcmp(cmd_buf, "HELP") == 0) {
		UART_SendStr(
			"F=10000 D=50 N=10 T=1   Set all (Hz / % / count / sec)\r\n"
			"START F=10000 D=50 N=10 T=1  Set + start\r\n"
			"STOP / STATUS? / HELP\r\n");
	}
	else UART_SendStr("ERR FORMAT\r\n");
}

/* ========== ISRs ========== */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		uart_rx_data = USART_ReceiveData(USART1);
		uart_rx_flag = 1;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		if (gen_running) {
			interval_cnt++;
			if (interval_cnt >= gen_interval) {
				interval_cnt  = 0;
				pulse_cnt     = 0;
				gen_cur_pulse = 0;
				Update_PWM_Params();
				TIM_Cmd(TIM3, ENABLE);
			}
		}
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);
		/* CC1 match = output just went LOW, one full pulse completed */
		pulse_cnt++;
		gen_cur_pulse = pulse_cnt;
		if (pulse_cnt >= gen_count) {
			/* Nth pulse just finished gracefully, output already LOW */
			TIM_Cmd(TIM3, DISABLE);
			TIM_ClearITPendingBit(TIM3, TIM_IT_Update); /* suppress stale update */
			gen_rounds++;
		}
	}

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		/* Update = next pulse starting. Nothing to do here. */
	}
}

/* ========== Main ========== */
int main(void)
{
	UART_Init();

	/* PA6 初始化为推挽输出低电平, START 后由 TIM3 AF 接管 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin   = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);
	GPIO_ResetBits(GPIOA, GPIO_Pin_6);

	UART_SendStr("Pulse Gen v1 (F=Hz D=% N=count T=sec)\r\n");

	while (1) {
		while (uart_rx_flag) {
			uart_rx_flag = 0;
			char ch = uart_rx_data;
			if (ch == '\r' || ch == '\n') {
				if (cmd_idx > 0) {
					cmd_buf[cmd_idx] = '\0';
					cmd_idx = 0;
					ParseCommand();
				}
			} else {
				if (cmd_idx < 31) cmd_buf[cmd_idx++] = ch;
			}
		}
	}
}
