#ifndef __GENERATOR_H
#define __GENERATOR_H

#include "stm32f10x.h"

/* 全局参数 */
extern volatile uint32_t gen_freq_hz;     /* 脉冲频率 1000~100000 Hz */
extern volatile uint32_t gen_duty_pct;    /* 占空比 1~99 % */
extern volatile uint32_t gen_pulse_count; /* 每轮脉冲个数 1~500 */
extern volatile uint8_t  gen_running;     /* 运行标志: 0=停止, 1=运行中 */

/* 状态查询 */
extern volatile uint32_t gen_current_pulse;  /* 当前已发出脉冲数 */
extern volatile uint32_t gen_total_rounds;   /* 总轮数 */

void Generator_Init(void);
void Generator_Start(void);
void Generator_Stop(void);

#endif
