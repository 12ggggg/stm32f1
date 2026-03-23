#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "./usart/bsp_usart.h"
#include "./led/bsp_led.h"  

#include "task_gps.h"
#include "bsp_gps.h"
#include"app_data.h"

#define GPS_REFRESH_MS 2000

TaskHandle_t task_gps_Handler=NULL; 

extern GPS_Data_t g_gps;

void task_gps(void *pv)
{
    uint32_t  notify_val = 0;
	  BaseType_t got;
	  GPS_Data_t temp;
	  bsp_gps_init();

    for(;;) {
		  // 等刷新按钮通知，超时则自动刷新
        got = xTaskNotifyWait(0x00, 0xFFFFFFFF,
                              &notify_val,
                              pdMS_TO_TICKS(GPS_REFRESH_MS));

        // 先写临时变量，解析成功再加锁写全局
        if (bsp_gps_get_data(&temp))
        {
            GPS_Data_LOCK();
            g_gps = temp;           // 锁内赋值，原子安全
            GPS_Data_UNLOCK();
            UI_NOTIFY(UI_EVT_GPS);  // 解析成功就通知UI
        }

        // 收到刷新按钮通知，立刻推一次 UI（用当前缓存）
        if (got == pdTRUE && (notify_val & GPS_EVT_UPDATE_DATA))
        {
					 printf("notify gps ui update\r\n"); 
            UI_NOTIFY(UI_EVT_GPS);
        }
				
//				static uint8_t s_printed = 0;
//        if (!s_printed) {
//            s_printed = 1;
//            printf(">> gps watermark: %d\r\n",
//                   uxTaskGetStackHighWaterMark(NULL));
//        }
				
				
    }
}

void task_gps_init(void)
{
	  BaseType_t  result=pdFALSE;
	
	
    result=xTaskCreate(task_gps,    // 任务函数
                "task_gps",       // 任务名
                200,         // 栈大小
                NULL,         // 参数
                3,            // 优先级
                &task_gps_Handler);        // 句柄
   if(result==pdFALSE)
	 {
		 printf("task_gps create failed!\r\n");
	 }
	 printf("task_gps create successful!\r\n");
}