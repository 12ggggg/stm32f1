#include "../../lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "menu_list.h"
#include "app_data.h"



// LVGL任务
void task_lvgl(void *pv)
{
    // LVGL初始化放在任务里
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    
	static uint8_t s_printed = 0;
    Menu_Init();
    for(;;)
    {
			  LVGL_LOCK();
        lv_task_handler();
			  LVGL_UNLOCK();
			

//        if (!s_printed) {
//            s_printed = 1;
//            printf(">> lvgl watermark: %d\r\n",
//                   uxTaskGetStackHighWaterMark(NULL));
//        }
				
        vTaskDelay(pdMS_TO_TICKS(5)); // 每5ms跑一次
    }
}

void task_lvgl_init(void)
{
	BaseType_t result=pdFALSE;
  // 创建LVGL任务
    result=xTaskCreate(task_lvgl,    // 任务函数
                "task_lvgl",       // 任务名
                1024,         // 栈大小
                NULL,         // 参数
                5,            // 优先级
                NULL);        // 句柄
	   if(result==pdFALSE)
	 {
		 printf("task_lvgl create failed!\r\n");
	 }
	 printf("task_lvgl create successful!\r\n");
}