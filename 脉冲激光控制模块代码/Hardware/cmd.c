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

static uint32_t n_max(void)
{
	/* N upper bound = 1 second worth of pulses = input frequency in Hz.
	 * Floor 1000, ceiling 1000000. If no signal yet, default to 65535. */
	if (input_freq_hz == 0) return 65535;
	if (input_freq_hz < 1000) return 1000;
	if (input_freq_hz > 1000000) return 1000000;
	return input_freq_hz;
}

static void dispatch(void)
{
	char *pN = strstr(buf, "N=");
	char *pT = strstr(buf, "T=");
	uint32_t nv = burst_count, tv = burst_delay;
	int n_ok = 0, t_ok = 0;
	uint32_t nmax = n_max();

	if (pN) {
		int v = atoi(pN + 2);
		if (v >= 1 && (uint32_t)v <= nmax) {
			nv = v; n_ok = 1;
		} else {
			UART_Printf("ERR N RANGE (1~%lu)\r\n", nmax);
			return;
		}
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
		UART_Printf("Fin=%luHz N=%lu/%lu T=%lums\r\n",
			input_freq_hz, burst_count, nmax, burst_delay);
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
