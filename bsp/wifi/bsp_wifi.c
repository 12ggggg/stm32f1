#include <string.h>
#include "delay.h"
#include "bsp_wifi.h"
#include "app_data.h"
#include "FreeRTOS.h"
#include "task.h"

/* ── 接收缓冲区 ── */
static uint8_t          wifi_rx_buf[WIFI_RX_BUF_SIZE];
static volatile uint16_t wifi_rx_len = 0;

/* ── 天气描述映射表 ── */
typedef struct {
    const char *chinese;
    const char *english;
} Weather_Map_t;

static const Weather_Map_t g_weather_map[] = {
    {"晴",   "Sunny"},
    {"多云", "Cloudy"},
    {"阴",   "Overcast"},
    {"阵雨", "Shower"},
    {"雷阵雨","T-Storm"},
    {"小雨", "L-Rain"},
    {"中雨", "M-Rain"},
    {"大雨", "H-Rain"},
    {"雨夹雪","Sleet"},
    {"小雪", "L-Snow"},
    {"阵雪", "Flurry"},
    {"雾",   "Foggy"},
    {"霾",   "Haze"},
    {"未知", "Unknown"},
};

/* ════════════════════════════════════════
   工具函数
   ════════════════════════════════════════ */
static void map_weather_text(const char *json_ptr,
                              char *out_en, int max_len)
{
    for (int i = 0;
         i < (int)(sizeof(g_weather_map)/sizeof(Weather_Map_t));
         i++)
    {
        if (strncmp(json_ptr,
                    g_weather_map[i].chinese,
                    strlen(g_weather_map[i].chinese)) == 0)
        {
            strncpy(out_en, g_weather_map[i].english, max_len - 1);
            out_en[max_len - 1] = '\0';
            return;
        }
    }
    strncpy(out_en, "Other", max_len - 1);
    out_en[max_len - 1] = '\0';
}

void WIFI_SendNum(uint16_t n)
{
    char    buf[6];
    uint8_t i = 0;
    if (n == 0) { WIFI_SendData("0"); return; }
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    while (i--) {
        while (USART_GetFlagStatus(WIFI_USART,
                                    USART_FLAG_TXE) == RESET);
        USART_SendData(WIFI_USART, buf[i]);
    }
}

/* ════════════════════════════════════════
   硬件初始化
   ════════════════════════════════════════ */
void bsp_wifi_init(uint32_t baudrate)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    /* TX PB10 */
    GPIO_InitStructure.GPIO_Pin   = WIFI_TX_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(WIFI_GPIO_PORT, &GPIO_InitStructure);

    /* RX PB11 */
    GPIO_InitStructure.GPIO_Pin  = WIFI_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(WIFI_GPIO_PORT, &GPIO_InitStructure);

    /* EN PB8, RST PB9 */
    GPIO_InitStructure.GPIO_Pin  = WIFI_CH_PD_PIN | WIFI_RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(WIFI_GPIO_PORT, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx
                                                  | USART_Mode_Rx;
    USART_Init(WIFI_USART, &USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = WIFI_USART_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(WIFI_USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(WIFI_USART, ENABLE);
}

/* ════════════════════════════════════════
   接收中断
   ════════════════════════════════════════ */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(WIFI_USART, USART_IT_RXNE) != RESET) {
        uint8_t ch = USART_ReceiveData(WIFI_USART);
        if (wifi_rx_len < WIFI_RX_BUF_SIZE - 1) {
            wifi_rx_buf[wifi_rx_len++] = ch;
            wifi_rx_buf[wifi_rx_len]   = '\0';
        }
        USART_ClearITPendingBit(WIFI_USART, USART_IT_RXNE);
    }
}

/* ════════════════════════════════════════
   基础操作
   ════════════════════════════════════════ */
void WIFI_ClearRx(void)
{
    memset(wifi_rx_buf, 0, WIFI_RX_BUF_SIZE);
    wifi_rx_len = 0;
}

char *WIFI_GetResponse(void)
{
    return (char *)wifi_rx_buf;
}

