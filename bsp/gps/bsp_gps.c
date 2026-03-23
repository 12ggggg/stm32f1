#include "bsp_gps.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ════════════════════════════════════════
   内部缓冲
   ════════════════════════════════════════ */
static char    s_rx_buf[GPS_BUF_SIZE];
static char s_rmc_buf[GPS_BUF_SIZE] = {0};
static char s_gngga_buf[GPS_BUF_SIZE] = {0};
static uint8_t s_rx_idx = 0;

static volatile uint8_t s_rmc_ready  = 0;
static volatile uint8_t s_gngga_ready  = 0;
static volatile uint8_t s_data_ready = 0;
static volatile uint8_t s_sat_updated = 0;

static GPS_Data_t s_gps_data = {0};


/* ════════════════════════════════════════
   工具函数
   ════════════════════════════════════════ */
static void get_field(const char *s,
                      uint8_t    index,
                      char      *out,
                      uint8_t    len)
{
    uint8_t  comma = 0;
    uint16_t i     = 0;
    uint8_t  j     = 0;
    out[0] = '\0';

    while (s[i] != '\0' && s[i] != '*') {
        if (s[i] == ',') {
            comma++;
            i++;
            continue;
        }
        if (comma == index && j < len - 1)
            out[j++] = s[i];
        if (comma > index) break;
        i++;
    }
    out[j] = '\0';
}

static uint8_t checksum_ok(const char *s)
{
    uint8_t  calc = 0;
    uint16_t i    = 1;

    while (s[i] != '*' && s[i] != '\0')
        calc ^= (uint8_t)s[i++];

    if (s[i] != '*') return 0;

    char hex[3] = {s[i+1], s[i+2], '\0'};
    return (calc == (uint8_t)strtol(hex, NULL, 16)) ? 1 : 0;
}

static float to_decimal(const char *raw, const char *dir)
{
    if (!raw || raw[0] == '\0') return 0.0f;

    float val = atof(raw);
    int   deg = (int)(val / 100);
    float min = val - deg * 100.0f;
    float dec = deg + min / 60.0f;

    if (dir[0] == 'S' || dir[0] == 'W')
        dec = -dec;

    return dec;
}

/* ════════════════════════════════════════
   解析 $GNRMC
   $GNRMC,时间,A,纬度,N,经度,E,速度,航向,日期,,,A,V*校验
   字段:    1   2   3  4   5  6   7    8    9
   ════════════════════════════════════════ */
static void parse_gnrmc(const char *s)
{
    if (!checksum_ok(s)) return;

    char field[20];

    get_field(s, 2, field, sizeof(field));
    if (field[0] != 'A') {
        s_gps_data.valid = 0;
        return;
    }
    s_gps_data.valid = 1;

    get_field(s, 1, field, sizeof(field));
    if (field[0] != '\0') {
        snprintf(s_gps_data.time,
                 sizeof(s_gps_data.time),
                 "%c%c:%c%c:%c%c",
                 field[0], field[1],
                 field[2], field[3],
                 field[4], field[5]);
    }

    char lat[16], lat_d[4];
    get_field(s, 3, lat,   sizeof(lat));
    get_field(s, 4, lat_d, sizeof(lat_d));
    s_gps_data.lat = to_decimal(lat, lat_d);

    char lon[16], lon_d[4];
    get_field(s, 5, lon,   sizeof(lon));
    get_field(s, 6, lon_d, sizeof(lon_d));
    s_gps_data.lon = to_decimal(lon, lon_d);

}

/* ════════════════════════════════════════
   解析 $GNGGA
   $GNGGA,时间,纬度,N,经度,E,质量,卫星数,HDOP,海拔,M,,,*校验
   字段:    1    2  3   4  5   6     7     8    9
   ════════════════════════════════════════ */
static void parse_gngga(const char *s)
{
    if (!checksum_ok(s)) return;

    char field[20];

    /* 第7字段：卫星数（始终解析） */
    get_field(s, 7, field, sizeof(field));
    if (field[0] != '\0')
        s_gps_data.satellites = (uint8_t)atoi(field);

    /* 第9字段：海拔高度（可选） */
    get_field(s, 9, field, sizeof(field));
    if (field[0] != '\0')
        s_gps_data.altitude = atof(field);

    /* 标记卫星数已更新 */
    s_sat_updated = 1;
}

