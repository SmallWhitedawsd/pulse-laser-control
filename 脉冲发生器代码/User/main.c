#include "stm32f10x.h"
#include "Serial.h"
#include "generator.h"
#include "uart_cmd.h"

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	Serial_Init();
	Generator_Init();

	Serial_SendString("Pulse Generator v1\r\n");
	Serial_Printf("FREQ=%luHz DUTY=%lu%% PULSES=%lu\r\n",
		gen_freq_hz, gen_duty_pct, gen_pulse_count);
	Serial_SendString("Type START to begin. HELP for commands.\r\n\r\n");

	while (1) {
		process_uart_command();
	}
}
