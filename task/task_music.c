#include "task_music.h"
#include "app_data.h"
#include "VS1053.h"
#include "ff.h"
#include "task_sd.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

TaskHandle_t task_music_Handler = NULL;

static uint8_t s_paused      = 0;
static uint8_t s_skip        = 0;
static uint8_t s_cur_idx_g   = 0;  // ? 全局cur_idx，避免指针传递混乱

#define MAX_MUSIC 8
static char    s_music_list[MAX_MUSIC][32];
static uint8_t s_music_count = 0;

static uint8_t is_music_file(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    if (ext[1]=='m'||ext[1]=='M')
        if (ext[2]=='p'||ext[2]=='P')
            if (ext[3]=='3') return 1;
    if (ext[1]=='w'||ext[1]=='W')
        if (ext[2]=='a'||ext[2]=='A')
            if (ext[3]=='v'||ext[3]=='V') return 1;
    return 0;
}

static void build_music_list(void)
{
    s_music_count = 0;
    for (uint8_t i = 0;
         i < s_file_count && s_music_count < MAX_MUSIC; i++)
    {
        if (is_music_file(s_file_names[i]))
        {
            strncpy(s_music_list[s_music_count], s_file_names[i], 31);
            s_music_list[s_music_count][31] = '\0';
            s_music_count++;
        }
    }
    printf(">> Music: found %d files\r\n", s_music_count);
}

/* ? 处理通知，带可选等待时间 */
static void process_notify(TickType_t wait_ticks)
{
    uint32_t val = 0;
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &val, wait_ticks) != pdTRUE)
        return;

    if (val & MUSIC_EVT_PAUSE)
    {
        s_paused = !s_paused;
        taskENTER_CRITICAL();
        g_music.is_playing = !s_paused;
        taskEXIT_CRITICAL();
        UI_NOTIFY(UI_EVT_MUSIC);
        printf(">> Music: %s\r\n", s_paused ? "Paused" : "Resumed");
    }
    if (val & MUSIC_EVT_NEXT)
    {
        s_skip       = 1;
        s_paused     = 0;  // ? 切歌时强制取消暂停
        s_cur_idx_g  = (s_cur_idx_g + 1) % s_music_count;
        printf(">> Music: Next -> %d\r\n", s_cur_idx_g);
    }
    if (val & MUSIC_EVT_PREV)
    {
        s_skip       = 1;
        s_paused     = 0;  // ? 切歌时强制取消暂停
        s_cur_idx_g  = (s_cur_idx_g + s_music_count - 1)
                       % s_music_count;
        printf(">> Music: Prev -> %d\r\n", s_cur_idx_g);
    }
    if (val & MUSIC_EVT_STOP)
    {
        s_skip   = 1;
        s_paused = 0;
        printf(">> Music: Stop\r\n");
    }
		// ? 音量加
    if (val & MUSIC_EVT_VOL_UP)
    {
        if (vsset.mvol < 240) vsset.mvol += 20;
        VS_Set_Vol(vsset.mvol);
        taskENTER_CRITICAL();
        g_music.volume = vsset.mvol / 27;
        taskEXIT_CRITICAL();
        UI_NOTIFY(UI_EVT_MUSIC);
        printf(">> Music: Vol+ mvol=%d\r\n", vsset.mvol);
    }
    // ? 音量减
    if (val & MUSIC_EVT_VOL_DOWN)
    {
        if (vsset.mvol > 20) vsset.mvol -= 20;
        VS_Set_Vol(vsset.mvol);
        taskENTER_CRITICAL();
        g_music.volume = vsset.mvol / 27;
        taskEXIT_CRITICAL();
        UI_NOTIFY(UI_EVT_MUSIC);
        printf(">> Music: Vol- mvol=%d\r\n", vsset.mvol);
    }
    // ? 倍速切换
    if (val & MUSIC_EVT_SPEED)
    {
        taskENTER_CRITICAL();
        g_music.speed = (g_music.speed + 1) % 3;
        taskEXIT_CRITICAL();
        uint8_t spd = g_music.speed == 0 ? 1 :
                      g_music.speed == 1 ? 2 : 4;
        VS_Set_Speed(spd);
        UI_NOTIFY(UI_EVT_MUSIC);
        printf(">> Music: Speed=%dx\r\n", spd);
    }
}