/* ════════════════════════════════════════
   硬件初始化
   ════════════════════════════════════════ */
void bsp_gps_init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef  NVIC_InitStruct;

    RCC_APB1PeriphClockCmd(GPS_USART_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(GPS_GPIO_CLK,  ENABLE);

    GPIO_InitStruct.GPIO_Pin   = GPS_TX_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPS_GPIO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.GPIO_Pin  = GPS_RX_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPS_GPIO_PORT, &GPIO_InitStruct);

    USART_InitStruct.USART_BaudRate            = GPS_BAUDRATE;
    USART_InitStruct.USART_WordLength          = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits            = USART_StopBits_1;
    USART_InitStruct.USART_Parity              = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(GPS_USART, &USART_InitStruct);

    USART_ITConfig(GPS_USART, USART_IT_RXNE, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannel                   = GPS_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    USART_Cmd(GPS_USART, ENABLE);
} 
/* ════════════════════════════════════════
   中断：只收字符 + memcpy，不解析
   ════════════════════════════════════════ */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(GPS_USART, USART_IT_RXNE) == RESET) return;

    char ch = (char)USART_ReceiveData(GPS_USART);
    if (ch == '$') {
        s_rx_idx    = 0;
        s_rx_buf[0] = '\0';
    }

    if (s_rx_idx < GPS_BUF_SIZE - 1) {
        s_rx_buf[s_rx_idx++] = ch;
        s_rx_buf[s_rx_idx]   = '\0';
    }
    if (ch == '\n') {
        if (strncmp(s_rx_buf, "$GNRMC", 6) == 0) {
            memcpy(s_rmc_buf, s_rx_buf, s_rx_idx + 1);
            s_rmc_ready = 1;
        } else if (strncmp(s_rx_buf, "$GNGGA", 6) == 0) {
            memcpy(s_gngga_buf, s_rx_buf, s_rx_idx + 1);
            s_gngga_ready = 1;
        } else if (strncmp(s_rx_buf, "$GNGLL", 6) == 0) {
            s_data_ready = 1;
        }
        s_rx_idx = 0;
    }
}

/* ════════════════════════════════════════
   对外接口
   ════════════════════════════════════════ */
uint8_t bsp_gps_get_data(GPS_Data_t *out)
{
    /* 1. 优先解析 GGA（卫星数） */
    if (s_gngga_ready) {
        taskENTER_CRITICAL();
        char local[GPS_BUF_SIZE];
        memcpy(local, s_gngga_buf, GPS_BUF_SIZE);
        s_gngga_ready = 0;
        taskEXIT_CRITICAL();
        parse_gngga(local);
        /* parse_gngga 内部已设置 s_sat_updated = 1 */
    }

    /* 2. 解析 RMC（位置、时间、有效标志） */
    if (s_rmc_ready) {
        taskENTER_CRITICAL();
        char local[GPS_BUF_SIZE];
        memcpy(local, s_rmc_buf, GPS_BUF_SIZE);
        s_rmc_ready = 0;
        taskEXIT_CRITICAL();
        parse_gnrmc(local);
    }

    /* 3. 卫星数有更新 → 立即返回数据 */
    uint8_t ret = 0;
    if (s_sat_updated) {
        taskENTER_CRITICAL();
        s_sat_updated = 0;
        *out = s_gps_data;   // 复制当前所有数据（包含可能为0的 valid）
        taskEXIT_CRITICAL();
        ret = 1;
    }

    /* 4. 收到 GLL 完整数据（即有效定位） → 返回数据 */
    if (s_data_ready) {
        taskENTER_CRITICAL();
        s_data_ready = 0;
        *out = s_gps_data;
        taskEXIT_CRITICAL();
        ret = 1;
    }

    return ret;
}
/* ════════════════════════════════════════
   调试：打印原始帧内容
   ════════════════════════════════════════ */
void bsp_gps_rx_printf(void)
{
    printf("RMC=[%s]\r\n", s_rmc_buf);
    printf("GGA=[%s]\r\n", s_gngga_buf);
}