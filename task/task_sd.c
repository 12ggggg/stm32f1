#include "task_sd.h"
#include "app_data.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "page_sd.h"
#include <stdio.h>
#include <string.h>

TaskHandle_t task_sd_Handler = NULL;

char    s_file_names[MAX_FILES][32];
uint8_t s_file_count = 0;

char g_sd_delete_name[32] = {0};

static FATFS s_fs;

/* ─────────────────────────────────────────
 * 扫描文件列表 + 获取容量
 * ───────────────────────────────────────── */
static void do_sd_scan(void)
{
    FATFS   *fs;
    DWORD    free_clust;
    FRESULT  res;

    // ? 改成 static，放在 .bss 段，不占任务栈
    static DIR     dir;
    static FILINFO fno;
	  memset(&dir, 0, sizeof(dir));
    memset(&fno, 0, sizeof(fno));
    /* 获取容量 */
    res = f_getfree("0:", &free_clust, &fs);
    if (res == FR_OK)
    {
        uint32_t total_sect = (fs->n_fatent - 2) * fs->csize;
        uint32_t free_sect  = free_clust * fs->csize;

        taskENTER_CRITICAL();
        g_sd.total_mb = total_sect / 2048;
        g_sd.used_mb  = (total_sect - free_sect) / 2048;
        g_sd.mounted  = 1;
        taskEXIT_CRITICAL();
    }
    else
    {
        taskENTER_CRITICAL();
        g_sd.mounted  = 0;
        g_sd.total_mb = 0;
        g_sd.used_mb  = 0;
        taskEXIT_CRITICAL();
        printf(">> SD: f_getfree failed res=%d\r\n", res);
        UI_NOTIFY(UI_EVT_SD);
        return;
    }

    /* 扫描根目录 */
    s_file_count = 0;
    res = f_opendir(&dir, "0:/");
    if (res != FR_OK)
    {
        printf(">> SD: f_opendir failed res=%d\r\n", res);
        UI_NOTIFY(UI_EVT_SD);
        return;
    }

    while (s_file_count < MAX_FILES)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == '\0') break;

        if (fno.fattrib & AM_DIR) continue;
        if (fno.fattrib & AM_HID) continue;

        strncpy(s_file_names[s_file_count], fno.fname, 31);
        s_file_names[s_file_count][31] = '\0';
        s_file_count++;
    }
    f_closedir(&dir);

    printf(">> SD: total=%luMB used=%luMB files=%d\r\n",
           g_sd.total_mb, g_sd.used_mb, s_file_count);

    UI_NOTIFY(UI_EVT_SD);
}

/* ─────────────────────────────────────────
 * 任务主体
 * ───────────────────────────────────────── */
void task_sd(void *pv)
{
    uint32_t   notify_val = 0;
    BaseType_t got;
    FRESULT    res;


    /* 挂载文件系统 */
    res = f_mount(&s_fs, "0:", 1);
    if (res == FR_NO_FILESYSTEM)
    {
        printf(">> SD: No filesystem, formatting...\r\n");
        res = f_mkfs("0:", 0, 0);
        if (res == FR_OK)
        {
            f_mount(NULL, "0:", 1);
            res = f_mount(&s_fs, "0:", 1);
            printf(">> SD: Format OK\r\n");
        }
        else
        {
            printf(">> SD: Format failed res=%d\r\n", res);
            g_sd.mounted = 0;
            UI_NOTIFY(UI_EVT_SD);
            vTaskDelete(NULL);
            return;
        }
    }
    else if (res != FR_OK)
    {
        printf(">> SD: f_mount failed res=%d\r\n", res);
        g_sd.mounted = 0;
        UI_NOTIFY(UI_EVT_SD);
        vTaskDelete(NULL);
        return;
    }

    printf(">> SD: Mounted OK\r\n");
    do_sd_scan();

    for (;;)
    {
        got = xTaskNotifyWait(0x00, 0xFFFFFFFF,
                              &notify_val,
                              pdMS_TO_TICKS(SD_AUTO_REFRESH_MS));

								if (got == pdTRUE)
				{
						if (notify_val & SD_EVT_DELETE)
						{
								char path[40];
								snprintf(path, sizeof(path), "0:/%s", g_sd_delete_name);
								FRESULT res = f_unlink(path);
								printf(">> SD: delete [%s] res=%d\r\n", path, res);
								do_sd_scan();
						}
						if (notify_val & SD_EVT_REFRESH)  // ← 用 if 不用 else if，两个可以同时处理
						{
								printf(">> SD: Manual refresh\r\n");
								do_sd_scan();
						}
				}
				else
				{
						printf(">> SD: Auto refresh\r\n");
						do_sd_scan();
				}
//				static uint8_t s_printed = 0;
//        if (!s_printed) {
//            s_printed = 1;
//            printf(">>sd watermark: %d\r\n",
//                   uxTaskGetStackHighWaterMark(NULL));
//        }
    }
}

/* ─────────────────────────────────────────
 * 任务创建
 * ───────────────────────────────────────── */
void task_sd_init(void)
{
    BaseType_t ret = xTaskCreate(task_sd,
                                  "task_sd",
                                  800,
                                  NULL,
                                  3,
                                  &task_sd_Handler);
    if (ret != pdPASS)
        printf("task_sd create failed!\r\n");
    else
        printf("task_sd create successful!\r\n");
}