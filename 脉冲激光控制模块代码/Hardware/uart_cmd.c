#include "uart_cmd.h"
#include "Serial.h"
#include "state_machine.h"
#include "param_store.h"
#include "fifo.h"
#include <string.h>
#include <stdlib.h>

static char cmd_buf[32];
static uint8_t cmd_idx = 0;

void process_uart_command(void)
{
	/* 串口每收到一个字节(在USART1_IRQHandler中置 Serial_RxFlag),
	   拼接命令行，以 \r 或 \n 结尾时解析 */
	while (Serial_RxFlag) {
		Serial_RxFlag = 0;
		char ch = Serial_RxData;

		if (ch == '\r' || ch == '\n') {
			if (cmd_idx == 0) continue;	/* 忽略空行 */

			cmd_buf[cmd_idx] = '\0';
			cmd_idx = 0;

			/* ── 解析命令 ── */
			if (strncmp(cmd_buf, "GAP=", 4) == 0) {
				uint32_t val = atoi(cmd_buf + 4);
				if (val >= 1 && val <= 50000) {
					g_gap_ms = val;
					Serial_Printf("OK GAP=%lums\r\n", val);
				} else {
					Serial_SendString("ERR RANGE\r\n");
				}
			}
			else if (strcmp(cmd_buf, "GAP?") == 0) {
				Serial_Printf("GAP=%lums\r\n", g_gap_ms);
			}
			else if (strcmp(cmd_buf, "SAVE") == 0) {
				param_save(g_gap_ms);
				Serial_SendString("OK SAVED\r\n");
			}
			else if (strcmp(cmd_buf, "STATUS?") == 0) {
				uint16_t count = (fifo_head - fifo_tail) & FIFO_MASK;
				Serial_Printf("STATE=%s GAP=%lums FIFO=%d%s\r\n",
					test_mode ? "TEST" :
					g_out_state == IDLE ? "IDLE" :
					g_out_state == PULSE_OUT ? "PULSE_OUT" : "GAP_WAIT",
					g_gap_ms, count,
					test_mode ? " (PA1: 1Hz)" : "");
			}
			else if (strcmp(cmd_buf, "TEST") == 0) {
				start_test_output();
				Serial_SendString("OK TEST: PA1 = 1Hz square wave\r\n");
			}
			else if (strcmp(cmd_buf, "STOPTEST") == 0) {
				stop_test_output();
				Serial_SendString("OK STOPPED TEST\r\n");
			}
			else if (strcmp(cmd_buf, "HELP") == 0) {
				Serial_SendString(
					"GAP=<ms>  Set gap (1~50000)\r\n"
					"GAP?      Query gap\r\n"
					"SAVE      Save to Flash\r\n"
					"STATUS?   Query status\r\n"
					"TEST      PA1 1Hz test (no input)\r\n"
					"STOPTEST  Stop test\r\n"
					"HELP      This help\r\n"
				);
			}
			else {
				Serial_SendString("ERR FORMAT\r\n");
			}
		} else {
			if (cmd_idx < sizeof(cmd_buf) - 1) {
				cmd_buf[cmd_idx++] = ch;
			}
		}
	}
}
