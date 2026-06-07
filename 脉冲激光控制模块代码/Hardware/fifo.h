/* fifo.h — 1024-entry ring buffer for pulse widths (units: μs) */

#ifndef __FIFO_H
#define __FIFO_H

#include "stm32f10x.h"

#define FIFO_N  1024           /* must be power-of-2 */
#define FIFO_M  (FIFO_N - 1)

extern volatile uint16_t fifo[FIFO_N];
extern volatile uint16_t fifo_wr;   /* write index (ISR) */
extern volatile uint16_t fifo_rd;   /* read  index (main) */

static inline int  fifo_full(void)  { return ((fifo_wr + 1) & FIFO_M) == fifo_rd; }
static inline int  fifo_empty(void) { return fifo_wr == fifo_rd; }

static inline void fifo_push(uint16_t w)
{
	fifo[fifo_wr] = w;
	fifo_wr = (fifo_wr + 1) & FIFO_M;
}

static inline uint16_t fifo_pop(void)
{
	uint16_t w = fifo[fifo_rd];
	fifo_rd = (fifo_rd + 1) & FIFO_M;
	return w;
}

static inline uint16_t fifo_count(void)
{
	return (fifo_wr - fifo_rd) & FIFO_M;
}

static inline void fifo_reset(void)
{
	fifo_wr = 0;
	fifo_rd = 0;
}

#endif
