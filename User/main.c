#include "main.h"
#include "delay.h"

#include "app_data.h"

#include "task_gps.h"
#include "task_lvgl.h"
#include "task_draw.h"
#include "task_wifi.h"
#include "task_music.h"


int main(void)
{
    // 硬件初始化
    ILI9341_Init();
    XPT2046_Init();
    ILI9341_GramScan(6);
    Debug_USART_Config();
    LED_GPIO_Config();
	  
	  //Delay_Init();
	  //test_wifi();
	  
      app_Data_Init();	  
      task_lvgl_init();  
    	task_draw_init();	 
    	task_wifi_init();
      task_gps_init();
	    task_sd_init();
      task_music_init();
//   SD_test();
	  printf("Free Heap: %d bytes\r\n", xPortGetFreeHeapSize());
    vTaskStartScheduler();
	  while(1);
}



void vApplicationTickHook()
{
	// 告诉lvgl已经过去了1毫秒
	lv_tick_inc(1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("STACK OVERFLOW: %s\r\n", pcTaskName);
    while(1);
}


/* ------------------------------------------end of file---------------------------------------- */

