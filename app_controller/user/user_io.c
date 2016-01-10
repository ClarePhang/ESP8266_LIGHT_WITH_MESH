/*
Handler routine for buttons and the charger. All the hardware differences between the old and new button
hardware should be put in this file; the rest of the firmware ought to be hardware agnostic.
*/

#include "user_io.h"
#include "os_type.h"
#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "gpio.h"
#include "driver/gpio16.h"



typedef struct {
	int mux;		//GPIO mux reg
	int func;		//Value to write to GPIO mux reg
	int num;		//GPIO number, or -1 if it doesn't exist on this specific hardware implementation
	int flags;		//One of FL_*
} InputDesc;

#define FL_ACTLO	(1<<0)			//Input is active low
#define FL_DISCHARGE	(1<<1)		//Input has a cap that needs discharging when not reading input

static const ICACHE_RODATA_ATTR InputDesc inputDesc[]={
#if NEW_BUTTON
	{PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 5, FL_DISCHARGE},
	{PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10, 10, FL_DISCHARGE},
	{PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, 4, FL_DISCHARGE},
	{PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9, 9, FL_DISCHARGE},
	{0, 0, 16, FL_ACTLO}, //I have no idea why this appears inverted. It's basically connected directly to Vusb, ffs.
	{PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12, FL_ACTLO}
#else
	{PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12, 12, FL_DISCHARGE|FL_ACTLO},
	{PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4, 4, FL_DISCHARGE|FL_ACTLO},
	{PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, 13, FL_DISCHARGE|FL_ACTLO},
	{PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5, 5, FL_DISCHARGE|FL_ACTLO},
	{0, 0, -1, 0},
	{0, 0, -1, 0}
#endif
};

#if NEW_BUTTON
uint32 pwmIoInfo[3][3]={
		{PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15, 15},
		{PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13, 13},
		{PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14, 14}
	};
#endif


#if NEW_BUTTON
#define HOLD_MUX PERIPHS_IO_MUX_GPIO2_U
#define HOLD_MUXVAL FUNC_GPIO2
#define HOLD_PIN 2
#else
#define HOLD_MUX PERIPHS_IO_MUX_MTMS_U
#define HOLD_MUXVAL FUNC_GPIO14
#define HOLD_PIN 14
#endif

#if NEW_BUTTON
#define BATTERY_LOW_THRESHOLD_MV 3000
#else
#define BATTERY_LOW_THRESHOLD_MV 2100
#endif

#if NEW_BUTTON
#define NIGHTLIGHT_ON_PIN 15
#define NIGHTLIGHT_OFF_PIN 14
#endif

static os_timer_t buttonTimer;
static int powerholdCount;
static int bootInputs;
static int batteryVoltageMv;

/* 
  Read the inputs as defined in inputDesc. Returns an int with a bit set for each input that's active.
  Some inputs are connected to capacitors, because we want to be able read the state of the buttons that 
  were pressed before the ESP started up. After the ESP has started, they hinder us: they'll lengthen the
  amount of time a button seems pressed, making a short press look like a long one. To go against that, we
  switch the GPIOs connected to those to outputs and manually drain the capacitors when not reading the 
  inputs.
*/
int ICACHE_FLASH_ATTR io_get_inputs() {
	int ret, isHi, x;
	uint32 val_raw;
	//Switch discharged outputs back to input
	for (x=0; x<INPUT_COUNT; x++) {
		if (inputDesc[x].flags&FL_DISCHARGE) {
			if (inputDesc[x].flags&FL_ACTLO) gpio_output_set((1<<inputDesc[x].num), 0, 0, (1<<inputDesc[x].num));
			if (!(inputDesc[x].flags&FL_ACTLO)) gpio_output_set(0, (1<<inputDesc[x].num), 0, (1<<inputDesc[x].num));
		}
	}
	//Let parasitic capacitance of gpio pin drain
	os_delay_us(1000);

	//Read values
	val_raw = gpio_input_get()&0xffff;
	if (gpio16_input_get()) val_raw|=(1<<16);
	ret=0;
	for (x=0; x<INPUT_COUNT; x++) {
		if (inputDesc[x].num!=-1) {
			isHi=(val_raw&(1<<inputDesc[x].num));
			if (isHi && !(inputDesc[x].flags&FL_ACTLO)) ret|=(1<<x);
			if (!isHi && (inputDesc[x].flags&FL_ACTLO)) ret|=(1<<x);
		}
	}

	//Go back to discharging capacitors where needed
	for (x=0; x<INPUT_COUNT; x++) {
		if (inputDesc[x].flags&FL_DISCHARGE) {
			if (inputDesc[x].flags&FL_ACTLO) gpio_output_set((1<<inputDesc[x].num), 0, (1<<inputDesc[x].num), 0);
			if (!(inputDesc[x].flags&FL_ACTLO)) gpio_output_set(0, (1<<inputDesc[x].num), (1<<inputDesc[x].num), 0);
		}
	}

	return ret;
}

