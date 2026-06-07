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
	while (Serial_RxFlag) {
		Serial_RxFlag = 0;
		char ch = Serial_RxData;

		if (ch == '\r' || ch == '\n') {
			if (cmd_idx == 0) continue;

			cmd_buf[cmd_idx] = '\0';
			cmd_idx = 0;

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
				Serial_Printf("S=%s G=%lums IN=%d OUT=%lu\r\n",
					g_out_state == IDLE ? "I" :
					g_out_state == PULSE_OUT ? "P" : "W",
					g_gap_ms, count, g_out_count);
			}
			else if (strcmp(cmd_buf, "HELP") == 0) {
				Serial_SendString(
					"GAP=<ms>  Set gap (1~50000)\r\n"
					"GAP?      Query gap\r\n"
					"SAVE      Save to Flash\r\n"
					"STATUS?   Query status\r\n"
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
