#ifndef _TASK_DRAW_H
#define _TASK_DRAW_H


#include"task.h"

extern TaskHandle_t task_draw_Handler; 
void task_draw_init();
void task_draw(void *pv);


#endif