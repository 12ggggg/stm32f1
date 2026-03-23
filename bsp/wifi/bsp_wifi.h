#ifndef __BSP_WIFI_H
#define __BSP_WIFI_H

#include "stm32f10x.h"
#include <stdint.h>
#include "app_data.h"

/* ── 硬件引脚 ── */
#define WIFI_USART          USART3
#define WIFI_USART_IRQn     USART3_IRQn
#define WIFI_GPIO_PORT      GPIOB
#define WIFI_TX_PIN         GPIO_Pin_10
#define WIFI_RX_PIN         GPIO_Pin_11
#define WIFI_CH_PD_PIN      GPIO_Pin_8
#define WIFI_RST_PIN        GPIO_Pin_9

/* ── 缓冲区大小 ── */
#define WIFI_RX_BUF_SIZE    1024   // ← 改成1024，HTTP响应头+JSON约500字节

/* ── 返回值 ── */
typedef enum {
    WIFI_OK = 0,
    WIFI_ERROR,
    WIFI_TIMEOUT,
} WIFI_Status;

/* ── 接口 ── */
void         bsp_wifi_init(uint32_t baudrate);
void         WIFI_ClearRx(void);
void         WIFI_SendData(char *data);
void         WIFI_SendNum(uint16_t n);
char        *WIFI_GetResponse(void);
WIFI_Status  WIFI_SendCmd(char *cmd, char *expect, uint32_t timeout_ms);
WIFI_Status  WIFI_WaitResponse(const char *expect, uint32_t timeout_ms);
WIFI_Status  WIFI_Connect(char *ssid, char *password);
WIFI_Status  WIFI_GetConnectStatus(void);
WIFI_Status  WIFI_GetWeather(const char *key, const char *city,
                              char *res_buf, uint16_t buf_size);
int          Parse_Weather(const char *json, Weather_Data_t *out_data);

#endif