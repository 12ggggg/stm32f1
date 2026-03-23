#include <stdio.h>
#include <string.h>
#include "page_weather.h"
#include "menu_list.h"
#include "app_data.h"

/* --- 外部任务和数据 --- */
extern TaskHandle_t task_wifi_Handler;
extern Weather_Data_t g_weather; // WiFi 任务更新的全局实时天气数据


/* --- UI 控件指针 --- */
static lv_obj_t *s_city_lbl = NULL;
static lv_obj_t *s_desc_lbl = NULL;
static lv_obj_t *s_temp_lbl = NULL;
static lv_obj_t *s_dot0     = NULL;
static lv_obj_t *s_dot1     = NULL;
static lv_obj_t *s_dot2     = NULL;

/* --- 内部私有函数声明 --- */
static void request_wifi_weather_update(void);
static void prev_city_cb(lv_event_t *e);
static void next_city_cb(lv_event_t *e);
static void refresh_btn_cb(lv_event_t *e);
static void back_cb(lv_event_t *e);

/* ── 刷新指示点状态 ── */
static void update_dots(void)
{
    lv_obj_t *dots[3] = {s_dot0, s_dot1, s_dot2};
    for (int i = 0; i < 3; i++) {
        if (!dots[i]) continue;
        lv_obj_set_style_bg_color(dots[i],
            i == s_city_idx ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x555555), 0);
    }
}

void page_weather_update(Weather_Data_t *d)
{
    if (!s_city_lbl || !s_desc_lbl || !s_temp_lbl || !d) return;

    char buf[16];
    lv_label_set_text(s_city_lbl, d->city);
    lv_label_set_text(s_desc_lbl, d->desc);
    snprintf(buf, sizeof(buf), "%d°C", d->temp_cur);
    lv_label_set_text(s_temp_lbl, buf);

    update_dots();
}

/* ── 返回回调 ── */
static void back_cb(lv_event_t *e)
{
    // 1. 清空指针，防止 WiFi 任务回调时操作无效内存
    s_city_lbl = s_desc_lbl = s_temp_lbl = NULL;
    s_dot0 = s_dot1 = s_dot2 = NULL;

    // 2. 重新初始化菜单 (内部 malloc)
    menu_go_back();
}

/* ── 内部工具：创建小圆点 ── */
static lv_obj_t *create_dot(lv_obj_t *parent, int x, int y, uint32_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_pos(dot, x, y);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    return dot;
}

/* ── 内部工具：创建圆形按钮 ── */
static lv_obj_t *create_round_btn(lv_obj_t *parent, int x, int y, int w, int h,
                                 uint32_t color, const char *symbol, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* ── 页面打开函数 ── */
void page_weather_open(void)
{
    // 【内存优化】释放菜单链表 RAM
    Menu_Deinit();

    // 清理状态栏和屏幕
    menu_clear_statusbar();
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0a1628), 0);

    /* ── 顶部控制栏 ── */
    // 返回按钮
    create_round_btn(lv_scr_act(), 5, 5, 36, 36, 0x333333, LV_SYMBOL_LEFT, back_cb);
    // 刷新按钮
    create_round_btn(lv_scr_act(), 240 - 36 - 5, 5, 36, 36, 0x333333, LV_SYMBOL_REFRESH, refresh_btn_cb);

    /* ── 城市切换 ── */
    create_round_btn(lv_scr_act(), 48, 8, 30, 30, 0x1a2a3a, LV_SYMBOL_LEFT, prev_city_cb);

    s_city_lbl = lv_label_create(lv_scr_act());
    lv_obj_align(s_city_lbl, LV_ALIGN_TOP_MID, 0, 14);
    lv_obj_set_style_text_color(s_city_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_city_lbl, &lv_font_montserrat_14, 0);

    create_round_btn(lv_scr_act(), 160, 8, 30, 30, 0x1a2a3a, LV_SYMBOL_RIGHT, next_city_cb);

    /* ── 城市指示点 ── */
    s_dot0 = create_dot(lv_scr_act(), 101, 44, 0xFFFFFF);
    s_dot1 = create_dot(lv_scr_act(), 115, 44, 0x555555);
    s_dot2 = create_dot(lv_scr_act(), 129, 44, 0x555555);

    /* ── 大温度显示 ── */
    s_temp_lbl = lv_label_create(lv_scr_act());
    lv_obj_align(s_temp_lbl, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(s_temp_lbl, lv_color_hex(0xFFFFFF), 0);
    // 注意：如果想要酷炫效果，建议在 lv_conf.h 开启 montserrat_48 字体
    lv_obj_set_style_text_font(s_temp_lbl, &lv_font_montserrat_14, 0); 

    /* ── 天气描述 ── */
    s_desc_lbl = lv_label_create(lv_scr_act());
    lv_obj_align_to(s_desc_lbl, s_temp_lbl, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_text_color(s_desc_lbl, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(s_desc_lbl, &lv_font_montserrat_14, 0);

    /* ── 初次加载数据 ── */
    Weather_Data_t local;
		WEATHER_Data_LOCK();
		local = city_data[s_city_idx];
		WEATHER_Data_UNLOCK();
		page_weather_update(&local);
}

/* ── 业务逻辑回调 ── */
static void refresh_btn_cb(lv_event_t *e)
{
	 if(s_desc_lbl) lv_label_set_text(s_desc_lbl, "Loading...");
    request_wifi_weather_update();
}

static void request_wifi_weather_update(void)
{
    if(task_wifi_Handler != NULL) {
        // 向 WiFi 任务发送通知进行数据抓取
        xTaskNotify(task_wifi_Handler, WIFI_EVT_FETCH_WEATHER, eSetBits);  
    }
}

static void prev_city_cb(lv_event_t *e)
{
    s_city_idx = (s_city_idx + CITY_COUNT - 1) % CITY_COUNT;
    Weather_Data_t local;
    WEATHER_Data_LOCK();
    local = city_data[s_city_idx];
    WEATHER_Data_UNLOCK();
    page_weather_update(&local);

    request_wifi_weather_update();
}

static void next_city_cb(lv_event_t *e)
{
    s_city_idx = (s_city_idx + 1) % CITY_COUNT;
    Weather_Data_t local;
    WEATHER_Data_LOCK();
    local = city_data[s_city_idx];
    WEATHER_Data_UNLOCK();
    page_weather_update(&local);
    request_wifi_weather_update();
}