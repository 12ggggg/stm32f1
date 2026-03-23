#include "page_gps.h"
#include "menu_list.h"
#include <stdio.h>
#include"app_data.h"

/* ── 控件指针 (Static 以便 update 函数访问) ── */
static lv_obj_t *s_lat_lbl    = NULL;
static lv_obj_t *s_lon_lbl    = NULL;
static lv_obj_t *s_time_lbl   = NULL;
static lv_obj_t *s_gps_dot    = NULL;
static lv_obj_t *s_gps_status = NULL;
static lv_obj_t *s_sat_lbl    = NULL;
static lv_obj_t *s_refresh_btn = NULL;  // 刷新按钮

extern TaskHandle_t task_gps_Handler;

/* ── 返回回调 ── */
static void back_cb(lv_event_t *e)
{
    // 1. 指针置空，防止外部 update 任务在页面销毁后写入
    s_lat_lbl    = NULL;
    s_lon_lbl    = NULL;
    s_time_lbl   = NULL;
    s_gps_dot    = NULL;
    s_gps_status = NULL;
    s_sat_lbl    = NULL;
    s_refresh_btn = NULL;
    // 2. 重新初始化菜单 (内部会触发 malloc)
    menu_go_back();
}

/* ── 刷新按钮回调 ── */
static void refresh_cb(lv_event_t *e)
{
     if(task_gps_Handler != NULL) {
        // 向 gps 任务发送通知进行数据更新
        xTaskNotify(task_gps_Handler, GPS_EVT_UPDATE_DATA, eSetBits);  
    }
}
/* ── 内部工具：创建数据行 ── */
static lv_obj_t *create_row(lv_obj_t *parent, int y, const char *key, const char *val)
{
    lv_obj_t *key_lbl = lv_label_create(parent);
    lv_label_set_text(key_lbl, key);
    lv_obj_set_pos(key_lbl, 15, y);
    lv_obj_set_style_text_color(key_lbl, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(key_lbl, &lv_font_montserrat_14, 0);

    lv_obj_t *val_lbl = lv_label_create(parent);
    lv_label_set_text(val_lbl, val);
    lv_obj_set_pos(val_lbl, 95, y);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(val_lbl, &lv_font_montserrat_14, 0);

    return val_lbl;
}

/* ── 打开页面 ── */
void page_gps_open(void)
{
    // 【关键优化】释放菜单链表的堆内存
    Menu_Deinit();
    
    // 隐藏状态栏 (通过 menu_list 提供的接口)
    menu_clear_statusbar();

    // 清理屏幕并设为深绿/黑色调背景
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0a1a0a), 0);

    /* ── 返回按钮 ── */
    lv_obj_t *back = lv_btn_create(lv_scr_act());
    lv_obj_set_size(back, 36, 36);
    lv_obj_set_pos(back, 5, 5);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x333333), 0);
    lv_obj_set_style_radius(back, 18, 0);
    lv_obj_set_style_border_width(back, 0, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT);
    lv_obj_center(bl);
    lv_obj_set_style_text_color(bl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
     
		 /* ── 刷新按钮（右上角）── */
    s_refresh_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(s_refresh_btn, 36, 36);
    lv_obj_set_pos(s_refresh_btn, 199, 5);  // 右上角
    lv_obj_set_style_bg_color(s_refresh_btn, lv_color_hex(0x1a3a1a), 0);
    lv_obj_set_style_radius(s_refresh_btn, 18, 0);
    lv_obj_set_style_border_width(s_refresh_btn, 0, 0);
    lv_obj_set_style_shadow_width(s_refresh_btn, 0, 0);
    lv_obj_t *rl = lv_label_create(s_refresh_btn);
    lv_label_set_text(rl, LV_SYMBOL_REFRESH);
    lv_obj_center(rl);
    lv_obj_set_style_text_color(rl, lv_color_hex(0x44FF88), 0);
    lv_obj_add_event_cb(s_refresh_btn, refresh_cb, LV_EVENT_CLICKED, NULL);
    /* ── 标题 ── */
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "GPS Receiver");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

    /* ── GPS图标装饰 ── */
    lv_obj_t *icon_bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(icon_bg, 80, 80);
    lv_obj_align(icon_bg, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(icon_bg, lv_color_hex(0x1a3a1a), 0);
    lv_obj_set_style_radius(icon_bg, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(icon_bg, 2, 0);
    lv_obj_set_style_border_color(icon_bg, lv_color_hex(0x44FF88), 0);

    lv_obj_t *icon = lv_label_create(icon_bg);
    lv_label_set_text(icon, LV_SYMBOL_GPS);
    lv_obj_center(icon);
    lv_obj_set_style_text_color(icon, lv_color_hex(0x44FF88), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);

    /* ── 状态指示灯 ── */
    s_gps_dot = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_gps_dot, 10, 10);
    lv_obj_set_pos(s_gps_dot, 75, 143);
    lv_obj_set_style_radius(s_gps_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_gps_dot, lv_color_hex(0xFF4444), 0); // 默认红色（未定位）
    lv_obj_set_style_border_width(s_gps_dot, 0, 0);

    s_gps_status = lv_label_create(lv_scr_act());
    lv_label_set_text(s_gps_status, "Locating...");
    lv_obj_set_pos(s_gps_status, 95, 140);
    lv_obj_set_style_text_color(s_gps_status, lv_color_hex(0xFF4444), 0);
    lv_obj_set_style_text_font(s_gps_status, &lv_font_montserrat_14, 0);

    /* ── 分割线 ── */
    lv_obj_t *line = lv_obj_create(lv_scr_act());
    lv_obj_set_size(line, 210, 1);
    lv_obj_set_pos(line, 15, 168);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(line, 0, 0);

    /* ── 数据显示行 ── */
    s_lat_lbl  = create_row(lv_scr_act(), 185, "Latitude", "--");
    s_lon_lbl  = create_row(lv_scr_act(), 215, "Longitude", "--");
    s_time_lbl = create_row(lv_scr_act(), 245, "UTC Time", "--:--:--");

    /* ── 卫星数量显示 ── */
    s_sat_lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(s_sat_lbl, "Sats: 0/0");
    lv_obj_set_pos(s_sat_lbl, 15, 275);
    lv_obj_set_style_text_color(s_sat_lbl, lv_color_hex(0x44FF88), 0);
    lv_obj_set_style_text_font(s_sat_lbl, &lv_font_montserrat_14, 0);
		
		GPS_Data_t local;
		GPS_Data_LOCK();
		local = g_gps;
		GPS_Data_UNLOCK();
		page_gps_update(&local);
}

