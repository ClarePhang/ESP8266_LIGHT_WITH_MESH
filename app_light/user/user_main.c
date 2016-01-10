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

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"
#include "user_light.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif
#if ESP_DEBUG_MODE
#include "user_debug.h"
#endif

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
#include "user_config.h"


#if 1

#if ESP_WEB_SUPPORT //not supported yet
#include "httpd.h"
#include "httpdespfs.h"
#include "cgiwifi.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "user_simplepair.h"
#include "user_cgi.h"
HttpdBuiltInUrl builtInUrls[]={
    {"*", cgiRedirectApClientToHostname, "light.nonet"},
    {"/", cgiRedirect, "/index.html"},
    {"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
    {"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
    {"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
    {"/wifi/connect.cgi", cgiWiFiConnect, NULL},
    {"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
    {"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},
    //Hook up original API
    {"/config", cgiEspApi, NULL},
    {"/client", cgiEspApi, NULL},
    {"/upgrade", cgiEspApi, NULL},
    {"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
    {NULL, NULL, NULL} //end marker
};
#endif

os_timer_t disp_t;
void ICACHE_FLASH_ATTR
    disp_heap(int idx)
{
    os_printf("heap: %d \r\n",system_get_free_heap_size());
}

os_timer_t reset_flg_t;
void user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR
    user_DispAppInfo()
{
    os_printf("SDK : %s \r\n",system_get_sdk_version());
    os_printf("******************************\r\n");
    os_printf("**  ESP_MESH_SUPPORT:  %d   **\r\n",ESP_MESH_SUPPORT);
    os_printf("**  ESP_NOW_SUPPORT:   %d   **\r\n",ESP_NOW_SUPPORT);
    os_printf("**  ESP_TOUCH_SUPPORT: %d   **\r\n",ESP_TOUCH_SUPPORT);
    os_printf("**  ESP_WEB_SUPPORT:   %d   **\r\n",ESP_WEB_SUPPORT);
    os_printf("**  ESP_PWM_VERSION:   %d   **\r\n",get_pwm_version());
    os_printf("**  ESP_MDNS_SUPPORT:  %d   **\r\n",ESP_MDNS_SUPPORT);
	os_printf("**  ESP_MESH_DEBUG_VER: %d  **\r\n",MESH_TEST_VERSION);
    os_printf("******************************\r\n\n\n");
}



/*The main working flow of this demo:
  a. start mesh function, searching for existing mesh network
  b. if there is no available mesh network, search AP cache to connect to a recorded router.
  c. If we still failed to establish a connection, start ESP-TOUCH waiting for configure.
  d. If ESP-TOUCH time out, re-search mesh network.
  e. During the whole procedure,we can control and configure the light via a restful webserver.
  f. ESP-NOW is the recently released function to control the light without Wi-Fi connection.You can find it in app_controller
*/
void ICACHE_FLASH_ATTR
    light_main_flow()
{

    UART_WaitTxFifoEmpty(0,50000);
    //uart_init(74880,74880);
    user_DispAppInfo();

    wifi_set_opmode(STATIONAP_MODE);
#if ESP_MESH_SUPPORT
    wifi_station_set_auto_connect(0);
    wifi_station_disconnect();
#else
    wifi_station_set_auto_connect(1);
#endif
    os_printf("SDK version:%s\n", system_get_sdk_version());

#if AP_CACHE
    wifi_station_ap_number_set(AP_CACHE_NUMBER);
#endif

#if ESP_NOW_SUPPORT
    /*We have added esp-now feature in the light project */
    /*So that the lights in a certain MAC group can be easily controlled by an ESP-NOW controller*/
    /*The sample code is in APP_CONTROLLER(must work with the reference button cercuit designed by Espressif)*/
    sp_MacInit();//add button Mac add and delet
    light_EspnowInit();
#endif

#if 1
#if ESP_PLATFORM
    /*Initialization of the peripheral drivers*/
    /*For light demo , it is user_light_init();*/
    user_esp_platform_init_peripheral();

#endif
    /*Establish a udp socket to receive local device detect info.*/
    /*Listen to the port 1025, as well as udp broadcast.
    /*If receive a string of device_find_request, it reply its IP address and MAC.*/
    user_devicefind_init();
    /*MESH INTERFACE IS AT PORT 8000*/
#if ESP_WEB_SUPPORT
    //Initialize DNS server for captive portal
    captdnsInit();
    //Initialize espfs containing static webpages
    espFsInit((void*)(webpages_espfs_start));
    //Initialize webserver
    httpdInit(builtInUrls, SERVER_PORT);
    //user_webserver_init(SERVER_PORT);
#elif (ESP_MESH_SUPPORT==0)
    #ifdef SERVER_SSL_ENABLE
    user_webserver_init(SERVER_SSL_PORT);
    #else
    user_webserver_init(SERVER_PORT);
    #endif
#endif

#endif

//In debug mode, if you restart the light within 2 seconds, it will get into softap mode and wait for local upgrading firmware.
//Restart again within 2 seconds, it will clear the system param and set to default status.
#if ESP_DEBUG_MODE
    extern struct esp_platform_saved_param esp_param;
    if(esp_param.reset_flg == MODE_APMODE){
        os_printf("==================\r\n");
        os_printf("RESET FLG==2,STATIONAP_MODE \r\n");
        os_printf("==================\r\n");
		DEBUG_FLG = true;
        #if ESP_MESH_SUPPORT
        user_DeviceFindRespSet(false);
        #endif
        
        struct softap_config config_softap;
        char ssid[33]={0};
        
        wifi_softap_get_config(&config_softap);
        os_memset(config_softap.password, 0, sizeof(config_softap.password));
        os_memset(config_softap.ssid, 0, sizeof(config_softap.ssid));
        os_sprintf(ssid,"ESP_%06X",system_get_chip_id());
        os_memcpy(config_softap.ssid, ssid, os_strlen(ssid));
        config_softap.ssid_len = os_strlen(ssid);
        config_softap.authmode = AUTH_OPEN;
        wifi_softap_set_config(&config_softap);
        
        os_printf("SET STATION-AP MODE\r\n");
        wifi_set_opmode(STATIONAP_MODE);
        user_esp_platform_set_reset_flg(MODE_RESET);

        
        #if ESP_WEB_SUPPORT
		    os_printf("WEBSERVER INIT...");
            //Initialize DNS server for captive portal
            captdnsInit();
            //Initialize espfs containing static webpages
            espFsInit((void*)(webpages_espfs_start));
            //Initialize webserver
            httpdInit(builtInUrls, SERVER_PORT);
		#else
		    user_webserver_init(SERVER_PORT);		
        #endif

        os_timer_disarm(&reset_flg_t);
        os_timer_setfn(&reset_flg_t,user_esp_platform_set_reset_flg,MODE_NORMAL);
        os_timer_arm(&reset_flg_t,2000,0);
        user_light_set_duty(0, LIGHT_RED);
        user_light_set_duty(0, LIGHT_GREEN);
        user_light_set_duty(0, LIGHT_BLUE);
        user_light_set_duty(22222, LIGHT_WARM_WHITE);
        user_light_set_duty(22222, LIGHT_COLD_WHITE);
        os_delay_us(5000);
        light_ShowDevLevel(2,1000);
        return;
    }
    
    #if ESP_RESET_DEBUG_EN
    else if(esp_param.reset_flg == MODE_RESET){
        os_printf("==================\r\n");
        os_printf("RESET FLG==1,RESET IN 200 MS \r\n");
        os_printf("==================\r\n");
        user_esp_platform_set_reset_flg(MODE_APMODE);
        os_timer_disarm(&reset_flg_t);
        os_timer_setfn(&reset_flg_t,user_esp_platform_reset_default,0);
        os_timer_arm(&reset_flg_t,200,0);
    }
    else{
        os_printf("==================\r\n");
        os_printf("RESET FLG==0,NORMAL MODE \r\n");
        os_printf("==================\r\n");
        user_esp_platform_set_reset_flg(MODE_APMODE);
        os_timer_disarm(&reset_flg_t);
        os_timer_setfn(&reset_flg_t,user_esp_platform_set_reset_flg,0);
        os_timer_arm(&reset_flg_t,2000,0);
    }
    #endif
#endif

#if ESP_MESH_SUPPORT
    /*initialize mesh network.
      1. search for existing mesh.
      2. if failed , try connecting recorded router.
    */
    user_MeshParamInit();
    user_MeshStart(0);
#elif ESP_TOUCH_SUPPORT
    esptouch_FlowStart();
#endif

    #if ESP_DEBUG_MODE
    os_timer_disarm(&disp_t);
    os_timer_setfn(&disp_t,disp_heap,NULL);
	os_timer_arm(&disp_t,1000,1);
	#endif
}

#endif

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    system_init_done_cb(light_main_flow);
}

