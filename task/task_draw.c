#include <stdio.h>
#include "../../lvgl.h"
#include "FreeRTOS.h"
#include "task.h"

#include "menu_list.h"
#include "app_data.h"
#include "task_sd.h"

#include "page_music.h"
#include "page_gps.h"
#include "page_sd.h"
#include "page_music.h"
#include "page_weather.h" 
TaskHandle_t task_draw_Handler = NULL; 

/**
 * @brief UI 刷新任务
 * 职责：接收来自各个硬件任务（WiFi, GPS, 传感器）的通知，并安全地更新 LVGL 界面
 */
void task_draw(void *pv)
{
    uint32_t notify_value;
    

    for(;;)
    {
        /**
         * 阻塞等待通知
         * xTaskNotifyWait 的参数解释：
         * 0x00: 进入时不清除位
         * 0xFFFFFFFF: 退出时清除所有位（表示已处理）
         */
        if(xTaskNotifyWait(0x00, 0xFFFFFFFF, &notify_value, portMAX_DELAY) == pdTRUE)
        {
            // --- 核心规范：操作 LVGL 必须加锁 ---
            // 假设你的工程已经定义了 LVGL_LOCK/UNLOCK（通常是互斥锁）
            // 如果没有定义，请确保你的 lv_timer_handler 在另一个任务里有互斥机制
            
            LVGL_LOCK(); 

            // 1. 处理天气更新事件
            if(notify_value & UI_EVT_WIFI_WEATHER) 
            {
                printf(">> UI_LOG: Received Weather Update\r\n");
                
                // 调用 page_weather.c 里的刷新函数
                // 它会读取最新的 city_data[s_city_idx] 并更新 Label
              Weather_Data_t local;
							WEATHER_Data_LOCK();
							local = city_data[s_city_idx];
							WEATHER_Data_UNLOCK();

							page_weather_update(&local);
            }

            // 2. 处理状态栏更新事件 (WiFi 图标颜色、电池等)
            if(notify_value & UI_EVT_STBAR)
            {
                // menu_update_statusbar(); // 假设你有这个函数
            }
            // 3. 处理 GPS 或其他事件
					if (notify_value & UI_EVT_GPS)
					{
						 printf("gps ui updata\r\n");
						 GPS_Data_t local;
						GPS_Data_LOCK();
						local = g_gps;
						GPS_Data_UNLOCK();
						page_gps_update(&local);
					}
					if (notify_value & UI_EVT_SD)
				{
					 SD_Data_t local_sd;
    SD_Data_LOCK();
    local_sd = g_sd;
    SD_Data_UNLOCK();
    page_sd_update(&local_sd);
    page_sd_update_files(s_file_names, s_file_count);
				}
				if (notify_value & UI_EVT_MUSIC)
{
    Music_State_t local;
    taskENTER_CRITICAL();
    local = g_music;
    taskEXIT_CRITICAL();
    page_music_update(&local);
}
             LVGL_UNLOCK();
        }
//				static uint8_t s_printed = 0;
//        if (!s_printed) {
//            s_printed = 1;
//            printf(">> draw watermark: %d\r\n",
//                   uxTaskGetStackHighWaterMark(NULL));
//        }
    }
}

void task_draw_init(void)
{
    BaseType_t result;

    result = xTaskCreate(task_draw,
                         "task_draw",
                         400,      // UI任务建议给 1024 或更高
                         NULL,
                         4,         // 优先级设为 4，略高于硬件任务
                         &task_draw_Handler); // 必须拿到句柄，否则别人没法通知你
    
    if(result == pdPASS) {
        printf("task_draw create successful!\r\n");
    } else {
        printf("task_draw create failed!\r\n");
    }
}