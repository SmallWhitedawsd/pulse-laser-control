/* config.h — Flash-backed persistent configuration
 *
 * Layout (page 63, 0x0800FC00):
 *   [0] = burst_count  (N)
 *   [1] = burst_delay  (T ms)
 *   0xFFFFFFFF = never written → use defaults
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

#define CONFIG_PAGE_ADDR  0x0800FC00U

void Config_Load(void);
void Config_Save(void);

#endif
