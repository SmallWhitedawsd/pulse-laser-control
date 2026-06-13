/* main.c — real-time pulse gate */

#include "stm32f10x.h"
#include "serial.h"
#include "output.h"
#include "cmd.h"
#include "config.h"

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	UART_Init();
	Output_Init();

	UART_SendStr("LASER-GATE\r\n");

	Config_Load();

	while (1) {
		Output_Poll();
		Cmd_Poll();
	}
}
