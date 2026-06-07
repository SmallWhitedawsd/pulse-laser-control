/* config.c — Flash-backed persistent configuration for gap_ms */

#include "config.h"
#include "output.h"
#include "serial.h"

/* Load gap_ms from the last Flash page.
 * If the stored value is 0xFFFFFFFF (erased) or out of range [0,50000],
 * gap_ms stays at its compile-time default (0 = passthrough). */
void Config_Load(void)
{
	uint32_t raw = *(__IO uint32_t *)CONFIG_PAGE_ADDR;

	if (raw != 0xFFFFFFFFU && raw <= 50000) {
		gap_ms = raw;
		UART_Printf("LOAD GAP=%lums\r\n", gap_ms);
	}
	/* else: first boot — keep gap_ms=0 → passthrough */
}

/* Save current gap_ms into Flash (page 63). */
void Config_Save(void)
{
	FLASH_Unlock();

	/* Erase page 63 */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	while (FLASH_ErasePage(CONFIG_PAGE_ADDR) != FLASH_COMPLETE);

	/* Program the 32-bit value */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ProgramWord(CONFIG_PAGE_ADDR, gap_ms);

	FLASH_Lock();
}
