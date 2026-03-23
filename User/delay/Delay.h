#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

// 直接定义DWT寄存器地址
#define DWT_BASE            (0xE0001000UL)
#define DWT_CTRL            (*(volatile uint32_t*)(DWT_BASE + 0x00))
#define DWT_CYCCNT          (*(volatile uint32_t*)(DWT_BASE + 0x04))

// CoreDebug寄存器地址
#define CoreDebug_BASE      (0xE000EDF0UL)
#define CoreDebug_DEMCR     (*(volatile uint32_t*)(CoreDebug_BASE + 0x0C))

// 位定义
#define CoreDebug_DEMCR_TRCENA_Msk    (1 << 24)
#define DWT_CTRL_CYCCNTENA_Msk        (1 << 0)

// 延时函数类型
typedef enum {
    DELAY_TYPE_BUSY_WAIT,     // 忙等待（用于时序严格的场景）
    DELAY_TYPE_TASK_DELAY,    // 任务延时（用于不严格的场景）
    DELAY_TYPE_HARDWARE_TIMER // 硬件定时器（最精确）
} Delay_Type_t;

// 初始化函数
void Delay_Init();
void Delay_DeInit(void);

// 延时函数
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

// 精确延时（不受任务调度影响）
void Delay_us_Precise(uint32_t us);
void Delay_ms_Precise(uint32_t ms);


void Delay_us_Loop(uint32_t us);
void Delay_ms_Loop(uint32_t ms);
#endif