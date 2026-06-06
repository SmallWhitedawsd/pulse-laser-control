#include "stm32f10x.h"                  // Device header
#include "Serial.h"

int main(void)
{
	/*串口初始化*/
	Serial_Init();

	/*发送欢迎信息*/
	Serial_SendString("USART1 Echo Test Ready.\r\n");
	Serial_SendString("Send data via serial assistant, MCU will echo back.\r\n\r\n");

	while (1)
	{
		/*回显已在USART1_IRQHandler中断中完成，主循环无需额外操作*/
	}
}
