#ifndef PAGE_GPS_H
#define PAGE_GPS_H

#include "lvgl.h"
#include "app_data.h" 

void page_gps_open(void);
void page_gps_update(GPS_Data_t *data);

#endif