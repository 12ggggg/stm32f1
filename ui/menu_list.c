#include "menu_list.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

/* --- 页面头文件 --- */
#include "page_music.h" 
#include "page_gps.h"
#include "page_weather.h"
#include "page_sd.h"
#include "app_data.h"
#include "page_recoder.h"

/* --- 动态内存变量 --- */
static MenuItem_t *current_menu_head = NULL; // 动态链表头
static MenuItem_t *active_root       = NULL; // 当前显示的根节点

/* --- 状态栏指针 --- */
static lv_obj_t *s_status_bar_cont = NULL;
static lv_obj_t *s_home_temp_lbl, *s_home_humi_lbl, *s_home_sat_lbl;
static lv_obj_t *s_home_gps_dot, *s_home_wifi_dot, *s_home_wifi_lbl, *s_home_time_lbl;

/* --- 交互状态 --- */
static bool    s_touching   = false;
static bool    s_dragged    = false;
static int32_t s_drag_accum = 0;
static int32_t s_last_x     = 0;

/* ── 内存管理：释放所有动态节点 ── */
void Menu_Deinit(void) {
    MenuItem_t *p = current_menu_head;
    while (p) {
        MenuItem_t *next = p->next;
        lv_mem_free(p); // 关键：释放堆内存
        p = next;
    }
    current_menu_head = NULL;
    active_root = NULL;
}

/* ── 内存管理：创建动态节点 ── */
MenuItem_t *menu_create_item(const char *name, const char *icon, void (*action)(void), MenuItem_t *back) {
    MenuItem_t *item = (MenuItem_t *)lv_mem_alloc(sizeof(MenuItem_t));
    if (!item) return NULL;
    memset(item, 0, sizeof(MenuItem_t));
    strncpy(item->name, name, 15);
    if (icon) strncpy(item->icon, icon, 7);
    item->action = action;
    item->back   = back;
    return item;
}

/* ── 内部工具：UI 刷新与坐标计算 ── */
static int32_t wrap(int32_t x) {
    int32_t range = 5 * MENU_GAP; // 假设5个展示位循环
    while (x >  range / 2) x -= range;
    while (x < -range / 2) x += range;
    return x;
}

static void apply_pos(MenuItem_t *item) {
    if (!item || !item->lv_obj || !item->name_obj) return;
    lv_obj_set_pos(item->lv_obj, MENU_CX + item->x - MENU_ICON_W / 2, MENU_ICON_Y - MENU_ICON_H / 2);
    lv_obj_set_pos(item->name_obj, MENU_CX + item->x - 40, MENU_ICON_Y + MENU_ICON_H / 2 + 10);
    
    int32_t ax  = (item->x < 0) ? -item->x : item->x;
    int32_t opa = 255 - (ax * 255 / MENU_GAP);
    lv_obj_set_style_opa(item->name_obj, (lv_opa_t)LV_CLAMP(0, opa, 255), 0);
}

static MenuItem_t *find_nearest(void) {
    MenuItem_t *closest = NULL;
    int32_t min_dist = 99999;
    for (MenuItem_t *p = active_root; p; p = p->next) {
        int32_t d = (p->x < 0) ? -p->x : p->x;
        if (d < min_dist) { min_dist = d; closest = p; }
    }
    return closest;
}

static void snap_offset(int32_t offset) {
    for (MenuItem_t *p = active_root; p; p = p->next) {
        p->x = wrap(p->x + offset);
        apply_pos(p);
    }
}

/* ── 事件回调 ── */
static void cb_pressing(lv_event_t *e) {
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (!s_touching) {
        s_touching = true; s_dragged = false; s_drag_accum = 0; s_last_x = pt.x;
        return;
    }

    int32_t dx = pt.x - s_last_x;
    s_last_x = pt.x;
    s_drag_accum += (dx < 0 ? -dx : dx);
    if (s_drag_accum > 10) s_dragged = true;
    if (!s_dragged) return;

    for (MenuItem_t *p = active_root; p; p = p->next) {
        p->x = wrap(p->x + dx);
        apply_pos(p);
    }
}

static void cb_released(lv_event_t *e) {
    s_touching = false;
    if (s_dragged) {
        MenuItem_t *nearest = find_nearest();
        if (nearest) snap_offset(-nearest->x);
    }
}

static void cb_clicked(lv_event_t *e) {
    if (s_dragged) return;
    MenuItem_t *p = (MenuItem_t *)lv_event_get_user_data(e);
    if (!p) return;
    if ((p->x < 0 ? -p->x : p->x) <= 20) {
        if (p->action) p->action(); // 此处子页面会调用 Menu_Deinit
    } else {
        snap_offset(-p->x);
    }
}

