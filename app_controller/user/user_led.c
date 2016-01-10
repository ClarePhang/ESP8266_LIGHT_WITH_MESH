#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "gpio.h"
#include "pwm.h"
#include "user_io.h"
#include "user_led.h"


static int nightlightEnabled=0;


#define LED_R (1<<0)
#define LED_G (1<<1)
#define LED_B (1<<2)

#define LPAT_SOLID 0	//solid color
#define LPAT_BLIP 1		//on for a while, then always off
#define LPAT_THROB 2	//sinodial breathing-like pattern
#define LPAT_BLINK 3	//on, off, on, off etc

typedef struct {
	int pattern;	//Pattern. One of LPAT_*
	int color;		//Bitwise OR of LED_R, LED_G, LED_B
	int speed;		//Repeat freq = about 10sec/speed
} LedPattern;

//Pattern definitions.
//Make sure the indexes of this conform to the LED_PATTERN_* defines in user_led.h!
static LedPattern patterns[]={
	{LPAT_SOLID, 0, 1},			//LED_PATTERN_OFF
	{LPAT_BLIP, LED_G, 10},			//LED_PATTERN_PACKETSENT
	{LPAT_BLIP, LED_R, 1},			//LED_PATTERN_PACKETSENT_BATLO
	{LPAT_BLINK, LED_B, 5},		//LED_PATTERN_SOFTAP
	{LPAT_THROB, LED_G, 5},		//LED_PATTERN_CHARGING
	{LPAT_SOLID, LED_G, 1},		//LED_PATTERN_FULL
};

static int color=0;				//Current color
static int pattern=LPAT_SOLID;	//Current pattern type
static int speed=1;				//Current speed

static int patPos=0;			//Current position within the pattern
static int prevPatternType=LED_PATTERN_OFF; //so we won't reset the pattern in successive calls to led_pattern

//This code does a powerhold to make sure the user doesn't see a pattern that's suddenly aborted because something
//decided to power off the button. It will keep the button powered until the pattern is finished.
static int powerHeld=0;			//True if we are doing a powerhold.

static ETSTimer ledTimer;


#define PATPOSMAX 512 //multiples of 2 please

//Periodic timer to calculate the current intensity for the LEDs. Called with a freq of 50Hz
static void ICACHE_FLASH_ATTR ledTimerCb(void *arg) {
	int wraparound=0;

	if (pattern==LPAT_SOLID) {
		if (patPos==0) {
			io_rgbled_set_color((color&LED_R)?255:0, (color&LED_G)?255:0, (color&LED_B)?255:0);
		}
		wraparound=0;
	} else if (pattern==LPAT_BLIP || pattern==LPAT_BLINK) {
		if (patPos<(PATPOSMAX/8)) {
			io_rgbled_set_color((color&LED_R)?255:0, (color&LED_G)?255:0, (color&LED_B)?255:0);
		} else {
			io_rgbled_set_color(0,0,0);
			if (powerHeld) {
				io_powerkeep_release();
				powerHeld=0;
			}
		}
		wraparound=(pattern==LPAT_BLINK);
	} else if (pattern==LPAT_THROB) {
		int i;
		if (patPos<(PATPOSMAX/2)) {
			i=(patPos*256)/(PATPOSMAX/2);
		} else {
			i=((PATPOSMAX-patPos)*256)/(PATPOSMAX/2);
		}
		//Clip i slightly, to look more breathing-like
		if (i<20) i=20;
		if (i>255-20) i=255-20;
		io_rgbled_set_color((color&LED_R)?i:0, (color&LED_G)?i:0, (color&LED_B)?i:0);
		wraparound=1;
		if (powerHeld && patPos>(PATPOSMAX/2)) {
			io_powerkeep_release();
			powerHeld=0;
		}
	}

	patPos+=speed;
	if (patPos>PATPOSMAX) patPos=wraparound?0:PATPOSMAX;
}

void ICACHE_FLASH_ATTR led_init() {
	os_timer_disarm(&ledTimer);
	os_timer_setfn(&ledTimer, ledTimerCb, NULL);
	os_timer_arm(&ledTimer, 20, 1);
}

void ICACHE_FLASH_ATTR led_pattern(int patternType) {
	if (patternType==prevPatternType) return; //don't do anything if pattern hasn't changed
	prevPatternType=patternType;
	pattern=patterns[patternType].pattern;
	color=patterns[patternType].color;
	speed=patterns[patternType].speed;
	patPos=0;
	if (pattern!=LPAT_SOLID) {
		if (powerHeld==0) io_powerkeep_hold();
		powerHeld=1;
	}
}


//ToDo: move these routines to user_io.c
void ICACHE_FLASH_ATTR led_ena_nightlight(int nlenable) {
	nightlightEnabled=nlenable;
}


//VERY IMPORTANT! Needs to be called before shutdown or else nightlight setting will be random!
void ICACHE_FLASH_ATTR led_stop() {
	//Hope this stops the PWM generator...
	pwm_init(0, NULL, 0, NULL);
	pwm_start();
	//ToDo: pulse GPIO14 or GPIO15
}