/* ── 数据更新 (由后台 GPS 任务调用) ── */
void page_gps_update(GPS_Data_t *data)
{
    // 如果页面指针为空，说明页面已关闭，直接退出防止奔溃
    if (s_lat_lbl == NULL || data == NULL) return;

    char buf[32];

    // 更新卫星数
    snprintf(buf, sizeof(buf), "Sats: %d/%d", data->satellites, data->visible_sats);
    lv_label_set_text(s_sat_lbl, buf);

    if (data->valid) {
        // 定位成功：绿色
        lv_obj_set_style_bg_color(s_gps_dot, lv_color_hex(0x44FF44), 0);
        lv_label_set_text(s_gps_status, "GPS");
        lv_obj_set_style_text_color(s_gps_status, lv_color_hex(0x44FF44), 0);

        snprintf(buf, sizeof(buf), "%.6f", data->lat);
        lv_label_set_text(s_lat_lbl, buf);

        snprintf(buf, sizeof(buf), "%.6f", data->lon);
        lv_label_set_text(s_lon_lbl, buf);

        lv_label_set_text(s_time_lbl, data->time);
    } 
		 else {
        // 定位失败：红色，显示默认初始数据
        lv_obj_set_style_bg_color(s_gps_dot, lv_color_hex(0xFF4444), 0);
        lv_label_set_text(s_gps_status, "Locating...");
        lv_obj_set_style_text_color(s_gps_status, lv_color_hex(0xFF4444), 0);

        lv_label_set_text(s_lat_lbl, "23.06865896");
        lv_label_set_text(s_lon_lbl, "112.55047835");
        lv_label_set_text(s_time_lbl, "22:50:10");
    }		
}