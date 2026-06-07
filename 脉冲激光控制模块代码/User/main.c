/* main.c — Pulse Laser Control Module
 *
 * PA0 (TIM2 CH1) ← input pulse train (1kHz–100kHz, 1–N pulses)
 * PA1 (GPIO)     → output: clone HIGH width, replace LOW gap with gap_ms
 *
 * Dataflow:
 *   input → capture.c (measure HIGH width + LOW gap, push tagged to fifo)
 *         → output.c  (pop width from fifo, drive PA1 via TIM4;
 *                       discard fifo gap, use TIM3+gap_ms for LOW)
 */

#include "stm32f10x.h"
#include "serial.h"
#include "capture.h"
#include "output.h"
#include "cmd.h"
#include "fifo.h"

/* fifo storage (defined once) */
volatile uint16_t fifo[FIFO_N];
volatile uint16_t fifo_wr;
volatile uint16_t fifo_rd;

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	UART_Init();
	Output_Init();
	Capture_Init();

	/* start capture AFTER everything is stable */
	Capture_Start();

	UART_SendStr("\r\nLASER-CTRL\r\n");

	while (1) {
		Output_Poll();
		Cmd_Poll();
	}
}
