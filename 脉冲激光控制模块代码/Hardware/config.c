/* config.c — Flash-backed persistent configuration */

#include "config.h"
#include "output.h"
#include "serial.h"

void Config_Load(void)
{
	uint32_t v0 = *(__IO uint32_t *)(CONFIG_PAGE_ADDR);
	uint32_t v1 = *(__IO uint32_t *)(CONFIG_PAGE_ADDR + 4);

	if (v0 != 0xFFFFFFFFU && v0 >= 1 && v0 <= 65535) {
		burst_count = v0;
	}
	if (v1 != 0xFFFFFFFFU && v1 >= 1 && v1 <= 50000) {
		burst_delay = v1;
	}
	UART_Printf("LOAD N=%lu T=%lums\r\n", burst_count, burst_delay);
}

void Config_Save(void)
{
	FLASH_Unlock();

	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	while (FLASH_ErasePage(CONFIG_PAGE_ADDR) != FLASH_COMPLETE);

	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
	FLASH_ProgramWord(CONFIG_PAGE_ADDR,     burst_count);
	FLASH_ProgramWord(CONFIG_PAGE_ADDR + 4, burst_delay);

	FLASH_Lock();

	/* reset gate state after CPU stall */
	Output_Resync();
}
