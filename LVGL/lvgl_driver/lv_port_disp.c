/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include"../../lvgl.h"
#include "./lcd/bsp_ili9341_lcd.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES 240
#define MY_DISP_VER_RES 320
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
        /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();
 
 
	 static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
	 lv_disp_drv_init(&disp_drv);                    	/*Basic initialization*/
 
    /*Set up the functions to access to your display*/
 
    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
 
    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;
   
#define  BUFFER_METHOD     1     //设置使用的缓存区大小
 
#if   BUFFER_METHOD == 1
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    static lv_color_t buf_1[MY_DISP_HOR_RES * 20];                          /*A buffer for 10 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 20);   /*Initialize the display buffer*/
	
	disp_drv.draw_buf = &draw_buf_dsc_1;
	
#elif BUFFER_METHOD == 2
  
    static lv_disp_draw_buf_t draw_buf_dsc_2;
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 15];                        /*A buffer for 10 rows*/
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 15];                        /*An other buffer for 10 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 15);   /*Initialize the display buffer*/
	disp_drv.draw_buf = &draw_buf_dsc_2;
 
#else
  
    static lv_disp_draw_buf_t draw_buf_dsc_3;
    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*An other screen sized buffer*/
    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2, MY_DISP_VER_RES * MY_DISP_VER_RES);   /*Initialize the display buffer*/
	disp_drv.draw_buf = &draw_buf_dsc_3;
	disp_drv.full_refresh = 1;
 
#endif
 
		
    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;
 
    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
	
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
#define FSMC_LCD_DATA    (*(volatile uint16_t *)(0x60020000))
#define FSMC_LCD_CMD     (*(volatile uint16_t *)(0x60000000))

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(disp_flush_enabled) {
        /* 1. 设置 LCD 的显示区域 (这个函数在 .h 中声明过，可以直接用) */
        ILI9341_OpenWindow(area->x1, area->y1, 
                           (area->x2 - area->x1 + 1), 
                           (area->y2 - area->y1 + 1));
        
        /* 2. 发送“开始写显存”命令 0x2C */
        FSMC_LCD_CMD = 0x2C; 

        /* 3. 批量发送像素数据 */
        uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
        
        /* 
         * 使用 while 循环直接往 FSMC 地址灌数据。
         * 这种方式绕过了所有函数调用开销，是 F103 上最快的刷屏方式。
         */
        while(size--) {
            FSMC_LCD_DATA = color_p->full; 
            color_p++;
//					__NOP(); __NOP(); __NOP();
        }
    }

    /* 4. 重要：通知 LVGL 刷新完成 */
    lv_disp_flush_ready(disp_drv);
}
/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}


#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
