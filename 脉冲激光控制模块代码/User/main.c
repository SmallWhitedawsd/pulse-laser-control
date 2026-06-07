/* main.c — Pulse Laser Control Module
 *
 * PA0 (TIM2 CH1) ← input pulse train (1kHz–100kHz, 1–N pulses)
 * PA1 (GPIO)     → output: clones HIGH width; LOW depends on mode
 *
 * Mode: gap_ms==0 → passthrough (mirror input gap)
 *       gap_ms>0  → fixed-gap  (replace gap with gap_ms ms)
 * Config saved to Flash page 63, auto-loaded on boot.
 *
 * Dataflow:
 *   input → capture.c (measure HIGH + LOW, push tagged to fifo)
 *         → output.c  (pop width→TIM4, gap handled per mode)
 */

#include "stm32f10x.h"
#include "serial.h"
#include "capture.h"
#include "output.h"
#include "cmd.h"
#include "config.h"
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

	/* start capture AFTER hardware is stable */
	Capture_Start();

	UART_SendStr("\r\nLASER-CTRL\r\n");

	/* load last-saved gap_ms from Flash (0 if never configured → passthrough) */
	Config_Load();

	while (1) {
		Output_Poll();
		Cmd_Poll();
	}
}
