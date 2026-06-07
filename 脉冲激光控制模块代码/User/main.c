#include "stm32f10x.h"                  // Device header
#include "Serial.h"
#include "state_machine.h"
#include "timer2_capture.h"
#include "timer3_gap.h"
#include "timer4_pulse.h"
#include "uart_cmd.h"
#include "param_store.h"
#include "fifo.h"

/* FIFO 数据区定义 */
volatile uint16_t pulse_fifo[FIFO_SIZE];
volatile uint16_t fifo_head = 0;
volatile uint16_t fifo_tail = 0;

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	/* GPIO: PA1 推挽输出(脉冲输出); PC13 LED */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);		/* 初始低电平 */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_13);		/* LED 初始灭(PC13低有效) */

	/* 模块初始化 */
	Serial_Init();
	Timer2_Capture_Init();		/* TIM2 CH1: PA0 脉冲输入 */
	Timer4_Pulse_Init();		/* TIM4: 脉宽输出计时 */
	Timer3_Gap_Init();			/* TIM3: 间隙等待计时 */

	/* 从 Flash 加载间隙参数 */
	g_gap_ms = param_load();

	Serial_Printf("Pulse Laser Control v2\r\n");
	Serial_Printf("GAP=%lums, Stream: N in -> N out\r\n", g_gap_ms);

	Timer2_Capture_Enable();    /* now start capturing — PA0 already connected */

	while (1) {
		process_pulse_output();		/* 消费者状态机 */
		process_uart_command();		/* 串口命令解析 */
	}
}
