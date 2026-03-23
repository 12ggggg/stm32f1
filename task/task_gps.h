#ifndef __TASK_GPS_H
#define __TASK_GPS_H

#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t task_draw_Handler; 
void task_gps(void *pv);
void task_gps_init(void);

#endif