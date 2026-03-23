#ifndef __TASK_WIFI_H
#define __TASK_WIFI_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t task_draw_Handler; 
void task_wifi(void *pv);
void task_wifi_init(void);

#endif