static void play_one_song(uint8_t idx)
{
    static char    path[40];
    static FIL     file;       // ? static 不占栈
    FRESULT        res;
    UINT           bw;
    static uint8_t buf[256];   // ? static 不占栈

    snprintf(path, sizeof(path), "0:/%s", s_music_list[idx]);
    printf(">> Music: [%s]\r\n", path);

    taskENTER_CRITICAL();
    strncpy(g_music.filename, s_music_list[idx], 31);
    g_music.filename[31] = '\0';
    g_music.is_playing   = 1;
    g_music.song_idx     = idx;
    g_music.song_total   = s_music_count;
    g_music.decode_time  = 0;
    taskEXIT_CRITICAL();
    UI_NOTIFY(UI_EVT_MUSIC);

    VS_Restart_Play();
    VS_Set_All();
    VS_Reset_DecodeTime();

    res = f_open(&file, path, FA_READ);
    if (res != FR_OK)
    {
        printf(">> Music: f_open failed %d\r\n", res);
        return;
    }

    VS_SPI_SpeedHigh();
    TickType_t last_tick = xTaskGetTickCount();
    uint16_t   i = 0;

    while (1)
    {
        /* 暂停：等待并持续检查通知（带50ms超时，响应更快）*/
        while (s_paused)
        {
            process_notify(pdMS_TO_TICKS(50));
            if (s_skip) goto song_end;
        }

        if (s_skip) goto song_end;

        res = f_read(&file, buf, sizeof(buf), &bw);
        if (bw == 0 || res != FR_OK) break;

        i = 0;
        while (i < bw)
        {
            if (s_skip)   goto song_end;
            if (s_paused) break;

            if (VS_Send_MusicData(buf + i) == 0)
                i += 32;
            else
                vTaskDelay(pdMS_TO_TICKS(1));
        }

        /* 每256字节检查一次通知（不等待）*/
        process_notify(0);

        if (xTaskGetTickCount() - last_tick >= pdMS_TO_TICKS(1000))
        {
            last_tick = xTaskGetTickCount();
            uint16_t t = VS_Get_DecodeTime();
            taskENTER_CRITICAL();
            g_music.decode_time = t;
            taskEXIT_CRITICAL();
            UI_NOTIFY(UI_EVT_MUSIC);
        }
    }

song_end:
    f_close(&file);
    taskENTER_CRITICAL();
    g_music.is_playing = 0;
    taskEXIT_CRITICAL();
    UI_NOTIFY(UI_EVT_MUSIC);
}

void task_music(void *pv)
{
    vTaskDelay(pdMS_TO_TICKS(3000));

    VS_Init();
    VS_HD_Reset();
    VS_Soft_Reset();
    VS_Set_All();

    build_music_list();

    if (s_music_count == 0)
    {
        printf(">> Music: No music files\r\n");
        vTaskDelete(NULL);
        return;
    }

    for (;;)
    {
        if (!s_paused && !g_music.is_playing)
        { 
            uint8_t play_idx = s_cur_idx_g;  // ? 播放前记录索引
            s_skip = 0;

            play_one_song(play_idx);

            // 只有正常播完（非切歌）才自动下一首
            if (!s_skip)
                s_cur_idx_g = (s_cur_idx_g + 1) % s_music_count;
        }
        else
        {
            // 暂停或等待状态，带100ms超时等通知
            process_notify(pdMS_TO_TICKS(100));
        }
    }
}

void task_music_init(void)
{
    BaseType_t ret = xTaskCreate(task_music,
                                  "task_music",
                                  700,
                                  NULL,
                                  4,
                                  &task_music_Handler);
    if (ret != pdPASS)
        printf("task_music create failed!\r\n");
    else
        printf("task_music create successful!\r\n");
}