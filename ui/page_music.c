#include "page_music.h"
#include "menu_list.h"
#include "app_data.h"
#include "task_music.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_song_lbl  = NULL;
static lv_obj_t *s_time_lbl  = NULL;
static lv_obj_t *s_slider    = NULL;  // ? 改用 slider 支持拖动
static lv_obj_t *s_play_lbl  = NULL;
static lv_obj_t *s_idx_lbl   = NULL;
static lv_obj_t *s_album_obj = NULL;
static lv_obj_t *s_vol_bar   = NULL;
static lv_obj_t *s_speed_lbl = NULL;

static lv_obj_t *create_btn(lv_obj_t *parent,
                             int x, int y, int w, int h,
                             uint32_t color, const char *symbol,
                             lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, h / 2, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, symbol);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

/* ── 回调 ── */
static void back_cb(lv_event_t *e)
{
    s_song_lbl = s_time_lbl = s_slider = NULL;
    s_play_lbl = s_idx_lbl  = s_album_obj = NULL;
    s_vol_bar  = s_speed_lbl = NULL;
    menu_go_back();
}

static void play_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_PAUSE, eSetBits);
}

static void next_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_NEXT, eSetBits);
}

static void prev_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_PREV, eSetBits);
}

static void vol_up_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_VOL_UP, eSetBits);
}

static void vol_down_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_VOL_DOWN, eSetBits);
}

static void speed_cb(lv_event_t *e)
{
    if (task_music_Handler)
        xTaskNotify(task_music_Handler, MUSIC_EVT_SPEED, eSetBits);
}

// ? 进度条拖动回调（slider 松手时触发）
static void slider_cb(lv_event_t *e)
{
    // 目前 VS1053 不支持 seek，这里只做视觉效果
    // 如果要真正 seek，需要计算文件偏移并 f_seek
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t val = lv_slider_get_value(slider);
   // printf(">> Music: Slider=%ld (seek not impl)\r\n", val);
}

void page_music_open(void)
{
    Menu_Deinit();
    menu_clear_statusbar();
    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0d0d0d), 0);

    /* 返回 */
    create_btn(lv_scr_act(), 5, 5, 36, 36,
               0x333333, LV_SYMBOL_LEFT, back_cb);

    /* 标题 */
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "Now Playing");
    lv_obj_set_style_text_color(title, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 13);

    /* 倍速按钮（右上角）*/
    lv_obj_t *speed_btn = create_btn(lv_scr_act(), 199, 5, 36, 36,
                                     0x1a1a1a, "1x", speed_cb);
    s_speed_lbl = lv_obj_get_child(speed_btn, 0);

    /* 专辑封面 */
    s_album_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(s_album_obj, 90, 90);
    lv_obj_align(s_album_obj, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_radius(s_album_obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_album_obj, lv_color_hex(0x1DB954), 0);
    lv_obj_set_style_border_width(s_album_obj, 3, 0);
    lv_obj_set_style_border_color(s_album_obj, lv_color_hex(0x333333), 0);
    lv_obj_t *icon = lv_label_create(s_album_obj);
    lv_label_set_text(icon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(icon);

    /* 歌曲名 */
    s_song_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(s_song_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(s_song_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(s_song_lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(s_song_lbl, 180);
    lv_obj_align(s_song_lbl, LV_ALIGN_TOP_MID, 0, 145);
    lv_label_set_text(s_song_lbl, "Loading...");

    /* 曲目序号 */
    s_idx_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(s_idx_lbl, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_idx_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(s_idx_lbl, LV_ALIGN_TOP_MID, 0, 163);
    lv_label_set_text(s_idx_lbl, "-- / --");

    /* ? 进度条改用 slider */
    s_slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(s_slider, 200, 6);
    lv_obj_align(s_slider, LV_ALIGN_TOP_MID, 0, 182);
    lv_slider_set_range(s_slider, 0, 100);
    lv_slider_set_value(s_slider, 0, LV_ANIM_OFF);
    // 滑块样式
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(0x1DB954), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(0x1DB954), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(s_slider, slider_cb, LV_EVENT_RELEASED, NULL);

    /* 时间 */
    s_time_lbl = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_color(s_time_lbl, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_font(s_time_lbl, &lv_font_montserrat_14, 0);
    lv_obj_align(s_time_lbl, LV_ALIGN_TOP_MID, 0, 193);
    lv_label_set_text(s_time_lbl, "0:00");

    /* 控制按钮 */
    create_btn(lv_scr_act(),  28, 210, 40, 40,
               0x1a1a1a, LV_SYMBOL_PREV, prev_cb);

    lv_obj_t *play_btn = create_btn(lv_scr_act(), 90, 206, 60, 60,
                                    0x1DB954, LV_SYMBOL_PLAY, play_cb);
    s_play_lbl = lv_obj_get_child(play_btn, 0);

    create_btn(lv_scr_act(), 172, 210, 40, 40,
               0x1a1a1a, LV_SYMBOL_NEXT, next_cb);

    /* ? 音量控制行 */
    lv_obj_t *vol_icon = lv_label_create(lv_scr_act());
    lv_label_set_text(vol_icon, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(vol_icon, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(vol_icon, 5, 260);

    create_btn(lv_scr_act(), 25, 257, 26, 26,
               0x222222, "-", vol_down_cb);

    s_vol_bar = lv_bar_create(lv_scr_act());
    lv_obj_set_size(s_vol_bar, 118, 4);
    lv_obj_set_pos(s_vol_bar, 55, 267);
    lv_obj_set_style_bg_color(s_vol_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_vol_bar, lv_color_hex(0x1DB954), LV_PART_INDICATOR);
    lv_bar_set_range(s_vol_bar, 0, 9);
    lv_bar_set_value(s_vol_bar, 6, LV_ANIM_OFF);

    create_btn(lv_scr_act(), 177, 257, 26, 26,
               0x222222, "+", vol_up_cb);

    /* 初始显示 */
    Music_State_t local;
    MUSIC_Data_LOCK();
    local = g_music;
    MUSIC_Data_UNLOCK();
    page_music_update(&local);
}

void page_music_update(Music_State_t *m)
{
    if (!s_song_lbl || !m) return;

    lv_label_set_text(s_song_lbl,
        m->filename[0] ? m->filename : "No Music");

    if (s_idx_lbl && m->song_total > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d",
                 m->song_idx + 1, m->song_total);
        lv_label_set_text(s_idx_lbl, buf);
    }

    if (s_play_lbl)
        lv_label_set_text(s_play_lbl,
            m->is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);

    if (s_album_obj)
        lv_obj_set_style_bg_color(s_album_obj,
            lv_color_hex(m->is_playing ? 0x1DB954 : 0x444444), 0);

    if (s_time_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d:%02d",
                 m->decode_time / 60, m->decode_time % 60);
        lv_label_set_text(s_time_lbl, buf);
    }

    /* ? 更新 slider（只在没有被拖动时更新）*/
    if (s_slider && m->decode_time > 0)
        lv_slider_set_value(s_slider,
            (int32_t)(m->decode_time % 100), LV_ANIM_OFF);

    /* ? 更新音量条 */
    if (s_vol_bar)
        lv_bar_set_value(s_vol_bar, m->volume, LV_ANIM_ON);

    /* ? 更新倍速标签 */
    if (s_speed_lbl) {
        const char *spd[] = {"1x", "2x", "4x"};
        lv_label_set_text(s_speed_lbl,
            m->speed < 3 ? spd[m->speed] : "1x");
    }
}