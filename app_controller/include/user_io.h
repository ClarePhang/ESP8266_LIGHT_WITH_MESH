#ifndef _USER_IO_H
#define _USER_IO_H

#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "gpio.h"

#define NEW_BUTTON  1

#define INPUT_COUNT 6
#define INPUT_BTN1 (1<<0)
#define INPUT_BTN2 (1<<1)
#define INPUT_BTN3 (1<<2)
#define INPUT_BTN4 (1<<3)
#define INPUT_CHRG (1<<4)
#define INPUT_CHRGDONE (1<<5)

#define INPUT_BUTTONMASK (INPUT_BTN1|INPUT_BTN2|INPUT_BTN3|INPUT_BTN4)

void ICACHE_FLASH_ATTR io_powerkeep_hold();
void ICACHE_FLASH_ATTR io_powerkeep_release();
void ICACHE_FLASH_ATTR io_init_early();
void ICACHE_FLASH_ATTR io_init();
int ICACHE_FLASH_ATTR io_get_inputs();
int ICACHE_FLASH_ATTR io_get_boot_inputs();
uint32 ICACHE_FLASH_ATTR io_get_battery_voltage_mv();
void ICACHE_FLASH_ATTR io_rgbled_set_color(int r, int g, int b);
int ICACHE_FLASH_ATTR io_battery_is_lo();

#endif