void WIFI_SendData(char *data)
{
    for (char *p = data; *p; p++) {
        while (USART_GetFlagStatus(WIFI_USART,
                                    USART_FLAG_TXE) == RESET);
        USART_SendData(WIFI_USART, (uint8_t)*p);
    }
}

/* ════════════════════════════════════════
   发送命令等待响应
   ← 改动1：加临界区保护strstr，防止中断打断读取
   ════════════════════════════════════════ */
WIFI_Status WIFI_SendCmd(char *cmd, char *expect,
                          uint32_t timeout_ms)
{
    WIFI_ClearRx();
    WIFI_SendData(cmd);
    WIFI_SendData("\r\n");

    uint32_t tick = 0;
    while (tick < timeout_ms) {
        taskENTER_CRITICAL();                              // 加锁
        int found = (strstr((char*)wifi_rx_buf,
                             expect) != NULL);
        int err   = (strstr((char*)wifi_rx_buf,
                             "ERROR") != NULL);
        taskEXIT_CRITICAL();                               //  解锁

        if (found) return WIFI_OK;
        if (err)   return WIFI_ERROR;
        Delay_ms(10);
        tick += 10;
    }
    return WIFI_TIMEOUT;
}

/* ════════════════════════════════════════
   等待响应（不发命令）
   ════════════════════════════════════════ */
WIFI_Status WIFI_WaitResponse(const char *expect,
                               uint32_t timeout_ms)
{
    uint32_t tick = 0;
    while (tick < timeout_ms) {
        taskENTER_CRITICAL();
        int found = (strstr((char*)wifi_rx_buf,
                             expect) != NULL);
        int err   = (strstr((char*)wifi_rx_buf,
                             "ERROR") != NULL);
        taskEXIT_CRITICAL();

        if (found) return WIFI_OK;
        if (err)   return WIFI_ERROR;
        Delay_ms(10);
        tick += 10;
    }
    return WIFI_TIMEOUT;
}

/* ════════════════════════════════════════
   连接WiFi
   ════════════════════════════════════════ */
WIFI_Status WIFI_Connect(char *WIFI_SSID, char *WIFI_PASSWORD)
{
    printf(">> [1/4] AT check...\r\n");
    if (WIFI_SendCmd("AT", "OK", 2000) != WIFI_OK) {
        printf(">> ESP8266 no response!\r\n");
        return WIFI_ERROR;
    }

    printf(">> [2/4] Station mode...\r\n");
    WIFI_SendCmd("AT+CWMODE=1", "OK", 1000);

    printf(">> [3/4] Connecting: %s\r\n", WIFI_SSID);
    WIFI_ClearRx();
    WIFI_SendData("AT+CWJAP=\"");
    WIFI_SendData(WIFI_SSID);
    WIFI_SendData("\",\"");
    WIFI_SendData(WIFI_PASSWORD);
    WIFI_SendData("\"\r\n");

    if (WIFI_WaitResponse("OK", 15000) != WIFI_OK) {
        printf(">> Connect failed: %s\r\n", WIFI_GetResponse());
        return WIFI_TIMEOUT;
    }
    printf(">> Connected!\r\n");

    printf(">> [4/4] ATE0...\r\n");
    WIFI_SendCmd("ATE0", "OK", 1000);

    return WIFI_OK;
}

/* ════════════════════════════════════════
   查询连接状态
   ════════════════════════════════════════ */
WIFI_Status WIFI_GetConnectStatus(void)
{
    if (WIFI_SendCmd("AT+CIPSTATUS",
                      "STATUS:2", 1000) == WIFI_OK)
        return WIFI_OK;
    return WIFI_ERROR;
}

/* ════════════════════════════════════════
   获取天气
   访问：api.seniverse.com/v3/weather/now.json
         ?key=xxx&location=xxx&language=zh-Hans&unit=c
   ════════════════════════════════════════ */
