#include "Delay.h"
#include "timers.h"
#include<stdio.h>

static Delay_Type_t delay_type = DELAY_TYPE_TASK_DELAY;
static uint8_t dwt_initialized = 0;

// ==================== 关键函数1：DWT初始化 ====================
static void DWT_Init(void)
{
    if(dwt_initialized) return;
    
    // 使用CMSIS函数
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // 使能跟踪
    DWT_CYCCNT = 0;                                 // 计数器清零
    DWT_CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // 使能计数器
    
    
    dwt_initialized = 1;
    
    //printf("DWT初始化完成，系统时钟: %lu Hz\n", SystemCoreClock);
}

// ==================== 关键函数2：精确微秒延时（用于DHT11） ====================
void Delay_us_Precise(uint32_t us)
{
    /*
     * 这个函数用于DHT11等时序严格的设备
     * 特点：不会被中断或任务切换打断
     */
    
    if(!dwt_initialized) {
        DWT_Init();
    }
    
    // 1. 进入临界区（关闭中断和调度）
    uint32_t primask = __get_PRIMASK();  // 保存当前中断状态
    __disable_irq();                     // 关闭所有中断
    
    // 2. 计算需要等待的周期数
    uint32_t start_tick = DWT_CYCCNT;
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
    
    // 3. 忙等待
    while((DWT_CYCCNT - start_tick) < delay_ticks) {
        // 空循环，保持时序
    }
    
    // 4. 恢复中断状态
    if(!primask) __enable_irq();
}

// ==================== 关键函数3：精确毫秒延时 ====================
void Delay_ms_Precise(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++) {
        Delay_us_Precise(1000);  // 循环1000次
    }
}

// ==================== 关键函数4：普通微秒延时 ====================
void Delay_us(uint32_t us)
{
    if(delay_type == DELAY_TYPE_BUSY_WAIT) {
        // 使用DWT忙等待
        if(!dwt_initialized) {
            DWT_Init();
        }
        
        uint32_t start_tick = DWT_CYCCNT;
        uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
        
        while((DWT_CYCCNT - start_tick) < delay_ticks);
        
    } else if(delay_type == DELAY_TYPE_TASK_DELAY) {
        // 对于较大的us值，使用FreeRTOS延时
        if(us >= 1000) {
            vTaskDelay(pdMS_TO_TICKS(us / 1000));
        } else {
            // 小于1ms使用忙等待
            Delay_us_Precise(us);
        }
    }
}

// ==================== 关键函数5：普通毫秒延时 ====================
void Delay_ms(uint32_t ms)
{
    if(delay_type == DELAY_TYPE_BUSY_WAIT) {
        Delay_ms_Loop(ms);
        
    } else if(delay_type == DELAY_TYPE_TASK_DELAY) {
        // 使用FreeRTOS延时（可能被任务切换）
        vTaskDelay(pdMS_TO_TICKS(ms));
        
    } else if(delay_type == DELAY_TYPE_HARDWARE_TIMER) {
        // 使用硬件定时器（需要额外配置）
        // 这里可以扩展硬件定时器的实现
        vTaskDelay(pdMS_TO_TICKS(ms));  // 暂时用任务延时替代
    }
}

// ==================== 关键函数6：初始化 ====================
void Delay_Init()
{
    
    DWT_Init();
    //printf("delay init ok!");
}

void Delay_DeInit(void)
{
    dwt_initialized = 0;
}


/**
  * @brief  空转微秒延时 (基于 72MHz 系统主频)
  * @note   由于指令执行时间，这种延时不精确，但不会被中断配置干扰
  */
void Delay_us_Loop(uint32_t us) {
    // 72MHz 下，大约每循环 8-12 次为 1us (视优化等级而定)
    // 这里取一个保守值，确保延时只长不短
    uint32_t count = us * 8; 
    while (count--) {
        __NOP(); // 空指令，防止循环被编译器优化掉
    }
}

/**
  * @brief  空转毫秒延时
  */
void Delay_ms_Loop(uint32_t ms) {
    while (ms--) {
        Delay_us_Loop(1000);
    }
}