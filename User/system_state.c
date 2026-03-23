#include "system_state.h"
#include "FreeRTOS.h"
#include "semphr.h"

static page_id_t current_page = PAGE_MAIN; // 牌子本身
static SemaphoreHandle_t page_mux;         // 小锁，防止两个人同时翻

void system_state_init(void)               // 上电时调用一次
{
    page_mux = xSemaphoreCreateMutex();
	if(page_mux==NULL)
	{
		printf("page_mux create failed!\r\n");
	}
	else{
		printf("page_mux create successful!\r\n");
	}
}

page_id_t get_current_page(void)
{
    xSemaphoreTake(page_mux, portMAX_DELAY);
    page_id_t tmp = current_page;
    xSemaphoreGive(page_mux);
    return tmp;
}

void set_current_page(page_id_t p)
{
    xSemaphoreTake(page_mux, portMAX_DELAY);
    current_page = p;
    xSemaphoreGive(page_mux);
}