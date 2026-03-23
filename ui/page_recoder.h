#ifndef __PAGE_RECODER_H
#define __PAGE_RECODER_H

#include "lvgl.h"
#include <stdint.h>

/* ── 录音应用数据结构 ── */
typedef struct {
    uint32_t duration_sec;   // 当前录音时长（秒）
    uint8_t  is_recording;   // 录音状态：0-停止，1-正在录制
    uint8_t  sd_status;      // SD卡状态：0-异常，1-正常
    char     current_fn[32]; // 当前录音文件名
} Record_Data_t;

/* ── 页面核心接口 ── */
void page_record_open(void);
void page_record_update(Record_Data_t *data);
void page_record_refresh_list(void);

#endif /* __PAGE_RECODER_H */