int ICACHE_FLASH_ATTR io_get_boot_inputs() {
	return bootInputs;
}

static void ICACHE_FLASH_ATTR io_pwm_init() {
	uint32 duty[]={0,0,0};
	pwm_init(470, duty, 3, pwmIoInfo);
	pwm_start();
}

void pwmdummy() {
	//dummy
}

//Call this with PWM disabled.
static void ICACHE_FLASH_ATTR enableNightlight(int ena) {
	int x;
	//Kill PWM
	RTC_REG_WRITE(FRC1_CTRL_ADDRESS, 0);
	TM1_EDGE_INT_DISABLE();
	if (ena) {
		gpio_output_set((1<<NIGHTLIGHT_ON_PIN), (1<<NIGHTLIGHT_OFF_PIN), (1<<NIGHTLIGHT_ON_PIN)|(1<<NIGHTLIGHT_OFF_PIN), 0);
		os_delay_us(100);
	} else {
		gpio_output_set((1<<NIGHTLIGHT_OFF_PIN), (1<<NIGHTLIGHT_ON_PIN), (1<<NIGHTLIGHT_ON_PIN)|(1<<NIGHTLIGHT_OFF_PIN), 0);
		os_delay_us(10000);
	}
	gpio_output_set(0, (1<<NIGHTLIGHT_OFF_PIN)|(1<<NIGHTLIGHT_ON_PIN), (1<<NIGHTLIGHT_ON_PIN)|(1<<NIGHTLIGHT_OFF_PIN), 0);
}

static void gpioInit() {
	int x;
#if NEW_BUTTON
	//Keep the ESP powered on
	PIN_FUNC_SELECT(HOLD_MUX, HOLD_MUXVAL);
	gpio_output_set((1<<HOLD_PIN), 0, (1<<HOLD_PIN), 0);
	//Change PWM pins to GPIO and output 0 to kill the LEDs
	for (x=0; x<3; x++) {
		PIN_FUNC_SELECT(pwmIoInfo[x][0], pwmIoInfo[x][1]);
		gpio_output_set(0, (1<<pwmIoInfo[x][2]), 0, (1<<pwmIoInfo[x][2]));
	}
#endif
	//Initialize the input GPIOs
	for (x=0; x<INPUT_COUNT; x++) {
		if (inputDesc[x].num==-1) {
			//Do nothing, input is undefined
		}else if (inputDesc[x].num==16) {
			gpio16_input_conf();
		} else {
			PIN_FUNC_SELECT(inputDesc[x].mux, inputDesc[x].func);
			gpio_output_set(0, (1<<inputDesc[x].num), 0, (1<<inputDesc[x].num));
		}
	}
}

