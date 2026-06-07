/* cmd.c — non-blocking UART command parser */

#include "cmd.h"
#include "serial.h"
#include "output.h"
#include "fifo.h"
#include <string.h>
#include <stdlib.h>

static char buf[32];
static uint8_t idx;

static void dispatch(void)
{
	if      (strncmp(buf, "GAP=", 4) == 0) {
		int v = atoi(buf + 4);
		if (v >= 1 && v <= 50000) {
			gap_ms = v;
			UART_Printf("OK GAP=%lums\r\n", gap_ms);
		} else UART_SendStr("ERR RANGE\r\n");
	}
	else if (strcmp(buf, "GAP?") == 0)  UART_Printf("GAP=%lums\r\n", gap_ms);
	else if (strcmp(buf, "SAVE") == 0)  UART_SendStr("SAVED\r\n");
	else if (strcmp(buf, "STATUS?") == 0)
		UART_Printf("FIFO=%u GAP=%lums\r\n", fifo_count(), gap_ms);
	else if (strcmp(buf, "HELP") == 0)
		UART_SendStr("GAP=<ms>(1~50000) GAP? STATUS? SAVE HELP\r\n");
	else UART_SendStr("?\r\n");
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
