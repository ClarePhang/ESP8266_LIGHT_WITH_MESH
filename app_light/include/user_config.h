#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "c_types.h"

#define ESP_PLATFORM        1

#define USE_OPTIMIZE_PRINTF
#define MESH_INFO(fmt,...)   //os_printf(fmt, ##__VA_ARGS__)  /*debug info for mesh*/
#define WEB_DBG(fmt,...)     //os_printf(fmt, ##__VA_ARGS__)   /*debug info for web server*/
#define ESPNOW_DBG(fmt,...)  //os_printf(fmt, ##__VA_ARGS__)	/*debug info for espnow*/
#define ACTION_DBG(fmt,...)  //os_printf(fmt, ##__VA_ARGS__) /*debug info for espnow action*/
#define SC_INFO(fmt,...)     //os_printf(fmt, ##__VA_ARGS__)   /*debug info for ESP-TOUCH*/
#define ESP_DBG(fmt,...)     //os_printf(fmt, ##__VA_ARGS__)    /*general debug info*/
#define SEND_DBG(fmt,...)    //os_printf(fmt, ##__VA_ARGS__)  /*debug info for user tx queue*/
#define SP_DBG(fmt,...)      //os_printf(fmt, ##__VA_ARGS__)  /*debug info for simple pair*/
#define DF_DBG(fmt,...)      //os_printf(fmt, ##__VA_ARGS__)  /*debug info for local device scan*/
#define LIGHT_INFO(fmt,...)  //os_printf(fmt, ##__VA_ARGS__)  /*debug info for LIGHT*/

#define STRING_SV_IN_FLASH 1




#if ESP_PLATFORM
#define PLUG_DEVICE             0
#define LIGHT_DEVICE            1
#define SENSOR_DEVICE			0
#define LIGHT_SWITCH            0

#define ESP_MESH_SUPPORT   1  /*set 1: enable mesh*/
    #define MESH_HEADER_EN  1
    #define MESH_HEADER_MODE  (ESP_MESH_SUPPORT&&MESH_HEADER_EN)

#define ESP_TOUCH_SUPPORT  1   /*set 1: enable esptouch*/
#define ESP_WEB_SUPPORT 0     /*We temporarily disable this function.  set 1: enable new http webserver*/
#define ESP_MDNS_SUPPORT 0     /*set 1: enable mdns*/
#define ESP_NOW_SUPPORT 1      /*set 1: enable espnow*/
    #define ESP_SIMPLE_PAIR_EN  1 /* set 1: enable simplepair for espnow*/
#define ESP_SIMPLE_PAIR_SUPPORT (ESP_NOW_SUPPORT && ESP_SIMPLE_PAIR_EN)


#define ESP_DEBUG_MODE  1
#define ESP_RESET_DEBUG_EN 1

#define MESH_TEST_VERSION   41


#if PLUG_DEVICE || LIGHT_DEVICE
#define BEACON_TIMEOUT  150000000
#define BEACON_TIME     (150*1000+os_random()%10000)

#endif



//not used
typedef enum{
    ESP_DEV_PLUG,
    ESP_DEV_PLUGS,
    ESP_DEV_LIGHT,
    ESP_DEV_HUMITURE,
    ESP_DEV_GAS_ALARM,
    ESP_DEV_VOLTAGE,
    ESP_DEV_REMOTE,
    ESP_DEV_SWITCH,
}DevType;


//flash data address.
//should be binded together later
#define FLASH_PARAM_ADDR_0 0x7C  //SAVE LIGHT PARAM(DUTY,PERIOD...)
#define FLASH_PARAM_ADDR_1 0X7D  //SAVE PLATFORM PARAM(DEVKEY,TOKEN,...)
#define FLASH_PARAM_ADDR_2 0X79  //SAVE SWITCH MAC LIST AND ESPNOW KEY
#define FLASH_PARAM_ADDR_3 0xF9  //RECORD EXCEPTION PARAM.
#define FLASH_PARAM_ADDR_4 0xF8  //RECORD EXCEPTION INFO.
//#define FLASH_PARAM_ADDR_5 0x76  //RECORD MODULE ENABLE FLAGS,not used
#define FLASH_PARAM_ADDR_6 0xF5  //RECORD ESPNOW ACTION FUNCTIONS
#define FLASH_PARAM_ADDR_7 0xF4  //RECORD EXECPTION DUMP INFO

//#define FLASH_PARAM_ADDR_8 0x76
#define FLASH_PARAM_ADDR_9 0x77
#define FLASH_PARAM_ADDR_10 0x78
#define FLASH_PARAM_ADDR_11 0x74


