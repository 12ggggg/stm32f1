#ifndef __PAGE_WEATHER_H
#define __PAGE_WEATHER_H

#include "app_data.h"
#include "lvgl.h"

#define CITY_COUNT  3

// 外部可以访问当前选中的城市索引
extern uint8_t s_city_idx;

void page_weather_open(void);
void page_weather_update(Weather_Data_t *d);

#endif