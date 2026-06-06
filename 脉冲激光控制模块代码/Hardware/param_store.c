#include "param_store.h"

#define PARAM_FLASH_ADDR  0x0800FC00
#define PARAM_MAGIC       0xAA55BEEF

typedef struct {
	uint32_t magic;
	uint32_t gap_ms;
	uint32_t crc;
} __attribute__((packed)) param_t;

static uint32_t calc_crc(const param_t *p)
{
	uint32_t sum = p->magic + p->gap_ms;
	return ~sum;
}

uint32_t param_load(void)
{
	const param_t *p = (const param_t *)PARAM_FLASH_ADDR;

	if (p->magic == PARAM_MAGIC) {
		uint32_t crc = calc_crc(p);
		if (p->crc == crc) {
			return p->gap_ms;
		}
	}
	return 5000;	/* 默认值 5s */
}

void param_save(uint32_t gap_ms)
{
	param_t p;
	p.magic = PARAM_MAGIC;
	p.gap_ms = gap_ms;
	p.crc = calc_crc(&p);

	FLASH_Unlock();
	FLASH_ErasePage(PARAM_FLASH_ADDR);

	uint32_t *src = (uint32_t *)&p;
	uint32_t addr = PARAM_FLASH_ADDR;
	for (int i = 0; i < sizeof(param_t) / 4; i++) {
		FLASH_ProgramWord(addr, *src++);
		addr += 4;
	}

	FLASH_Lock();
}
