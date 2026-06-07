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
uint8_t  gen_running   = 0;
uint32_t gen_cur_pulse = 0;
uint32_t gen_rounds    = 0;

uint32_t pulse_cnt      = 0;
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
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef nvic = {0};
	nvic.NVIC_IRQChannel    = TIM3_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	/* TIM2: 5s interval */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	tim.TIM_Period    = 50000 - 1;
	tim.TIM_Prescaler = 7200 - 1;
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

	uint32_t arr = 999;
	uint32_t psc = 72000000UL / (f * (arr + 1));
	if (psc == 0) { psc = 1; arr = 72000000UL / f - 1; }
	if (arr > 65535) {
		arr = 65535;
		psc = 72000000UL / (f * (arr + 1));
		if (psc == 0) psc = 1;
	}
	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * d / 100);
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
	Update_PWM_Params();
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	UART_SendStr("STARTED\r\n");
}

void Generator_Stop(void)
{
	gen_running = 0;
	TIM_Cmd(TIM2, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
	UART_SendStr("STOPPED\r\n");
}

/* ========== Command Parser ========== */
void ParseCommand(void)
{
	if      (strncmp(cmd_buf, "FREQ=", 5) == 0) {
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
		if (v >= 1 && v <= 500) {
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
			"S=%s F=%luHz D=%lu%% N=%lu C=%lu R=%lu\r\n",
			gen_running ? "RUN" : "STOP",
			gen_freq, gen_duty, gen_count, gen_cur_pulse, gen_rounds);
	}
	else if (strcmp(cmd_buf, "HELP") == 0) {
		UART_SendStr(
			"FREQ=xxx   Hz (1000~100000)\r\n"
			"DUTY=xx    %% (1~99)\r\n"
			"PULSES=N   Per round (1~500)\r\n"
			"START/STOP/STATUS?/HELP\r\n");
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
			pulse_cnt     = 0;
			gen_cur_pulse = 0;
			Update_PWM_Params();
			TIM_Cmd(TIM3, ENABLE);
		}
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		pulse_cnt++;
		gen_cur_pulse = pulse_cnt;
		if (pulse_cnt >= gen_count) {
			TIM_Cmd(TIM3, DISABLE);
			gen_rounds++;
		}
	}
}

/* ========== Main ========== */
int main(void)
{
	UART_Init();

	UART_SendStr("Pulse Gen v1 (START to begin)\r\n");

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
