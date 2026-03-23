#ifndef __TASK_MUSIC_H
#define __TASK_MUSIC_H

#include "FreeRTOS.h"
#include "task.h"
#include "app_data.h"

extern TaskHandle_t task_music_Handler;

void task_music_init(void);

#endif