#include "uart_cmd.h"
#include "Serial.h"
#include "generator.h"
#include <string.h>
#include <stdlib.h>

static char cmd_buf[32];
static uint8_t cmd_idx = 0;

static void parse_command(void);

void process_uart_command(void)
{
	while (Serial_RxFlag) {
		Serial_RxFlag = 0;
		char ch = Serial_RxData;

		if (ch == '\r' || ch == '\n') {
			if (cmd_idx == 0) continue;
			cmd_buf[cmd_idx] = '\0';
			cmd_idx = 0;
			parse_command();
		} else {
			if (cmd_idx < sizeof(cmd_buf) - 1)
				cmd_buf[cmd_idx++] = ch;
		}
	}
}

static void parse_command(void)
{
	if (strncmp(cmd_buf, "PULSES=", 7) == 0) {
		int val = atoi(cmd_buf + 7);
		if (val >= 1 && val <= 500) {
			gen_pulse_count = val;
			Serial_Printf("OK PULSES=%d\r\n", val);
		} else {
			Serial_SendString("ERR RANGE (1~500)\r\n");
		}
	}
	else if (strncmp(cmd_buf, "FREQ=", 5) == 0) {
		int val = atoi(cmd_buf + 5);
		if (val >= 1000 && val <= 100000) {
			gen_freq_hz = val;
			Serial_Printf("OK FREQ=%dHz\r\n", val);
		} else {
			Serial_SendString("ERR RANGE (1000~100000)\r\n");
		}
	}
	else if (strncmp(cmd_buf, "DUTY=", 5) == 0) {
		int val = atoi(cmd_buf + 5);
		if (val >= 1 && val <= 99) {
			gen_duty_pct = val;
			Serial_Printf("OK DUTY=%d%%\r\n", val);
		} else {
			Serial_SendString("ERR RANGE (1~99)\r\n");
		}
	}
	else if (strcmp(cmd_buf, "START") == 0) {
		if (!gen_running) {
			gen_running = 1;
			TIM_SetCounter(TIM2, 0);
			TIM_Cmd(TIM2, ENABLE);
			Serial_SendString("OK STARTED\r\n");
		} else {
			Serial_SendString("ERR ALREADY RUNNING\r\n");
		}
	}
	else if (strcmp(cmd_buf, "STOP") == 0) {
		gen_running = 0;
		TIM_Cmd(TIM2, DISABLE);
		TIM_Cmd(TIM3, DISABLE);
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
			"PULSES=N   Count per round (1~500)\r\n"
			"FREQ=xxx   Frequency Hz (1000~100000)\r\n"
			"DUTY=xx    Duty cycle % (1~99)\r\n"
			"START      Start (5s interval)\r\n"
			"STOP       Stop\r\n"
			"STATUS?    Query state\r\n"
			"HELP       This help\r\n"
		);
	}
	else {
		Serial_SendString("ERR FORMAT\r\n");
	}
}
