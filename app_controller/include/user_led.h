#ifndef USER_LED_H
#define USER_LED_H

//Do not change these defines without changing the pattern definitions in user_led.c!
#define LED_PATTERN_OFF					0
#define LED_PATTERN_PACKETSENT			1
#define LED_PATTERN_PACKETSENT_BATLO	2
#define LED_PATTERN_SOFTAP				3
#define LED_PATTERN_CHARGING			4
#define LED_PATTERN_FULL				5


void led_init();
void led_pattern(int pattern);
void led_ena_nightlight(int nlenable);
void led_stop();


#endif