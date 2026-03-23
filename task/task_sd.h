#ifndef __TASK_SD_H
#define __TASK_SD_H

#include "FreeRTOS.h"
#include "task.h"
#include "app_data.h"

#define MAX_FILES          8
#define SD_AUTO_REFRESH_MS 30000

extern TaskHandle_t task_sd_Handler;
extern char         s_file_names[MAX_FILES][32];
extern uint8_t      s_file_count;
extern char g_sd_delete_name[32];  // ´ıÉ¾³ıÎÄ¼şÃû

void task_sd_init(void);

#endif