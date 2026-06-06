/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @brief   System exception handlers (Cortex-M3).
  *          Peripheral ISRs are defined in their respective driver files:
  *            USART1_IRQHandler -> Hardware/Serial.c
  *            TIM2_IRQHandler   -> Hardware/timer2_capture.c
  *            TIM3_IRQHandler   -> Hardware/timer3_gap.c
  *            TIM4_IRQHandler   -> Hardware/timer4_pulse.c
  ******************************************************************************
  */

#include "stm32f10x_it.h"

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

void NMI_Handler(void) {}

void HardFault_Handler(void)  { while (1); }

void MemManage_Handler(void)  { while (1); }

void BusFault_Handler(void)   { while (1); }

void UsageFault_Handler(void) { while (1); }

void SVC_Handler(void) {}

void DebugMon_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {}