WIFI_Status WIFI_GetWeather(const char *key,
                             const char *city,
                             char       *res_buf,
                             uint16_t    buf_size)
{
    /* ── 建立TCP连接 ── */
    if (WIFI_SendCmd(
            "AT+CIPSTART=\"TCP\",\"api.seniverse.com\",80",
            "OK", 5000) != WIFI_OK) {
        printf(">> TCP connect failed\r\n");
        return WIFI_ERROR;
    }
    printf(">> TCP OK\r\n");

    /* ── 计算HTTP请求长度 ── */
    const char *p1 = "GET /v3/weather/now.json?key=";
    const char *p2 = "&location=";
    const char *p3 = "&language=zh-Hans&unit=c HTTP/1.1\r\n"
                     "Host: api.seniverse.com\r\n"
                     "Connection: close\r\n\r\n";

    uint16_t total_len = strlen(p1) + strlen(key)
                       + strlen(p2) + strlen(city)
                       + strlen(p3);

    /* ── 告诉ESP8266要发多少字节 ── */
    WIFI_ClearRx();
    WIFI_SendData("AT+CIPSEND=");
    WIFI_SendNum(total_len);
    WIFI_SendData("\r\n");

    if (WIFI_WaitResponse(">", 2000) != WIFI_OK) {
        printf(">> CIPSEND prompt timeout\r\n");
        return WIFI_ERROR;
    }
    printf(">> Sending HTTP request...\r\n");

    /* ── 发送HTTP请求 ── */
    WIFI_ClearRx();
    WIFI_SendData((char *)p1);
    WIFI_SendData((char *)key);
    WIFI_SendData((char *)p2);
    WIFI_SendData((char *)city);
    WIFI_SendData((char *)p3);

    /* ── 等待发送确认 ── */
    if (WIFI_WaitResponse("SEND OK", 3000) != WIFI_OK) {
        printf(">> SEND OK timeout\r\n");
        return WIFI_ERROR;
    }

    /* ── 等待服务器关闭连接（数据接收完毕） ── */
    if (WIFI_WaitResponse("CLOSED", 8000) != WIFI_OK) {
        printf(">> Wait CLOSED timeout (data may be ok)\r\n");
    }

    /* ── 等待最后数据接收完成 ── */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* ── 在缓冲区里找JSON ── */
    char *ipd = strstr((char *)wifi_rx_buf, "+IPD,");
    if (!ipd) {
        printf(">> No +IPD in response\r\n");
        printf(">> Raw: %.200s\r\n", wifi_rx_buf); // 打印前200字节调试
        return WIFI_TIMEOUT;
    }

    char *json = strchr(ipd, '{');
    if (!json) {
        printf(">> No JSON in response\r\n");
        return WIFI_TIMEOUT;
    }

    /* ── 安全拷贝JSON ── */
    size_t len = strlen(json);
    if (len >= buf_size) {
        len = buf_size - 1;
        printf(">> JSON truncated!\r\n");
    }
    strncpy(res_buf, json, len);
    res_buf[len] = '\0';

    printf(">> JSON OK len=%d\r\n", (int)len);
    return WIFI_OK;
}

/* ════════════════════════════════════════
   解析天气JSON
   响应格式：
   {"results":[{...,
     "now":{"text":"霾","code":"31","temperature":"17"},
     ...}]}
   ════════════════════════════════════════ */
int Parse_Weather(const char *json, Weather_Data_t *out_data)
{
    if (!json || !out_data) return -1;

    char *p;

    /* ── 提取温度 ── */
    p = strstr(json, "\"temperature\":\"");
    if (p) {
        out_data->temp_cur = (int8_t)atoi(p + 15);
    } else {
        printf(">> Parse: no temperature\r\n");
        return -1;
    }

    /* ── 提取天气描述 ── */
    p = strstr(json, "\"text\":\"");
    if (p) {
        map_weather_text(p + 8,
                         out_data->desc,
                         sizeof(out_data->desc));
    } else {
        strncpy(out_data->desc, "Unknown",
                sizeof(out_data->desc) - 1);
    }

    return 0;
}