static void Statusbar_Init(void) {
    if (s_status_bar_cont) {
        lv_obj_clear_flag(s_status_bar_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_status_bar_cont);
        return;
    }

    // 1. 创建容器 (Top 层独立存在)
    s_status_bar_cont = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_status_bar_cont, 240, 25);
    lv_obj_set_pos(s_status_bar_cont, 0, 0);
    
    // 2. 样式：黑底白字，取消边框
    lv_obj_set_style_bg_opa(s_status_bar_cont, LV_OPA_COVER, 0); 
    lv_obj_set_style_bg_color(s_status_bar_cont, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(s_status_bar_cont, 0, 0);
    lv_obj_set_style_pad_hor(s_status_bar_cont, 8, 0);
    lv_obj_set_style_radius(s_status_bar_cont, 0, 0);
    lv_obj_clear_flag(s_status_bar_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 3. 【左侧】WiFi 和 GPS
    // WiFi 图标
    s_home_wifi_lbl = lv_label_create(s_status_bar_cont);
    lv_label_set_text(s_home_wifi_lbl, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_home_wifi_lbl, lv_color_hex(0x00FF00), 0); // 绿色表示连接
    lv_obj_align(s_home_wifi_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    // GPS 图标 (偏移一点防止重叠)
    s_home_sat_lbl = lv_label_create(s_status_bar_cont); 
    lv_label_set_text(s_home_sat_lbl, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(s_home_sat_lbl, lv_color_hex(0xFFCC00), 0); // 橙色
    lv_obj_align(s_home_sat_lbl, LV_ALIGN_LEFT_MID, 25, 0);

    // 4. 【中间】时间
    s_home_time_lbl = lv_label_create(s_status_bar_cont);
    lv_label_set_text(s_home_time_lbl, "12:00");
    lv_obj_set_style_text_font(s_home_time_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_home_time_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_home_time_lbl, LV_ALIGN_CENTER, 0, 0);

    // 5. 【右侧】SD 卡状态
    s_home_temp_lbl = lv_label_create(s_status_bar_cont); // 复用 temp 指针作为 SD 卡显示
    lv_label_set_text(s_home_temp_lbl, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(s_home_temp_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_home_temp_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
}

/* ── 渲染逻辑 ── */
void menu_render(MenuItem_t *root) {
    if (!root) return;
    active_root = root;
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), 0);
    
    Statusbar_Init();

    int32_t n = 0;
    for (MenuItem_t *tmp = root; tmp; tmp = tmp->next) n++;

    int32_t i = 0;
    for (MenuItem_t *p = root; p != NULL; p = p->next) {
        p->x = (i * MENU_GAP) - ((n - 1) * MENU_GAP / 2);
        
        p->lv_obj = lv_btn_create(lv_scr_act());
        lv_obj_set_size(p->lv_obj, MENU_ICON_W, MENU_ICON_H);
        lv_obj_set_style_bg_opa(p->lv_obj, 0, 0);
        lv_obj_set_style_border_width(p->lv_obj, 0, 0);
        lv_obj_set_style_shadow_width(p->lv_obj, 0, 0);

        lv_obj_add_event_cb(p->lv_obj, cb_clicked, LV_EVENT_CLICKED, p);
        lv_obj_add_event_cb(p->lv_obj, cb_pressing, LV_EVENT_PRESSING, NULL);
        lv_obj_add_event_cb(p->lv_obj, cb_released, LV_EVENT_RELEASED, NULL);

        lv_obj_t *icon_lbl = lv_label_create(p->lv_obj);
        lv_label_set_text(icon_lbl, p->icon[0] ? p->icon : p->name);
        lv_obj_center(icon_lbl);

        p->name_obj = lv_label_create(lv_scr_act());
        lv_label_set_text(p->name_obj, p->name);
        
        apply_pos(p);
        i++;
    }
    lv_obj_add_event_cb(lv_scr_act(), cb_pressing, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(lv_scr_act(), cb_released, LV_EVENT_RELEASED, NULL);
}

void Menu_Init(void) {
    Statusbar_Init();
    // 动态分配主菜单
    MenuItem_t *m1 = menu_create_item("gps",     LV_SYMBOL_GPS,     page_gps_open, NULL);
    MenuItem_t *m2 = menu_create_item("music",   LV_SYMBOL_AUDIO,   page_music_open, NULL);
    MenuItem_t *m3 = menu_create_item("weather", LV_SYMBOL_WIFI,    page_weather_open, NULL);
    MenuItem_t *m4 = menu_create_item("sd",      LV_SYMBOL_SD_CARD, page_sd_open, NULL);
    MenuItem_t *m5 = menu_create_item("record",  LV_SYMBOL_SETTINGS, page_record_open, NULL);
    
    m1->next = m2; m2->next = m3; m3->next = m4; m4->next = m5; m5->next = NULL;
    current_menu_head = m1;
    menu_render(m1);
}

void menu_go_back(void) {
    Menu_Init(); // 返回时重新分配
}
// 销毁状态栏，回收所有 RAM
void menu_clear_statusbar(void) {
    if (s_status_bar_cont) {
        lv_obj_del(s_status_bar_cont);
        s_status_bar_cont = NULL;
        s_home_time_lbl = NULL; // 必须置空，防止后续调用 API 崩溃
    }
}