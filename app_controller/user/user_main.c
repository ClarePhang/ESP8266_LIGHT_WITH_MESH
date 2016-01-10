/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_io.h"
#include "user_led.h"
#include "user_interface.h"

#ifdef SERVER_SSL_ENABLE
#include "ssl/cert.h"
#include "ssl/private_key.h"
#else
#ifdef CLIENT_SSL_ENABLE
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;
#endif
#endif


#define BUTTON_CHECK_INTERVAL_MS 100
#define LONGPRESS_TIME_MS 2000
os_timer_t buttonTimer;
os_timer_t chargeTimer;

static int timeSinceButtonPressed=0;
static uint16 buttonsPressedCum=0;


void user_rf_pre_init(void) {
	//Call io_init ASAP: it captures the buttons pressed at boot and keeps the power on.
	io_init_early();
}

//ToDo: This should also enable the buttons; people may want to use the thing while it is charging.
void chargeTimerCb(void *arg) {
	int input=io_get_inputs();
	os_printf("Inputs: %x\n", input);
	if ((input&INPUT_CHRG)==0) {
		//USB cable was unplugged. Kill power.
		os_timer_disarm(&chargeTimer);
		io_powerkeep_release(); //charge power release
	}
	if (input&INPUT_CHRGDONE) {
		led_pattern(LED_PATTERN_FULL);
	} else {
		led_pattern(LED_PATTERN_CHARGING);
	}
}

//ToDo: move to light_action.h ?
enum {
	COLOR_SET = 0,
	COLOR_CHG ,
	COLOR_TOGGLE,
	COLOR_LEVEL,
	LIGHT_RESET
};


void buttonTimerCb(void *arg) {
	int buttonsPressed=io_get_inputs()&INPUT_BUTTONMASK;
//	os_printf("Buttons: %x\n", buttonsPressed);
	timeSinceButtonPressed+=BUTTON_CHECK_INTERVAL_MS;
	if (timeSinceButtonPressed>=LONGPRESS_TIME_MS) {
		//Detected a long press. Handle accordingly.
		os_printf("Longpress %x\n", buttonsPressedCum);
		led_pattern(LED_PATTERN_SOFTAP);
        //A fixed key value for pairing.
        os_printf("pressed: %02x , pcum: %02x \r\n",buttonsPressed,buttonsPressedCum);
		if (buttonsPressed==(INPUT_BTN1|INPUT_BTN3)) {
			os_printf("PAIR START\r\n");
			buttonSimplePairStart(NULL);
		} else {
			//switch_EspnowChnSyncStart();
			switch_EspnowSendCmd( (buttonsPressedCum&INPUT_BUTTONMASK)|0xff00);
		}
		buttonsPressedCum=0;
	} else if (buttonsPressed==0) {
		if (!io_battery_is_lo()) {
			led_pattern(LED_PATTERN_PACKETSENT);
		} else {
			led_pattern(LED_PATTERN_PACKETSENT_BATLO);
		}
		//Buttons have been released.
		os_printf("Shortpress %x\n", buttonsPressedCum);
		switch_EspnowSendCmd(buttonsPressedCum&INPUT_BUTTONMASK);
	} else {
		//Buttons are still pressed. Check again in a while.
		buttonsPressedCum|=buttonsPressed;
		os_timer_arm(&buttonTimer, BUTTON_CHECK_INTERVAL_MS, 0);
	}
}

//ToDo: Can we put this in a header somewhere? --JD
extern void ieee80211_mesh_quick_init();

void user_system_init_done_cb() {
	io_init();
	led_init();
	//See why we booted.
	if (io_get_boot_inputs()&INPUT_CHRG) {
		//We're charging.
		os_printf("Charging...\n");
		os_timer_disarm(&chargeTimer);
		os_timer_setfn(&chargeTimer, chargeTimerCb, NULL);
		os_timer_arm(&chargeTimer, BUTTON_CHECK_INTERVAL_MS, 1);
		io_powerkeep_hold(); //charge power hold
	}
	buttonsPressedCum=io_get_boot_inputs();
	//Probably some button has been pressed.
	os_timer_disarm(&buttonTimer);
	os_timer_setfn(&buttonTimer, buttonTimerCb, NULL);
	os_timer_arm(&buttonTimer, BUTTON_CHECK_INTERVAL_MS, 0);

	ieee80211_mesh_quick_init();
	wifi_enable_6m_rate(true);
	switch_EspnowInit();
}

void user_init(void)
{
	uart_init(74880,74880);

	wifi_set_opmode(STATION_MODE); //todo: re-enable
	system_init_done_cb(user_system_init_done_cb);
}




