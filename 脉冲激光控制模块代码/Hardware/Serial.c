/* serial.c */

#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include "serial.h"

uint8_t uart_rx_data;
uint8_t uart_rx_flag;

void UART_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef g = {0};
	g.GPIO_Mode  = GPIO_Mode_AF_PP;
	g.GPIO_Pin   = GPIO_Pin_9;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &g);
	g.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	g.GPIO_Pin   = GPIO_Pin_10;
	GPIO_Init(GPIOA, &g);

	USART_InitTypeDef u = {0};
	u.USART_BaudRate            = 115200;
	u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	u.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
	u.USART_Parity              = USART_Parity_No;
	u.USART_StopBits            = USART_StopBits_1;
	u.USART_WordLength          = USART_WordLength_8b;
	USART_Init(USART1, &u);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	/* USART1 IRQ priority = 2,0 (lowest) */
	NVIC_InitTypeDef n = {0};
	n.NVIC_IRQChannel    = USART1_IRQn;
	n.NVIC_IRQChannelPreemptionPriority = 2;
	n.NVIC_IRQChannelSubPriority = 0;
	n.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&n);

	USART_Cmd(USART1, ENABLE);
}

void UART_SendByte(uint8_t b)
{
	USART_SendData(USART1, b);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendStr(const char *s)
{
	while (*s) UART_SendByte(*s++);
}

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

void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		uart_rx_data = USART_ReceiveData(USART1);
		uart_rx_flag = 1;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