/*============================================================*/
/*====                                                    ====*/
/*====        PARAMS FOR KEEPING EXCEPTION INFO           ====*/
/*====                                                    ====*/
/*============================================================*/
#if ESP_DEBUG_MODE
//if a fatal exception occured, read out the dumped info from these address
//the corresponding .dump and .s files are also needed
#define FLASH_DEBUG_DUMP_ADDR  FLASH_PARAM_ADDR_7
#define Flash_DEBUG_INFO_ADDR  FLASH_PARAM_ADDR_3
#define FLASH_DEBUG_SV_ADDR    FLASH_PARAM_ADDR_4
#define FLASH_DEBUG_SV_SIZE    0x1000  

#define FLASH_PRINT_INDEX_ADDR   FLASH_PARAM_ADDR_11
#define FLASH_PRINT_INFO_ADDR_0  FLASH_PARAM_ADDR_9
#define FLASH_PRINT_INFO_ADDR_1  FLASH_PARAM_ADDR_10
#endif



/*============================================================*/
/*====                                                    ====*/
/*====        PARAMS FOR ESP-MESH                         ====*/
/*====                                                    ====*/
/*============================================================*/

#if ESP_MESH_SUPPORT

    /*This is the configuration of mesh*/
	#define MESH_SERVER_CONF  1
	#if MESH_SERVER_CONF
	    #define MESH_SERVER_PORT  7000
	#else
        #define MESH_SERVER_PORT  8000
	#endif

    /*set 1 to use the GROUP_ID below,otherwise, a default GROUP_ID would be used  */
    #define MESH_SET_GROUP_ID  1
	
	/*A unique ID for a mesh network, suggest to use the MAC of one of the device in the mesh.*/
	static const uint8 MESH_GROUP_ID[6] = {0x18,0xfe,0x34,0x00,0x00,0x11};

	/*number of retry attempts after mesh enable failed;*/
    #define MESH_INIT_RETRY_LIMIT 0       

	/*set max hops of mesh*/
    #define MESH_MAX_HOP_NUM  5      

	/*limit time of mesh init, 0:no limit*/
    #define MESH_INIT_TIME_LIMIT  0 

	/* timeout for mesh enable */
    #define MESH_TIME_OUT_MS   30000     

	/*0:do not set a timeout for mesh init, mesh would try searching and joining in a available network*/
    #define MESH_INIT_TIMEOUT_SET  0	  

	/*time expire to check mesh init status*/
    #define MESH_STATUS_CHECK_MS  1000    

	/*length of binary upgrade stream in a single packet*/
    #define MESH_UPGRADE_SEC_SIZE 640   

	/*SET THE DEFAULT MESH SSID PATTEN;THE FINAL SSID OF SOFTAP WOULD BE "MESH_SSID_PREFIX_X_YYYYYY"*/
    #define MESH_SSID_PREFIX "ESP_MESH"

	/*AUTH_MODE OF SOFTAP FOR EACH MESH NODE*/
    #define MESH_AUTH  AUTH_WPA2_PSK     

	/*SET PASSWORD OF SOFTAP FOR EACH MESH NODE*/
    #define MESH_PASSWORD "123123123"    

#endif






/*============================================================*/
/*====                                                    ====*/
/*====        PARAMS FOR ESP-LIGHT                        ====*/
/*====                                                    ====*/
/*============================================================*/

#if LIGHT_DEVICE
    /* TAKE 8Mbit flash as example*/
    /* 0x7C000     |     0x7D000        |        0x7E000      |  0x7F000 */
    /* light_param | platform_param(a)  |  platform_param(b)  |  platform_param_flag */

    /* You can change to other sector if you use other size spi flash. */
    /* Refer to the documentation about OTA support and flash mapping*/
    #define PRIV_PARAM_START_SEC		FLASH_PARAM_ADDR_0
    #define PRIV_PARAM_SAVE     0

	/* NOTICE---this is for 1024KB spi flash.
	 * you can change to other sector if you use other size spi flash. */
    #define ESP_PARAM_START_SEC		FLASH_PARAM_ADDR_1    	
    #define TOKEN_SIZE 41
    
    /*Define the channel number of PWM*/
    /*In this demo, we can set 3 for 3 PWM channels: RED, GREEN, BLUE*/
    /*Or , we can choose 5 channels : RED,GREEN,BLUE,COLD-WHITE,WARM-WHITE*/
    #define PWM_CHANNEL	5  /*5:5channel ; 3:3channel*/
    
    /*Definition of GPIO PIN params, for GPIO initialization*/
    #define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
    #define PWM_0_OUT_IO_NUM 12
    #define PWM_0_OUT_IO_FUNC  FUNC_GPIO12
    #define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
    #define PWM_1_OUT_IO_NUM 15
    #define PWM_1_OUT_IO_FUNC  FUNC_GPIO15
    #define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
    #define PWM_2_OUT_IO_NUM 13
    #define PWM_2_OUT_IO_FUNC  FUNC_GPIO13
    #define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
    #define PWM_3_OUT_IO_NUM 14
    #define PWM_3_OUT_IO_FUNC  FUNC_GPIO14
    #define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_GPIO5_U
    #define PWM_4_OUT_IO_NUM 5
    #define PWM_4_OUT_IO_FUNC  FUNC_GPIO5
    
    //---------------------------------------------------
    /*save RGB params to flash when calling light_set_aim*/
    #define SAVE_LIGHT_PARAM  0                 /*set to 0: do not save color params*/
    /*check current consumption and limit the total current for LED driver IC*/
    /*NOTE: YOU SHOULD REPLACE WIHT THE LIMIT CURRENT OF YOUR OWN APPLICATION*/
    #define LIGHT_CURRENT_LIMIT  1              /*set to 0: do not limit total current*/
    #if LIGHT_CURRENT_LIMIT
    #define LIGHT_TOTAL_CURRENT_MAX  (550000) /*550000/1000 MA AT MOST*/
    #define LIGHT_CURRENT_MARGIN  (80000)     /*80000/1000 MA CURRENT RAISES WHILE TEMPERATURE INCREASING*/
    #define LIGHT_CURRENT_MARGIN_L2  (110000) /*110000/1000 MA */
    #define LIGHT_CURRENT_MARGIN_L3  (140000) /*140000/1000 MA */
    #endif
	extern void _LINE_DESP();
