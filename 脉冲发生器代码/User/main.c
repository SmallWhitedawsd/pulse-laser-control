#include "stm32f10x.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* ========== 串口 ========== */
uint8_t  Serial_RxData;
uint8_t  Serial_RxFlag;
char     cmd_buf[32];
uint8_t  cmd_idx;

void UART_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin   = GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	gpio.GPIO_Pin   = GPIO_Pin_10;
	GPIO_Init(GPIOA, &gpio);

	USART_InitTypeDef usart = {0};
	usart.USART_BaudRate            = 115200;
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
	usart.USART_Parity              = USART_Parity_No;
	usart.USART_StopBits            = USART_StopBits_1;
	usart.USART_WordLength          = USART_WordLength_8b;
	USART_Init(USART1, &usart);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef nvic = {0};
	nvic.NVIC_IRQChannel    = USART1_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	USART_Cmd(USART1, ENABLE);
}

void UART_SendByte(uint8_t byte)
{
	USART_SendData(USART1, byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendStr(const char *s) { while (*s) UART_SendByte(*s++); }

int fputc(int ch, FILE *f) { UART_SendByte((uint8_t)ch); return ch; }

void UART_Printf(const char *fmt, ...)
{
	char buf[100];
	va_list va;
	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);
	UART_SendStr(buf);
}

/* ========== 脉冲发生器参数 ========== */
uint32_t gen_freq      = 10000;    /* 1KHz~100KHz */
uint32_t gen_duty      = 50;       /* 1~99% */
uint32_t gen_count     = 5;        /* 每轮脉冲数 */
uint8_t  gen_running   = 0;
uint32_t gen_cur_pulse = 0;
uint32_t gen_rounds    = 0;

uint32_t pulse_cnt      = 0;
uint8_t  timer_inited   = 0;

/* ========== 定时器硬件初始化（START 时才调用） ========== */
void Timer_Hardware_Init(void)
{
	/* PA6 → TIM3_CH1 PWM 输出 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef gpio = {0};
	gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin   = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	/* TIM3: PWM */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef tim = {0};
	tim.TIM_Period        = 7200 - 1;
	tim.TIM_Prescaler     = 0;
	tim.TIM_CounterMode   = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &tim);

	TIM_OCInitTypeDef oc = {0};
	oc.TIM_OCMode       = TIM_OCMode_PWM1;
	oc.TIM_OutputState  = TIM_OutputState_Enable;
	oc.TIM_Pulse        = 3600;
	oc.TIM_OCPolarity   = TIM_OCPolarity_High;
	TIM_OC1Init(TIM3, &oc);
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	NVIC_InitTypeDef nvic = {0};
	nvic.NVIC_IRQChannel    = TIM3_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	/* TIM2: 5s 间隔 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	tim.TIM_Period    = 50000 - 1;
	tim.TIM_Prescaler = 7200 - 1;
	TIM_TimeBaseInit(TIM2, &tim);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	nvic.NVIC_IRQChannel    = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_Init(&nvic);

	timer_inited = 1;
}

void Update_PWM_Params(void)
{
	uint32_t f = gen_freq, d = gen_duty;
	if (f < 1000) f = 1000; if (f > 100000) f = 100000;
	if (d < 1)    d = 1;    if (d > 99)       d = 99;

	uint32_t arr = 999;
	uint32_t psc = 72000000UL / (f * (arr + 1));
	if (psc == 0) { psc = 1; arr = 72000000UL / f - 1; }
	if (arr > 65535) {
		arr = 65535;
		psc = 72000000UL / (f * (arr + 1));
		if (psc == 0) psc = 1;
	}
	uint32_t ccr = (uint32_t)((uint64_t)(arr + 1) * d / 100);
	if (ccr > arr) ccr = arr;

	TIM_Cmd(TIM3, DISABLE);
	TIM3->PSC  = psc - 1;
	TIM3->ARR  = arr;
	TIM3->CCR1 = ccr;
}

void Generator_Start(void)
{
	if (!timer_inited) Timer_Hardware_Init();
	gen_running   = 1;
	pulse_cnt     = 0;
	gen_cur_pulse = 0;
	Update_PWM_Params();
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	UART_SendStr("已启动, 每5秒发出一轮脉冲\r\n");
}

void Generator_Stop(void)
{
	gen_running = 0;
	TIM_Cmd(TIM2, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
	UART_SendStr("已停止\r\n");
}

/* ========== 命令解析 ========== */
void ParseCommand(void)
{
	if      (strncmp(cmd_buf, "FREQ=", 5) == 0) {
		int v = atoi(cmd_buf + 5);
		if (v >= 1000 && v <= 100000) {
			gen_freq = v;
			if (timer_inited) Update_PWM_Params();
			UART_Printf("OK 频率=%dHz\r\n", v);
		} else UART_SendStr("错误: 频率范围1000~100000Hz\r\n");
	}
	else if (strncmp(cmd_buf, "DUTY=", 5) == 0) {
		int v = atoi(cmd_buf + 5);
		if (v >= 1 && v <= 99) {
			gen_duty = v;
			if (timer_inited) Update_PWM_Params();
			UART_Printf("OK 占空比=%d%%\r\n", v);
		} else UART_SendStr("错误: 占空比范围1~99%\r\n");
	}
	else if (strncmp(cmd_buf, "PULSES=", 7) == 0) {
		int v = atoi(cmd_buf + 7);
		if (v >= 1 && v <= 500) {
			gen_count = v;
			UART_Printf("OK 每轮脉冲数=%d\r\n", v);
		} else UART_SendStr("错误: 脉冲数范围1~500\r\n");
	}
	else if (strcmp(cmd_buf, "START") == 0) {
		if (!gen_running) Generator_Start();
		else UART_SendStr("已在运行中\r\n");
	}
	else if (strcmp(cmd_buf, "STOP") == 0) {
		Generator_Stop();
	}
	else if (strcmp(cmd_buf, "STATUS?") == 0) {
		UART_Printf(
			"状态=%s 频率=%luHz 占空比=%lu%% 每轮脉冲=%lu 当前=%lu 已发轮数=%lu\r\n",
			gen_running ? "运行中" : "已停止",
			gen_freq, gen_duty, gen_count, gen_cur_pulse, gen_rounds);
	}
	else if (strcmp(cmd_buf, "HELP") == 0) {
		UART_SendStr(
			"===== 脉冲发生器命令 =====\r\n"
			"FREQ=10000    频率 Hz (1000~100000)\r\n"
			"DUTY=50       占空比 % (1~99)\r\n"
			"PULSES=5      每轮脉冲数 (1~500)\r\n"
			"START         启动 (每5秒一轮)\r\n"
			"STOP          停止\r\n"
			"STATUS?       查看当前状态\r\n"
			"HELP          显示帮助\r\n");
	}
	else UART_SendStr("错误: 无效命令, 发送 HELP 查看帮助\r\n");
}

/* ========== ISR ========== */
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
		Serial_RxData = USART_ReceiveData(USART1);
		Serial_RxFlag = 1;
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		if (gen_running) {
			pulse_cnt     = 0;
			gen_cur_pulse = 0;
			Update_PWM_Params();
			TIM_Cmd(TIM3, ENABLE);
		}
	}
}

void TIM3_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		pulse_cnt++;
		gen_cur_pulse = pulse_cnt;
		if (pulse_cnt >= gen_count) {
			TIM_Cmd(TIM3, DISABLE);
			gen_rounds++;
		}
	}
}

/* ========== 主循环 ========== */
int main(void)
{
	UART_Init();

	UART_SendStr("\r\n===== 脉冲发生器 V1 =====\r\n");
	UART_Printf("频率=%luHz 占空比=%lu%% 每轮脉冲=%lu\r\n", gen_freq, gen_duty, gen_count);
	UART_SendStr("发送 HELP 查看命令列表\r\n");

	while (1) {
		while (Serial_RxFlag) {
			Serial_RxFlag = 0;
			char ch = Serial_RxData;
			if (ch == '\r' || ch == '\n') {
				if (cmd_idx > 0) {
					cmd_buf[cmd_idx] = '\0';
					cmd_idx = 0;
					ParseCommand();
				}
			} else {
				if (cmd_idx < 31) cmd_buf[cmd_idx++] = ch;
			}
		}
	}
}
