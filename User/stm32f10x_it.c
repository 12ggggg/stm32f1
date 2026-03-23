/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI
  
  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "./lcd/bsp_xpt2046_lcd.h"
#include "./led/bsp_led.h"   
#include "./usart/bsp_usart.h"

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

#include "./led/bsp_led.h" 
/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */

//void HardFault_Handler(void)
//{   
//	GPIO_ResetBits(LED1_GPIO_PORT, LED1_GPIO_PIN);
//  /* Go to infinite loop when Hard Fault exception occurs */
//  while (1)
//  {
//   printf("HardFault!\n");
//  }
//}


/* 全局变量保存现场 */
volatile uint32_t hfault_args[8];
volatile uint32_t stacked_pc;
volatile uint32_t stacked_lr;
volatile uint32_t stacked_psr;

/* 入口：HardFault 被触发时自动调用 */
/* 把原来的整段汇编改成 Keil 语法 */
__asm void HardFault_Handler(void)
{
    IMPORT hardfault_c_handler
    tst lr, #4
    ite eq
    mrseq r0, msp
    mrsne r0, psp
    b hardfault_c_handler
}

/* C 语言处理函数 */
void hardfault_c_handler(uint32_t *args)
{
    /* 保存现场 */
    for (int i = 0; i < 8; i++) hfault_args[i] = args[i];
    stacked_pc = args[6];   // PC
    stacked_lr = args[5];   // LR
    stacked_psr = args[7];  // PSR

    printf("\nHardFault!\r\n");
    printf("PC = 0x%08X\r\n", stacked_pc);
    printf("LR = 0x%08X\r\n", stacked_lr);
    printf("PSR= 0x%08X\r\n", stacked_psr);

    /* 永远停住，方便断点 */
    while (1) __NOP();
}


/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
 */
//void SVC_Handler(void)
//{
//}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
//void PendSV_Handler(void)
//{
//}

#include "FreeRTOS.h"
#include "task.h"

#include "lvgl.h"  // 添加LVGL头文件
#include "lv_hal_tick.h"  // 如果你有的话，否则可以移除


/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void) {
	xPortSysTickHandler();
}


/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

#include "./sdio/bsp_sdio_sdcard.h"

void SDIO_IRQHandler(void) 
{
    SD_ProcessIRQSrc();
}



/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