//We need to keep the ESP powered and capture the bootup button state ASAP. We can't immediately
//init everything: eg reading the battery voltage is better done when the system is completely up.
void ICACHE_FLASH_ATTR io_init_early() {
	//Initialize GPIOs
	gpioInit();
	//Capture boot button state
	bootInputs=io_get_inputs();
}

//Read battery, enable PWM etc.
void ICACHE_FLASH_ATTR io_init() {
	int x;
	//Between io_init_main and io_init, the firmware will re-initialize some gpios. Re-re-initialize them.
	gpioInit();
#if NEW_BUTTON
	//We can only measure the battery status with the nightlight feature on. Nightlight has to be done with pwm off.
	enableNightlight(1);

	os_delay_us(1000);
	x=system_adc_read();
	//AD is 1024 at Tout=1V. There is an (800K+1M)/200K resistor divider to lower the battery voltage into that range.
	//This means that the Tout value of 1024 is reached at a voltage of 10 volt.
	batteryVoltageMv=(x*10000)/1024;
	
	//Okay, initialize PWM.
	io_pwm_init();
#else
	x=system_get_vdd33();
	batteryVoltageMv=(x*1000)/1024;
#endif
	os_printf("Battery: %d mv\n", batteryVoltageMv);
}


/*
  We make the decision to keep power on or power off the ESP using these routines. Any process that wants 
  to keep the power to the ESP on, can call io_powerkeep_hold. If the process is finished, it will do a 
  io_powerkeep_release. The ESP will only power off after all processes are done and have called 
  io_powerkeep_release.
*/

void ICACHE_FLASH_ATTR io_powerkeep_hold() {
	powerholdCount++;
	os_printf("Powerdown hold count: %d\n", powerholdCount);
}

void ICACHE_FLASH_ATTR io_powerkeep_release() {
	powerholdCount--;
	os_printf("Powerdown hold count: %d\n", powerholdCount);
	if (powerholdCount<=0) {
		//Okay, we're done; release power.
		os_printf("Powering down.\n");
		UART_WaitTxFifoEmpty(0, 5000);
		//This should disable PWM, which is needed to set the nightlight.
		io_rgbled_set_color(0,0,0);
		os_delay_us(10000);
		//Set nightlight to on or off.
		//ToDo: Read the enable value from flash config area. Is now hardcoded to on.
		enableNightlight(0);
		//Make GPIO0 high, to lower the chance of the ESP booting up in programming mode next time.
		gpio_output_set((1<<0), 0, (1<<0), 0);
#if NEW_BUTTON
		//Software fix for hardware problem... sometimes GPIO12 (=USB power detect) will mysteriously 
		//keep leaking power into the LDO enable input. Force low for a while to stop this from happening.
		gpio_output_set(0, (1<<12), (1<<12), 0);
		os_delay_us(1000);
		gpio_output_set(0, (1<<12), 0, (1<<12));
#endif
		//Kill power.
		gpio_output_set(0, (1<<HOLD_PIN), (1<<HOLD_PIN), 0);
		//We should be dead now. If not, getting killed by the wdt may be a good idea anyway.
		while(1);
	}
}


//Generic getter for minimum battery voltage
uint32 ICACHE_FLASH_ATTR io_get_battery_voltage_mv() {
	return batteryVoltageMv;
}

int ICACHE_FLASH_ATTR io_battery_is_lo() {
	return batteryVoltageMv<=BATTERY_LOW_THRESHOLD_MV;
}


#if NEW_BUTTON

/* 
Connections:
GPIO0 - Green (voltage doubler & enable mosfet)
GPIO14 - Blue (voltage doubler & enable mosfet), nightlight disable
GPIO15 - Red (enable mosfet), nightlight enable

For the green and blue LEDs, the relation between PWM and intensity can be a bit wonky, because
the GPIO-pin controls both the voltage doubler as well as the LED duty cycle. It's safe to assume that
up to a duty cycle of 50%, the LED power will be roughly linear with the PWM value. (Due to the human
eye, it will look like an exponential curve, however.)
*/

