/* cmd.c — non-blocking UART command parser
 *
 * Commands:
 *   GAP=<ms>     set fixed-gap (1~50000), auto-save to Flash, mode becomes gap
 *   GAP=0        switch to passthrough mode, auto-save
 *   GAP?         query current gap
 *   MODE?        show current mode (PASSTHROUGH or GAP=<ms>)
 *   SAVE         force-save current gap_ms to Flash
 *   STATUS?      FIFO count + mode info
 *   HELP
 */

#include "cmd.h"
#include "serial.h"
#include "output.h"
#include "config.h"
#include "fifo.h"
#include <string.h>
#include <stdlib.h>

static char buf[32];
static uint8_t idx;

static void dispatch(void)
{
	if      (strncmp(buf, "GAP=", 4) == 0) {
		int v = atoi(buf + 4);
		if (v == 0) {
			gap_ms = 0;
			Config_Save();
			UART_SendStr("OK MODE=PASSTHROUGH\r\n");
		} else if (v >= 1 && v <= 50000) {
			gap_ms = v;
			Config_Save();
			UART_Printf("OK GAP=%lums\r\n", gap_ms);
		} else {
			UART_SendStr("ERR RANGE\r\n");
		}
	}
	else if (strcmp(buf, "GAP?") == 0) {
		if (gap_ms == 0) UART_SendStr("MODE=PASSTHROUGH\r\n");
		else             UART_Printf("GAP=%lums\r\n", gap_ms);
	}
	else if (strcmp(buf, "MODE?") == 0) {
		if (gap_ms == 0) UART_SendStr("MODE=PASSTHROUGH\r\n");
		else             UART_Printf("MODE=GAP GAP=%lums\r\n", gap_ms);
	}
	else if (strcmp(buf, "SAVE") == 0) {
		Config_Save();
		if (gap_ms == 0) UART_SendStr("SAVED PASSTHROUGH\r\n");
		else             UART_SendStr("SAVED\r\n");
	}
	else if (strcmp(buf, "STATUS?") == 0) {
		if (gap_ms == 0)
			UART_Printf("FIFO=%u MODE=PASSTHROUGH\r\n", fifo_count());
		else
			UART_Printf("FIFO=%u MODE=GAP GAP=%lums\r\n", fifo_count(), gap_ms);
	}
	else if (strcmp(buf, "HELP") == 0)
		UART_SendStr("GAP=<ms>(0~50000) GAP=0=passthrough GAP? MODE? STATUS? SAVE HELP\r\n");
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
