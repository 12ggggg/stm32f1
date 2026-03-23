#include "app_data.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>

/* ════════════════════════════════════════
   全局数据（初始值）
   ════════════════════════════════════════ */
StatusBar_Data_t g_statusbar = {0};
Weather_Data_t   g_weather   = {0};
SD_Data_t        g_sd        = {0};
Music_State_t    g_music     = {0};

/* GPS默认值（室内调试用，实际定位后会被覆盖）*/
GPS_Data_t g_gps = {
    .lat          = 23.06865896f,
    .lon          = 112.55047835f,
    .altitude     = 20.0f,
    .satellites   = 0,
    .visible_sats = 0,
    .time         = "22:50:10",
    .valid        = 0,           // 默认无效，等GPS真正定位
};

/* 天气城市数据 */
Weather_Data_t city_data[CITY_COUNT] = {
    { "Shenzhen",  "Sunny",  28 },
    { "Guangzhou", "Cloudy", 26 },
    { "Beijing",   "Clear",  12 },
};
uint8_t s_city_idx = 0;

/* ════════════════════════════════════════
   互斥量句柄
   只保留真正需要的：LVGL操作耗时长需要互斥量
   数据拷贝直接用临界区，不需要互斥量
   ════════════════════════════════════════ */
SemaphoreHandle_t lvgl_Mutex    = NULL;

/* ════════════════════════════════════════
   初始化
   ════════════════════════════════════════ */
static void app_mutex_init(void)
{
    lvgl_Mutex = xSemaphoreCreateMutex();
    if (lvgl_Mutex == NULL)
        printf(">> lvgl_Mutex create failed!\r\n");
    else
        printf(">> lvgl_Mutex OK\r\n");
}

void app_Data_Init(void)
{
    app_mutex_init();
}