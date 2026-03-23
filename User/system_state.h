#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

typedef enum {
    PAGE_MAIN = 0,
    PAGE_ADC,
    PAGE_USART,
    PAGE_EEPROM,
	  PAGE_EEPROM_Read,
		PAGE_Wifi,
		PAGE_BlueTeeth,
		PAGE_Buzze,
		PAGE_Led,
		PAGE_Key,
	  PAGE_Select,
	  PAGE_USART_Result,
	  PAGE_Weather
} page_id_t;
void system_state_init(void);
page_id_t get_current_page(void);   // ¿´ÅÆ×Ó
void set_current_page(page_id_t p); // ·­ÅÆ×Ó

#endif
