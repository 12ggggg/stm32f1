#ifndef __BSP_GPS_H
#define __BSP_GPS_H

#include "stm32f10x.h"
#include "app_data.h"
#include <stdint.h>

/* ── 硬件配置 ── */
#define GPS_USART            USART2
#define GPS_USART_CLK        RCC_APB1Periph_USART2
#define GPS_GPIO_CLK         RCC_APB2Periph_GPIOA
#define GPS_GPIO_PORT        GPIOA
#define GPS_TX_PIN           GPIO_Pin_2
#define GPS_RX_PIN           GPIO_Pin_3
#define GPS_IRQn             USART2_IRQn
#define GPS_BAUDRATE         9600

#define GPS_BUF_SIZE         128

extern int g_gps_irq_count;
/* ── 对外接口 ── */
void    bsp_gps_init(void);
uint8_t bsp_gps_get_data(GPS_Data_t *out);

void bsp_gps_rx_printf(void);
#endif