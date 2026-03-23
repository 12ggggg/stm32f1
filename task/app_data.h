#ifndef __APP_DATA_H
#define __APP_DATA_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* ════════════════════════════════════════
   数据结构定义
   ════════════════════════════════════════ */
typedef struct {
    float    lat;
    float    lon;
    float    altitude;
    uint8_t  satellites;
    uint8_t  visible_sats;
    char     time[12];
    uint8_t  valid;
} GPS_Data_t;

typedef struct {
    char   city[16];
    char   desc[16];
    int8_t temp_cur;
} Weather_Data_t;

typedef struct {
    uint32_t total_mb;
    uint32_t used_mb;
    uint8_t  mounted;
    uint16_t cached;
    uint16_t cache_total;
} SD_Data_t;

typedef struct {
    char     filename[32];
    uint16_t decode_time;
    uint8_t  is_playing;
    uint8_t  song_idx;
    uint8_t  song_total;
    uint8_t  volume;
    uint8_t  speed;
} Music_State_t;

typedef struct {
    char    time[12];
    uint8_t gps_valid;
    uint8_t wifi_connected;
    uint8_t sd_connected;
} StatusBar_Data_t;

/* ════════════════════════════════════════
   全局数据
   ════════════════════════════════════════ */
extern StatusBar_Data_t g_statusbar;
extern Weather_Data_t   g_weather;
extern GPS_Data_t       g_gps;
extern Music_State_t    g_music;
extern SD_Data_t        g_sd;

#define CITY_COUNT 3
extern Weather_Data_t city_data[CITY_COUNT];
extern uint8_t        s_city_idx;

/* ════════════════════════════════════════
   互斥量
   ════════════════════════════════════════ */
extern SemaphoreHandle_t lvgl_Mutex;

/* ════════════════════════════════════════
   线程安全宏
   
   原则：
   - LVGL操作耗时长 → 互斥量
   - 简单结构体拷贝 → 临界区
   ════════════════════════════════════════ */

/* LVGL互斥量（唯一的互斥量）*/
#define LVGL_LOCK()   xSemaphoreTake(lvgl_Mutex, portMAX_DELAY)
#define LVGL_UNLOCK() xSemaphoreGive(lvgl_Mutex)

/* 数据保护用临界区（结构体拷贝够快，不需要互斥量）*/
#define GPS_Data_LOCK()      taskENTER_CRITICAL()
#define GPS_Data_UNLOCK()    taskEXIT_CRITICAL()

#define WEATHER_Data_LOCK()  taskENTER_CRITICAL()
#define WEATHER_Data_UNLOCK() taskEXIT_CRITICAL()

#define MUSIC_Data_LOCK()    taskENTER_CRITICAL()
#define MUSIC_Data_UNLOCK()  taskEXIT_CRITICAL()

#define SD_Data_LOCK()       taskENTER_CRITICAL()
#define SD_Data_UNLOCK()     taskEXIT_CRITICAL()

/* ════════════════════════════════════════
   任务句柄
   ════════════════════════════════════════ */
extern TaskHandle_t task_draw_Handler;
extern TaskHandle_t task_gps_Handler;
extern TaskHandle_t task_wifi_Handler;
extern TaskHandle_t task_sd_Handler;
extern TaskHandle_t task_music_Handler;

/* ════════════════════════════════════════
   通知宏
   ════════════════════════════════════════ */
#define UI_NOTIFY(bit)  do { if (task_draw_Handler) xTaskNotify(task_draw_Handler, (bit), eSetBits); } while(0)

/* ════════════════════════════════════════
   事件位定义
   ════════════════════════════════════════ */
/* task_draw接收 */
#define UI_EVT_WIFI_WEATHER  (1 << 0)
#define UI_EVT_STBAR         (1 << 1)
#define UI_EVT_GPS           (1 << 2)
#define UI_EVT_SD            (1 << 3)
#define UI_EVT_MUSIC         (1 << 4)

/* task_wifi接收 */
#define WIFI_EVT_FETCH_WEATHER (1 << 0)
#define WIFI_EVT_RECONNECT     (1 << 1)

/* task_gps接收 */
#define GPS_EVT_UPDATE_DATA    (1 << 0)

/* task_sd接收 */
#define SD_EVT_REFRESH         (1 << 0)
#define SD_EVT_DELETE          (1 << 1)

/* task_music接收 */
#define MUSIC_EVT_PAUSE        (1 << 0)
#define MUSIC_EVT_NEXT         (1 << 1)
#define MUSIC_EVT_PREV         (1 << 2)
#define MUSIC_EVT_STOP         (1 << 3)
#define MUSIC_EVT_VOL_UP       (1 << 4)
#define MUSIC_EVT_VOL_DOWN     (1 << 5)
#define MUSIC_EVT_SPEED        (1 << 6)

/* ════════════════════════════════════════
   初始化
   ════════════════════════════════════════ */
void app_Data_Init(void);

#endif