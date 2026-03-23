#include "page_sd.h"
#include "menu_list.h"
#include "app_data.h"
#include "task_sd.h"
#include <stdio.h>
#include <string.h>

#define MAX_FILES 8

/* ── 内部数据 ── */
static char    s_files[MAX_FILES][32];
static uint8_t s_file_count   = 0;
static int8_t  s_selected_idx = -1;

/* ── UI 控件指针 ── */
static lv_obj_t *s_status_dot  = NULL;
static lv_obj_t *s_status_lbl  = NULL;
static lv_obj_t *s_total_lbl   = NULL;
static lv_obj_t *s_bar         = NULL;
static lv_obj_t *s_file_list   = NULL;
static lv_obj_t *s_del_btn     = NULL;
static lv_obj_t *s_confirm_box = NULL;

/* ── 前向声明 ── */
static void refresh_file_list_ui(void);

/* ════════════════════════════════════════
   返回
   ════════════════════════════════════════ */
static void back_cb(lv_event_t *e)
{
    s_status_dot = s_status_lbl = s_total_lbl = NULL;
    s_bar = s_file_list = s_del_btn = s_confirm_box = NULL;
    s_selected_idx = -1;
    menu_go_back();
}

/* ════════════════════════════════════════
   刷新按钮
   ════════════════════════════════════════ */
static void refresh_btn_cb(lv_event_t *e)
{
    if (s_status_lbl) {
        lv_label_set_text(s_status_lbl, "Scanning...");
        lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(0xFFCC44), 0);
    }
    if (task_sd_Handler)
        xTaskNotify(task_sd_Handler, SD_EVT_REFRESH, eSetBits);
}

/* ════════════════════════════════════════
   删除逻辑
   ════════════════════════════════════════ */
static void confirm_del_cb(lv_event_t *e)
{
    if (s_confirm_box) { lv_obj_del(s_confirm_box); s_confirm_box = NULL; }
    if (s_selected_idx < 0) return;

    // ? 把文件名传给 task_sd，让任务层删除
    taskENTER_CRITICAL();
    strncpy(g_sd_delete_name, s_files[s_selected_idx], 31);
    g_sd_delete_name[31] = '\0';
    taskEXIT_CRITICAL();

    s_selected_idx = -1;
    if (s_del_btn) lv_obj_add_flag(s_del_btn, LV_OBJ_FLAG_HIDDEN);

    // ? 通知 task_sd 去删除
    if (task_sd_Handler)
        xTaskNotify(task_sd_Handler, SD_EVT_DELETE, eSetBits);
}

static void cancel_del_cb(lv_event_t *e)
{
    if (s_confirm_box) { lv_obj_del(s_confirm_box); s_confirm_box = NULL; }
}

