#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include <stdio.h>
#include "main.h"
#include "FreeRTOS.h"
#include "timers.h"

/* 串口1-USART1 (用于printf) */
#define DEBUG_USARTx                   USART1
#define DEBUG_USART_CLK                RCC_APB2Periph_USART1
#define DEBUG_USART_APBxClkCmd         RCC_APB2PeriphClockCmd
#define DEBUG_USART_BAUDRATE           115200

/* USART1 GPIO 引脚 */
#define DEBUG_USART_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define DEBUG_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
#define DEBUG_USART_TX_GPIO_PORT       GPIOA   
#define DEBUG_USART_TX_GPIO_PIN        GPIO_Pin_9
#define DEBUG_USART_RX_GPIO_PORT       GPIOA
#define DEBUG_USART_RX_GPIO_PIN        GPIO_Pin_10

#define DEBUG_USART_IRQ                USART1_IRQn
#define DEBUG_USART_IRQHandler         USART1_IRQHandler

/* DMA 发送配置（USART1 TX -> DMA1 Channel4） */
#define DEBUG_USART_TX_DMA_CLK         RCC_AHBPeriph_DMA1
#define DEBUG_USART_TX_DMA_STREAM      DMA1_Channel4
#define DEBUG_USART_DMA_TX_IRQn        DMA1_Channel4_IRQn
#define DEBUG_USART_DMA_TX_IRQHandler  DMA1_Channel4_IRQHandler
#define DEBUG_USART_TX_DMA_TC_FLAG     DMA1_FLAG_TC4
#define DMA1_IT_TC4                    DMA1_IT_TC4


// 串口2-USART2
#define  USART2_CLK                RCC_APB1Periph_USART2
#define  USART2_APBxClkCmd         RCC_APB1PeriphClockCmd
#define  USART2_BAUDRATE           115200

// USART GPIO 引脚宏定义
#define  USART2_GPIO_CLK           (RCC_APB2Periph_GPIOA)
#define  USART2_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
    
#define  USART2_TX_GPIO_PORT         GPIOA   
#define  USART2_TX_GPIO_PIN          GPIO_Pin_2
#define  USART2_RX_GPIO_PORT       GPIOA
#define  USART2_RX_GPIO_PIN        GPIO_Pin_3

#define  USART2_IRQ                USART2_IRQn
#define  USART2_IRQHandler         USART2_IRQHandler
/* 串口3-USART3 (用于通信) */
#define USART3_CLK                     RCC_APB1Periph_USART3
#define USART3_APBxClkCmd              RCC_APB1PeriphClockCmd
#define USART3_BAUDRATE                115200

#define USART3_GPIO_CLK                (RCC_APB2Periph_GPIOB)
#define USART3_GPIO_APBxClkCmd         RCC_APB2PeriphClockCmd
#define USART3_TX_GPIO_PORT            GPIOB   
#define USART3_TX_GPIO_PIN             GPIO_Pin_10
#define USART3_RX_GPIO_PORT            GPIOB
#define USART3_RX_GPIO_PIN             GPIO_Pin_11

#define USART3_IRQ                     USART3_IRQn
#define USART3_IRQHandler              USART3_IRQHandler

/* 回调函数类型定义 */
typedef void (*usart_rx_cb_t)(uint8_t ch);

/* 缓冲区大小 */
#define TX_BUF_SIZE 512

/* 函数声明 */
void Debug_USART_Config(void);
void USART_Config(void);

void usart_send_byte(uint8_t ch);
void usart_send_buf(uint8_t *buf, uint16_t len);
void usart_send_str(const char *s);
void usart_send_hex(uint8_t *data, uint16_t len);

void USART2_register_rx_cb(usart_rx_cb_t fn);

int blocking_printf(const char *fmt, ...);

#endif /* __USART_H */