#endif





/*============================================================*/
/*====                                                    ====*/
/*====        PARAMS FOR ESP-NOW                          ====*/
/*====                                                    ====*/
/*============================================================*/
#define DEV_MAC_LEN 6		  /*MAC ADDR LENGTH(BYTES)*/

#if ESP_NOW_SUPPORT
    #define ESPNOW_DEBUG 1
	#define WIFI_DEFAULT_CHANNEL 1
    #define ESPNOW_ENCRYPT  0     /*enable espnow encryption*/
	
    #define ESPNOW_KEY_HASH 0     /*Use hashed value of given key as the espnow encryption key */
    #define ESPNOW_KEY_LEN 16     /*ESPNOW KEY LENGTH,DO NOT CHANGE IT,FIXED AS 16*/
    #define SWITCH_DEV_NUM 5      /*SET THE NUMBER OF THE CONTROLLER DEVICES,IF SET ESPNOW_ENCRYPT,THE NUMBER IS LIMITED TO 5*/
	#define ESPNOW_SIMPLE_CMD_MODE  1 /*"simple command mode: the button always sends the fixed command on each key*/
	#define LIGHT_PAIRED_DEV_PARAM_ADDR FLASH_PARAM_ADDR_2 /* flash addr to save the paired button info*/
	#define LIGHT_ESPNOW_ACTION_MAP_PARAM_ADDR FLASH_PARAM_ADDR_6  /* flash address to save the espnow action definition.*/
	
#endif






/*============================================================*/
/*====                                                    ====*/
/*====        PARAMS FOR ESP-TOUCH                        ====*/
/*====                                                    ====*/
/*============================================================*/
#if ESP_TOUCH_SUPPORT
    #define ESPTOUCH_CONNECT_TIMEOUT_MS 60000   /*Time limit for connecting WiFi after ESP-TOUCH figured out the SSID&PWD*/
    #define ESP_TOUCH_TIME_ENTER  20000         /*Time limit for ESP-TOUCH to receive config packets*/
    #define ESP_TOUCH_TIMEOUT_MS 120000         /*Total time limit for ESP-TOUCH*/
	#define ESP_TOUCH_TIME_LIMIT  2             /*Get into ESPTOUCH MODE only twice at most, then scan mesh if not connected*/
#endif




#if SENSOR_DEVICE
#define HUMITURE_SUB_DEVICE         1
#define FLAMMABLE_GAS_SUB_DEVICE    0
#endif

//#define SERVER_SSL_ENABLE
//#define CLIENT_SSL_ENABLE
//#define UPGRADE_SSL_ENABLE


//#define USE_DNS
#ifdef USE_DNS
#define ESP_DOMAIN      "iot.espressif.cn"
#endif

#if ESP_MESH_SUPPORT
    #define ESP_SERVER_PORT  (MESH_SERVER_PORT)
	const static uint8 esp_server_ip[4] = {115,29,202,58};
#else
    #define ESP_SERVER_PORT 8000
	const static uint8 esp_server_ip[4] = {115,29,202,58};
#endif




//#define SOFTAP_ENCRYPT
#ifdef SOFTAP_ENCRYPT
#define PASSWORD	"v*%W>L<@i&Nxe!"
#endif

#if SENSOR_DEVICE
#define SENSOR_DEEP_SLEEP

#if HUMITURE_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    30000000
#elif FLAMMABLE_GAS_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    60000000
#endif
#endif

#if LIGHT_DEVICE
#define USE_US_TIMER
#endif

#define AP_CACHE           0

#if AP_CACHE
#define AP_CACHE_NUMBER    5
#endif

#endif

#endif

