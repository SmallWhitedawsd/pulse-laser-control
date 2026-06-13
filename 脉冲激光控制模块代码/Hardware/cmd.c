/* cmd.c — UART command parser
 *
 *   N=<count> T=<ms>   set both, save to Flash
 *   STATUS?            show current N, T
 */

#include "cmd.h"
#include "serial.h"
#include "output.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>

static char buf[32];
static uint8_t idx;

static void dispatch(void)
{
	char *pN = strstr(buf, "N=");
	char *pT = strstr(buf, "T=");
	uint32_t nv = burst_count, tv = burst_delay;
	int n_ok = 0, t_ok = 0;

	if (pN) {
		int v = atoi(pN + 2);
		if (v >= 1 && v <= 65535) { nv = v; n_ok = 1; }
		else { UART_SendStr("ERR N RANGE (1~65535)\r\n"); return; }
	}
	if (pT) {
		int v = atoi(pT + 2);
		if (v >= 1 && v <= 50000) { tv = v; t_ok = 1; }
		else { UART_SendStr("ERR T RANGE (1~50000)\r\n"); return; }
	}

	if (n_ok || t_ok) {
		burst_count = nv;
		burst_delay = tv;
		Config_Save();
		UART_Printf("OK N=%lu T=%lums\r\n", burst_count, burst_delay);
	}
	else if (strcmp(buf, "STATUS?") == 0) {
		UART_Printf("N=%lu T=%lums\r\n", burst_count, burst_delay);
	}
}

void Cmd_Poll(void)
{
	while (uart_rx_flag) {
		uart_rx_flag = 0;
		char ch = uart_rx_data;
		if (ch == '\r' || ch == '\n') {
			if (idx) { buf[idx] = 0; idx = 0; dispatch(); }
		} else if (idx < 31) buf[idx++] = ch;
	}
}
