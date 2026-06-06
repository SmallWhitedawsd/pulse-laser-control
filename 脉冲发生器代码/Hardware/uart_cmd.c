#include "uart_cmd.h"
#include "Serial.h"
#include "generator.h"
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

			/* ── 解析命令 ── */

			if (strncmp(cmd_buf, "PULSES=", 7) == 0) {
				uint32_t val = atoi(cmd_buf + 7);
				if (val >= 1 && val <= 500) {
					gen_pulse_count = val;
					Serial_Printf("OK PULSES=%lu\r\n", val);
				} else {
					Serial_SendString("ERR RANGE (1~500)\r\n");
				}
			}
			else if (strncmp(cmd_buf, "FREQ=", 5) == 0) {
				uint32_t val = atoi(cmd_buf + 5);
				if (val >= 1000 && val <= 100000) {
					gen_freq_hz = val;
					Serial_Printf("OK FREQ=%luHz\r\n", val);
				} else {
					Serial_SendString("ERR RANGE (1000~100000)\r\n");
				}
			}
			else if (strncmp(cmd_buf, "DUTY=", 5) == 0) {
				uint32_t val = atoi(cmd_buf + 5);
				if (val >= 1 && val <= 99) {
					gen_duty_pct = val;
					Serial_Printf("OK DUTY=%lu%%\r\n", val);
				} else {
					Serial_SendString("ERR RANGE (1~99)\r\n");
				}
			}
			else if (strcmp(cmd_buf, "START") == 0) {
				if (!gen_running) {
					Generator_Start();
					Serial_SendString("OK STARTED\r\n");
				} else {
					Serial_SendString("ERR ALREADY RUNNING\r\n");
				}
			}
			else if (strcmp(cmd_buf, "STOP") == 0) {
				Generator_Stop();
				Serial_SendString("OK STOPPED\r\n");
			}
			else if (strcmp(cmd_buf, "STATUS?") == 0) {
				Serial_Printf(
					"STATE=%s FREQ=%luHz DUTY=%lu%% PULSES=%lu CUR=%lu ROUNDS=%lu\r\n",
					gen_running ? "RUN" : "STOP",
					gen_freq_hz, gen_duty_pct, gen_pulse_count,
					gen_current_pulse, gen_total_rounds);
			}
			else if (strcmp(cmd_buf, "HELP") == 0) {
				Serial_SendString(
					"PULSES=N   Per-round pulse count (1~500)\r\n"
					"FREQ=xxx   Pulse frequency Hz (1000~100000)\r\n"
					"DUTY=xx    Duty cycle % (1~99)\r\n"
					"START      Start generating\r\n"
					"STOP       Stop generating\r\n"
					"STATUS?    Query status\r\n"
					"HELP       This help\r\n"
				);
			}
			else {
				Serial_SendString("ERR FORMAT\r\n");
			}
		} else {
			if (cmd_idx < sizeof(cmd_buf) - 1)
				cmd_buf[cmd_idx++] = ch;
		}
	}
}
