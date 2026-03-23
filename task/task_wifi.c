#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include "app_data.h"
#include "bsp_wifi.h"
#include "task_wifi.h"

#define WEATHER_KEY         "SDIGuEC-o_u8u9f1J"
#define WIFI_SSID           "linxuezhi"
#define WIFI_PASSWORD       "li88888888"
#define WIFI_MAX_RETRY      3
#define WEATHER_REFRESH_MS  60000

TaskHandle_t task_wifi_Handler = NULL;

static char s_result[600];

static uint8_t ensure_connected(void);
static void    do_fetch_weather(void);

/* ════════════════════════════════════════
   任务主体
   ════════════════════════════════════════ */
void task_wifi(void *pv)
{
    uint32_t   notify_val = 0;
    BaseType_t got;

    /* ── 等LVGL画完首屏再启动硬件，避免FSMC干扰 ── */
    vTaskDelay(pdMS_TO_TICKS(2000));

    bsp_wifi_init(115200);
    GPIO_SetBits(WIFI_GPIO_PORT, WIFI_CH_PD_PIN);
    GPIO_SetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf(">> WiFi: Ready, waiting for trigger...\r\n");

    for (;;)
    {
        /* ── 阻塞等待，直到有人点刷新或60秒超时 ── */
        got = xTaskNotifyWait(0x00, 0xFFFFFFFF,
                              &notify_val,
                              pdMS_TO_TICKS(WEATHER_REFRESH_MS));

        if (got == pdFALSE) {
            printf(">> WiFi: [AUTO] 60s timer fired\r\n");
        } else if (notify_val & WIFI_EVT_FETCH_WEATHER) {
            printf(">> WiFi: [MANUAL] Refresh button pressed\r\n");
        } else {
            continue;  // 其他通知，忽略
        }

        /* ── 先确保WiFi连接 ── */
        printf(">> WiFi: Checking connection...\r\n");
        if (!ensure_connected()) {
            printf(">> WiFi: Cannot connect, skip fetch\r\n");
            continue;
        }

        /* ── 拉取天气 ── */
        do_fetch_weather();
    }
}

/* ════════════════════════════════════════
   确保WiFi连接
   返回 1=已连接  0=连接失败
   ════════════════════════════════════════ */
static uint8_t ensure_connected(void)
{
    /* 先查在不在线 */
    if (WIFI_GetConnectStatus() == WIFI_OK) {
        printf(">> WiFi: Already connected\r\n");
        g_statusbar.wifi_connected = 1;
        UI_NOTIFY(UI_EVT_STBAR);
        return 1;
    }

    /* 不在线，尝试连接最多WIFI_MAX_RETRY次 */
    for (uint8_t i = 0; i < WIFI_MAX_RETRY; i++) {
        printf(">> WiFi: Connecting [%s] (%d/%d)...\r\n",
               WIFI_SSID, i + 1, WIFI_MAX_RETRY);

        if (WIFI_Connect(WIFI_SSID, WIFI_PASSWORD) == WIFI_OK) {
            printf(">> WiFi: Connected!\r\n");
            g_statusbar.wifi_connected = 1;
            UI_NOTIFY(UI_EVT_STBAR);
            return 1;
        }

        printf(">> WiFi: Attempt %d failed\r\n", i + 1);

        /* 最后一次失败才做硬件复位 */
        if (i == WIFI_MAX_RETRY - 1) {
            printf(">> WiFi: HW reset...\r\n");
            GPIO_ResetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);
            vTaskDelay(pdMS_TO_TICKS(500));
            GPIO_SetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);
            vTaskDelay(pdMS_TO_TICKS(1000));
            WIFI_ClearRx();
        } else {
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    printf(">> WiFi: All retries failed\r\n");
    g_statusbar.wifi_connected = 0;
    UI_NOTIFY(UI_EVT_STBAR);
    return 0;
}

/* ════════════════════════════════════════
   获取天气并通知UI
   ════════════════════════════════════════ */
static void do_fetch_weather(void)
{
    const char *city = city_data[s_city_idx].city;
    printf(">> WiFi: Fetching weather [%s]...\r\n", city);

    if (WIFI_GetWeather(WEATHER_KEY, city,
                        s_result, sizeof(s_result)) != WIFI_OK) {
        printf(">> WiFi: Fetch failed\r\n");
        return;
    }

    if (Parse_Weather(s_result, &g_weather) == -1) {
        printf(">> WiFi: Parse failed\r\n");
        return;
    }

    strncpy(g_weather.city, city, sizeof(g_weather.city) - 1);
    g_weather.city[sizeof(g_weather.city) - 1] = '\0';

    WEATHER_Data_LOCK();
    city_data[s_city_idx].temp_cur = g_weather.temp_cur;
    strncpy(city_data[s_city_idx].desc, g_weather.desc,
            sizeof(city_data[s_city_idx].desc) - 1);
    city_data[s_city_idx].desc[
        sizeof(city_data[s_city_idx].desc) - 1] = '\0';
    WEATHER_Data_UNLOCK();

    UI_NOTIFY(UI_EVT_WIFI_WEATHER);
    printf(">> WiFi: Done! temp=%d°C desc=%s\r\n",
           g_weather.temp_cur, g_weather.desc);
}

/* ════════════════════════════════════════
   任务创建
   ════════════════════════════════════════ */
void task_wifi_init(void)
{
    BaseType_t ret = xTaskCreate(task_wifi,
                                  "task_wifi",
                                  800,
                                  NULL,
                                  2,
                                  &task_wifi_Handler);
    if (ret != pdPASS)
        printf("task_sd create failed!\r\n");
    else
        printf("task_sd create successful!\r\n");
}
