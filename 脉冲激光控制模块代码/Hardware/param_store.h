#ifndef __PARAM_STORE_H
#define __PARAM_STORE_H

#include "stm32f10x.h"

uint32_t param_load(void);
void param_save(uint32_t gap_ms);

#endif
