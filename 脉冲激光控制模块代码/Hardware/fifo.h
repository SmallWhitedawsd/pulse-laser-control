#ifndef __FIFO_H
#define __FIFO_H

#include "stm32f10x.h"

#define FIFO_SIZE  1024
#define FIFO_MASK  (FIFO_SIZE - 1)

extern volatile uint16_t pulse_fifo[FIFO_SIZE];
extern volatile uint16_t fifo_head;
extern volatile uint16_t fifo_tail;

#define FIFO_IS_EMPTY()   (fifo_head == fifo_tail)
#define FIFO_IS_FULL()    (((fifo_head + 1) & FIFO_MASK) == fifo_tail)

static inline void fifo_push(uint16_t width)
{
	pulse_fifo[fifo_head] = width;
	fifo_head = (fifo_head + 1) & FIFO_MASK;
}

static inline uint16_t fifo_pop(void)
{
	uint16_t width = pulse_fifo[fifo_tail];
	fifo_tail = (fifo_tail + 1) & FIFO_MASK;
	return width;
}

#endif