/*
Duty cycle is 0-10240
These DUTY_MAX values give a light that's most similar to white.
ToDo (if really needed): Because the red LED feeds directly off the battery voltage, its intensity
changes slightly when the battery voltage decreases or a charger is attached. Should we compensate for
this? We know the battery voltage so this is theoretically possible.
*/
#define DUTY_MAX_R 2000
#define DUTY_MAX_G 5120
#define DUTY_MAX_B 5120

//Gives the PWM duty cycle (0-4095) for a required intensity (0-255)
static const int ICACHE_RODATA_ATTR intPwmLookupTab[256]={
	0, 1, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 21, 23, 24, 26, 
	28, 30, 31, 33, 35, 37, 39, 40, 42, 44, 46, 49, 51, 53, 55, 58, 
	60, 63, 65, 68, 71, 74, 76, 79, 82, 86, 89, 92, 95, 99, 102, 106, 
	110, 113, 117, 121, 125, 129, 133, 138, 142, 147, 151, 156, 160, 165, 170, 175, 
	180, 186, 191, 196, 202, 207, 213, 219, 225, 231, 237, 243, 250, 256, 263, 270, 
	276, 283, 290, 297, 305, 312, 320, 327, 335, 343, 351, 359, 367, 376, 384, 393, 
	401, 410, 419, 428, 438, 447, 456, 466, 476, 486, 496, 506, 516, 527, 538, 548, 
	559, 570, 582, 593, 604, 616, 628, 640, 652, 664, 676, 689, 702, 714, 727, 741, 
	754, 767, 781, 795, 809, 823, 837, 852, 866, 881, 896, 911, 926, 942, 957, 973, 
	989, 1005, 1022, 1038, 1055, 1072, 1089, 1106, 1123, 1141, 1159, 1176, 1195, 1213, 1231, 1250, 
	1269, 1288, 1307, 1327, 1346, 1366, 1386, 1406, 1427, 1447, 1468, 1489, 1510, 1532, 1553, 1575, 
	1597, 1619, 1641, 1664, 1687, 1710, 1733, 1757, 1780, 1804, 1828, 1852, 1877, 1902, 1926, 1952, 
	1977, 2003, 2028, 2054, 2081, 2107, 2134, 2161, 2188, 2215, 2243, 2271, 2299, 2327, 2355, 2384, 
	2413, 2442, 2472, 2501, 2531, 2561, 2592, 2622, 2653, 2684, 2716, 2747, 2779, 2811, 2844, 2876, 
	2909, 2942, 2975, 3009, 3043, 3077, 3111, 3146, 3180, 3216, 3251, 3287, 3322, 3359, 3395, 3432, 
	3468, 3506, 3543, 3581, 3619, 3657, 3695, 3734, 3773, 3813, 3852, 3892, 3932, 3973, 4013, 4054
};


//Routine to abstract away any hardware and eye dislinearities. Feed this with the required visual intensity
//of the LEDs (0-255).
//ToDo: Do we need to compensate for any weirdness in the voltage multiplier routines?
void ICACHE_FLASH_ATTR io_rgbled_set_color(int r, int g, int b) {
	//Clip values
	if (r<0) r=0;
	if (r>255) r=255;
	if (g<0) g=0;
	if (g>255) g=255;
	if (b<0) b=0;
	if (b>255) b=255;
	pwm_set_duty((intPwmLookupTab[r]*DUTY_MAX_R)/4096, 0);
	pwm_set_duty((intPwmLookupTab[g]*DUTY_MAX_G)/4096, 1);
	pwm_set_duty((intPwmLookupTab[b]*DUTY_MAX_B)/4096, 2);
	pwm_start();
}


#else

void ICACHE_FLASH_ATTR io_rgbled_set_color(int r, int g, int b) {
	//Not implemented: old button does not have any RGB leds
}
#endif
