/* serial.h - USART1 115200 8N1 */

#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdio.h>

extern uint8_t uart_rx_data;
extern uint8_t uart_rx_flag;

void UART_Init(void);
void UART_SendByte(uint8_t b);
void UART_SendStr(const char *s);
void UART_Printf(const char *fmt, ...);

#endif
