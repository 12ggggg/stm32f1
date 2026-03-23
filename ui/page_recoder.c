#include "page_recoder.h"
#include "menu_list.h"
#include <stdio.h>
#include <string.h>

/* ── 内部静态变量 ── */
static lv_obj_t *s_time_lbl = NULL;
static lv_obj_t *s_rec_btn_lbl = NULL;
static lv_obj_t *s_rec_list = NULL;
static uint8_t  s_is_recording = 0;

/* 模拟数据：实际开发建议放在 sd_task 中扫描 */
#define MAX_REC_FILES 10
static char s_rec_files[MAX_REC_FILES][32] = {"REC001.WAV", "REC002.WAV"};
static int  s_file_count = 2;

/* ── 内部函数声明 ── */
static void refresh_rec_list_ui(void);
static void toggle_record_cb(lv_event_t *e);
static void back_cb(lv_event_t *e);

/* ════════════════════════════════════════
    1. 返回与清理逻辑
   ════════════════════════════════════════ */
static void back_cb(lv_event_t *e) {
    // 置空指针，防止后台 page_record_update 意外操作已销毁的对象
    s_time_lbl = NULL;
    s_rec_btn_lbl = NULL;
    s_rec_list = NULL;
    s_is_recording = 0;

    // 释放当前页面资源并重新初始化菜单
    menu_go_back();
}

/* ════════════════════════════════════════
    2. 录音开关逻辑
   ════════════════════════════════════════ */
static void toggle_record_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    s_is_recording = !s_is_recording;

    if (s_is_recording) {
        // UI 切换至“正在录音”状态
        lv_label_set_text(s_rec_btn_lbl, LV_SYMBOL_STOP " STOP");
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF4444), 0); // 红色表示停止
        
        /* 
           TODO: 等你定义好任务后在此添加：
           xTaskNotify(task_record_handler, START_SIGNAL, eSetBits); 
        */
      //  printf("Record started (Simulation)\n");
    } else {
        // UI 切换至“就绪”状态
        lv_label_set_text(s_rec_btn_lbl, LV_SYMBOL_PLAY " START");
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1DB954), 0); // 绿色表示开始

        /* 
           TODO: 等你定义好任务后在此添加：
           xTaskNotify(task_record_handler, STOP_SIGNAL, eSetBits); 
        */

        // 模拟生成新文件并刷新列表
        if (s_file_count < MAX_REC_FILES) {
            snprintf(s_rec_files[s_file_count], 32, "REC%03d.WAV", s_file_count + 1);
            s_file_count++;
            refresh_rec_list_ui();
        }
    }
}

/* ════════════════════════════════════════
    3. UI 刷新逻辑
   ════════════════════════════════════════ */
static void refresh_rec_list_ui(void) {
    if (!s_rec_list) return;

    lv_obj_clean(s_rec_list);
    for (int i = 0; i < s_file_count; i++) {
        // 使用 list 专门的添加按钮接口
        lv_obj_t * btn = lv_list_add_btn(s_rec_list, LV_SYMBOL_AUDIO, s_rec_files[i]);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xCCCCCC), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 5, 0);
       
    }
}

/* ════════════════════════════════════════
    4. 页面打开入口
   ════════════════════════════════════════ */
void page_record_open(void) {
    // 释放菜单占用的内存
    Menu_Deinit();

    menu_clear_statusbar();
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x050505), 0);

    /* --- 返回按钮 --- */
    lv_obj_t *back = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back, 36, 36);
    lv_obj_set_pos(back, 5, 5);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    /* --- 计时器显示 --- */
    s_time_lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(s_time_lbl, "00:00");
    lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_14, 0); 
    lv_obj_align(s_time_lbl, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_text_color(s_time_lbl, lv_color_hex(0xFFFFFF), 0);

    /* --- 录音主按钮 --- */
    lv_obj_t *rec_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(rec_btn, 130, 45);
    lv_obj_align(rec_btn, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(rec_btn, lv_color_hex(0x1DB954), 0);
    lv_obj_set_style_radius(rec_btn, 22, 0);
    lv_obj_set_style_shadow_width(rec_btn, 0, 0);
    
    s_rec_btn_lbl = lv_label_create(rec_btn);
    lv_label_set_text(s_rec_btn_lbl, LV_SYMBOL_PLAY " START");
    lv_obj_center(s_rec_btn_lbl);
    lv_obj_add_event_cb(rec_btn, toggle_record_cb, LV_EVENT_CLICKED, NULL);

    /* --- 列表标题 --- */
    lv_obj_t * list_title = lv_label_create(lv_scr_act());
    lv_label_set_text(list_title, "Recent Records");
    lv_obj_align(list_title, LV_ALIGN_TOP_LEFT, 15, 155);
    lv_obj_set_style_text_color(list_title, lv_color_hex(0x44FF88), 0);

    /* --- 录音列表 --- */
    s_rec_list = lv_list_create(lv_scr_act());
    lv_obj_set_size(s_rec_list, 210, 110);
    lv_obj_align(s_rec_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(s_rec_list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(s_rec_list, 0, 0);
    lv_obj_set_style_pad_all(s_rec_list, 5, 0);
    lv_obj_set_style_pad_row(s_rec_list, 8, 0);
		
    refresh_rec_list_ui();
}

/* ════════════════════════════════════════
    5. 外部更新接口
   ════════════════════════════════════════ */
void page_record_update(Record_Data_t *data) {
    if (!s_time_lbl || !data) return;

    // 仅在录音时更新时间
    if (s_is_recording) {
        char buf[16];
        uint32_t m = data->duration_sec / 60;
        uint32_t s = data->duration_sec % 60;
        snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned int)m, (unsigned int)s);
        lv_label_set_text(s_time_lbl, buf);
    }
}