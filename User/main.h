#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
/***********************STM32***********************/
#include "stm32f10x.h"
#include "palette.h"
#include <string.h>
#include "./usart/bsp_usart.h"	
#include "./lcd/bsp_ili9341_lcd.h"
#include "./lcd/bsp_xpt2046_lcd.h"
#include "./flash/bsp_spi_flash.h"
#include "./led/bsp_led.h" 
/*********************END******************/

/***********************LVGL***********************/
#include "../../lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_examples.h"


/*********************END******************/

/***********************FreeRtos***********************/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"   // Èí¼þ¶¨Ê±Æ÷ API
/*********************END******************/

#endif
