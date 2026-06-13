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

/* ========== PWM Generator (always running) ========== */
volatile uint32_t gen_freq = 10000;

void Update_PWM(uint32_t freq)
{
	if (freq < 1000) freq = 1000;
	if (freq > 100000) freq = 100000;

	uint32_t target = 72000000UL / freq;
	uint32_t psc = target / 65536;
	uint32_t arr = target / (psc + 1) - 1;
	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * 50 / 100);

	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC  = psc;
	TIM3->ARR  = arr;
	TIM3->CCR1 = ccr;
	TIM_SetCounter(TIM3, 0);
	TIM_Cmd(TIM3, ENABLE);
}

void PWM_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin   = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

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

	TIM_Cmd(TIM3, ENABLE);
}

/* ========== Command Parser ========== */
void ParseCommand(void)
{
	if (strncmp(cmd_buf, "FREQ=", 5) == 0) {
		int v = atoi(cmd_buf + 5);
		if (v >= 1000 && v <= 100000) {
			gen_freq = v;
			Update_PWM(gen_freq);
			UART_Printf("OK FREQ=%dHz\r\n", v);
		} else {
			UART_SendStr("ERR RANGE (1000~100000)\r\n");
		}
	}
	else if (strncmp(cmd_buf, "F=", 2) == 0) {
		int v = atoi(cmd_buf + 2);
		if (v >= 1000 && v <= 100000) {
			gen_freq = v;
			Update_PWM(gen_freq);
			UART_Printf("OK F=%dHz\r\n", v);
		} else {
			UART_SendStr("ERR RANGE (1000~100000)\r\n");
		}
	}
	else if (strcmp(cmd_buf, "HELP") == 0) {
		UART_SendStr(
			"FREQ=1000~100000  Set output frequency (Hz)\r\n"
			"HELP              Show this help\r\n");
	}
	else {
		UART_SendStr("ERR FORMAT\r\n");
	}
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

/* ========== Main ========== */
int main(void)
{
	UART_Init();
	PWM_Init();

	UART_SendStr("Pulse Gen (FREQ=1K~100KHz)\r\n");

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