static void show_confirm_dialog(void)
{
    if (s_selected_idx < 0 || s_confirm_box) return;

    s_confirm_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_confirm_box, 200, 110);
    lv_obj_center(s_confirm_box);
    lv_obj_set_style_bg_color(s_confirm_box, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(s_confirm_box, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_border_width(s_confirm_box, 2, 0);

    lv_obj_t *msg = lv_label_create(s_confirm_box);
    lv_label_set_text_fmt(msg, "Delete?\n%s", s_files[s_selected_idx]);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_text_color(msg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *btn_c = lv_btn_create(s_confirm_box);
    lv_obj_set_size(btn_c, 70, 32);
    lv_obj_align(btn_c, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(0x444444), 0);
    lv_obj_t *lc = lv_label_create(btn_c);
    lv_label_set_text(lc, "No");
    lv_obj_center(lc);
    lv_obj_add_event_cb(btn_c, cancel_del_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_o = lv_btn_create(s_confirm_box);
    lv_obj_set_size(btn_o, 70, 32);
    lv_obj_align(btn_o, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btn_o, lv_color_hex(0xFF4444), 0);
    lv_obj_t *lo = lv_label_create(btn_o);
    lv_label_set_text(lo, "Yes");
    lv_obj_center(lo);
    lv_obj_add_event_cb(btn_o, confirm_del_cb, LV_EVENT_CLICKED, NULL);
}

/* ════════════════════════════════════════
   文件列表
   ════════════════════════════════════════ */
static void file_row_cb(lv_event_t *e)
{
    int8_t idx = (int8_t)(intptr_t)lv_event_get_user_data(e);
    if (s_selected_idx == idx) {
        s_selected_idx = -1;
        if (s_del_btn) lv_obj_add_flag(s_del_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        s_selected_idx = idx;
        if (s_del_btn) lv_obj_clear_flag(s_del_btn, LV_OBJ_FLAG_HIDDEN);
    }
    refresh_file_list_ui();
}

static void del_btn_click_cb(lv_event_t *e)
{
    show_confirm_dialog();
}

static void refresh_file_list_ui(void)
{
    if (!s_file_list) return;
    lv_obj_clean(s_file_list);

    for (int i = 0; i < s_file_count; i++) {
        lv_obj_t *row = lv_obj_create(s_file_list);
        lv_obj_set_size(row, lv_pct(100), 32);
        lv_obj_set_pos(row, 0, i * 36);
        lv_obj_set_style_radius(row, 4, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        if (i == s_selected_idx) {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x2a4a2a), 0);
            lv_obj_set_style_border_width(row, 1, 0);
            lv_obj_set_style_border_color(row, lv_color_hex(0x44FF88), 0);
        } else {
            lv_obj_set_style_bg_color(row, lv_color_hex(0x1a1a1a), 0);
            lv_obj_set_style_border_width(row, 0, 0);
        }

        lv_obj_t *icon = lv_label_create(row);
        lv_label_set_text(icon, LV_SYMBOL_FILE);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_text_color(icon, lv_color_hex(0x44FF88), 0);

        lv_obj_t *name = lv_label_create(row);
        lv_label_set_text(name, s_files[i]);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 25, 0);
        lv_obj_set_style_text_color(name, lv_color_hex(0xEEEEEE), 0);

        lv_obj_add_event_cb(row, file_row_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);
    }
}

/* ════════════════════════════════════════
   页面打开
   ════════════════════════════════════════ */
void page_sd_open(void)
{
    Menu_Deinit();
    menu_clear_statusbar();
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x050a05), 0);

    /* 返回按钮 */
    lv_obj_t *back = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back, 36, 36);
    lv_obj_set_pos(back, 5, 5);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_center(bl);
    lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    /* 标题 */
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Storage");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    /* 刷新按钮 */
    lv_obj_t *ref = lv_btn_create(lv_scr_act());
    lv_obj_set_size(ref, 36, 36);
    lv_obj_set_pos(ref, 199, 5);
    lv_obj_set_style_bg_color(ref, lv_color_hex(0x1a3a1a), 0);
    lv_obj_set_style_radius(ref, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(ref, 0, 0);
    lv_obj_t *rl = lv_label_create(ref);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH);
    lv_obj_center(rl);
    lv_obj_set_style_text_color(rl, lv_color_hex(0x44FF88), 0);
    lv_obj_add_event_cb(ref, refresh_btn_cb, LV_EVENT_CLICKED, NULL);

    /* 状态指示灯 */
    s_status_dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_status_dot, 8, 8);
    lv_obj_set_pos(s_status_dot, 15, 52);
    lv_obj_set_style_radius(s_status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_status_dot, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_border_width(s_status_dot, 0, 0);

    s_status_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_pos(s_status_lbl, 28, 48);
    lv_obj_set_style_text_font(s_status_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(0xFF4444), 0);
    lv_label_set_text(s_status_lbl, "Checking...");

    /* 容量显示 */
    s_total_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_pos(s_total_lbl, 15, 70);
    lv_obj_set_style_text_color(s_total_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(s_total_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_total_lbl, "-- MB / -- MB");

    /* 进度条 */
    s_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(s_bar, 210, 6);
    lv_obj_set_pos(s_bar, 15, 95);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x44FF88), LV_PART_INDICATOR);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);

    /* 文件列表容器 */
    s_file_list = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_file_list, 220, 155);
    lv_obj_set_pos(s_file_list, 10, 110);
    lv_obj_set_style_bg_opa(s_file_list, 0, 0);
    lv_obj_set_style_border_width(s_file_list, 0, 0);
    lv_obj_set_style_pad_all(s_file_list, 0, 0);
    lv_obj_set_scrollbar_mode(s_file_list, LV_SCROLLBAR_MODE_AUTO);

    /* 删除按钮 */
    s_del_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(s_del_btn, 40, 40);
    lv_obj_align(s_del_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(s_del_btn, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_radius(s_del_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(s_del_btn, 0, 0);
    lv_obj_t *dl = lv_label_create(s_del_btn);
    lv_label_set_text(dl, LV_SYMBOL_TRASH);
    lv_obj_center(dl);
    lv_obj_add_event_cb(s_del_btn, del_btn_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_del_btn, LV_OBJ_FLAG_HIDDEN);

    refresh_file_list_ui();

    /* 打开时自动触发一次刷新 */
    if (task_sd_Handler)
        xTaskNotify(task_sd_Handler, SD_EVT_REFRESH, eSetBits);
}

/* ════════════════════════════════════════
   外部更新接口
   ════════════════════════════════════════ */
void page_sd_update(SD_Data_t *data)
{
    if (!s_status_lbl || !s_status_dot ||
        !s_total_lbl  || !s_bar || !data) return;

    if (data->mounted) {
        lv_obj_set_style_bg_color(s_status_dot, lv_color_hex(0x44FF44), 0);
        lv_label_set_text(s_status_lbl, "SD Ready");
        lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(0x44FF44), 0);
        lv_label_set_text_fmt(s_total_lbl, "%luMB / %luMB",
                              data->used_mb, data->total_mb);
        if (data->total_mb > 0)
            lv_bar_set_value(s_bar,
                (int32_t)(data->used_mb * 100 / data->total_mb),
                LV_ANIM_ON);
    } else {
        lv_obj_set_style_bg_color(s_status_dot, lv_color_hex(0xFF4444), 0);
        lv_label_set_text(s_status_lbl, "No Card");
        lv_obj_set_style_text_color(s_status_lbl, lv_color_hex(0xFF4444), 0);
        lv_label_set_text(s_total_lbl, "-- MB / -- MB");
        lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    }
}

void page_sd_update_files(char files[][32], uint8_t count)
{
    s_file_count = (count > MAX_FILES) ? MAX_FILES : count;
    for (uint8_t i = 0; i < s_file_count; i++) {
        strncpy(s_files[i], files[i], 31);
        s_files[i][31] = '\0';
    }
    refresh_file_list_ui();
}