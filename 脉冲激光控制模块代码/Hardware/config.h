/* config.h — Flash-backed persistent configuration
 *
 * Stores gap_ms in the last 1-KB page of 64-KB Flash (page 63, 0x0800FC00).
 * A stored value of 0xFFFFFFFF (erased) means "never set → passthrough".
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f10x.h"

#define CONFIG_PAGE_ADDR  0x0800FC00U      /* page 63, last page of 64-KB Flash */
#define CONFIG_PAGE_SIZE  1024U

void Config_Load(void);   /* read gap_ms from Flash, 0 if never saved */
void Config_Save(void);   /* write current gap_ms to Flash */

#endif
