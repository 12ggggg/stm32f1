#ifndef __PAGE_SD_H
#define __PAGE_SD_H

#include "lvgl.h"
#include "app_data.h"

void page_sd_open(void);
void page_sd_update(SD_Data_t *data);
void page_sd_update_files(char files[][32], uint8_t count);

#endif