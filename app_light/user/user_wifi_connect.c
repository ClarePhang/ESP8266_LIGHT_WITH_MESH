#include "user_config.h"
#include "user_wifi_connect.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
#include "user_config.h"
#include "user_esp_platform.h"
#if ESP_MESH_SUPPORT
#include "mesh.h"
#endif
#define INFO os_printf

static ETSTimer WiFiLinker;
WifiCallback wifiCb = NULL;
static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;
static bool wifiReconFlg = false;
static bool wifiReconProcessing = false;

void ICACHE_FLASH_ATTR WIFI_StopCheckIp();
void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb);

os_timer_t mesh_scan_t;
#include "user_light_mesh.h"

bool ICACHE_FLASH_ATTR
wifi_GetReconnProcessFlg()
{
    return wifiReconProcessing;
}

void ICACHE_FLASH_ATTR
wifi_ClrReconnProcessFlg()
{
    wifiReconProcessing = false;
}

void ICACHE_FLASH_ATTR
wifi_SetReconnProcessFlg()
{
    wifiReconProcessing = true;
}

#if ESP_MESH_SUPPORT
//restart wifi scan in (t) ms
void ICACHE_FLASH_ATTR
wifi_RestartMeshScan(uint32 t)
{
	INFO("RE-SEARCH MESH NETWORK,in %ds \r\n",t/1000);
	os_timer_disarm(&mesh_scan_t);
	os_timer_setfn(&mesh_scan_t,user_MeshStart,NULL);
	os_timer_arm(&mesh_scan_t,t,0);
}

#endif

//callback function after connected to the router
void ICACHE_FLASH_ATTR
    WIFI_StatusHandler(uint8_t status)
{    
    if(status == STATION_GOT_IP){
		//connected,
        #if ESP_TOUCH_SUPPORT
        esptouch_setAckFlag(true);
        #endif
        INFO("WIFI CONNECTED , RUN ESP PLATFORM...\r\n");
        user_esp_platform_connect_ap_cb();
    }else{
        //not connected, restart mesh scan in 10 seconds
        if(status != STATION_CONNECTING){
            INFO("---------------------\r\n");
            INFO("STATION STATUS: %d \r\n",status);
			INFO("ENABLE MESH.\r\n");
            INFO("---------------------\r\n");
            wifiReconFlg = false;
			wifi_SetReconnProcessFlg();
			#if ESP_MESH_SUPPORT
			wifi_RestartMeshScan(10000);
			#endif
        }
    }
}


//check station ip to see if we have connected to the target router
//if status changed,run callback(WIFI_StatusHandler)
static void ICACHE_FLASH_ATTR WIFI_CheckIp(void *arg)
{
    os_timer_disarm(&WiFiLinker);
    #if ESP_MESH_SUPPORT
        if(MESH_DISABLE != espconn_mesh_get_status()){
            /*MESH layer would handle wifi status exception at first*/
            os_timer_setfn(&WiFiLinker, (os_timer_func_t *)WIFI_CheckIp, NULL);
            os_timer_arm(&WiFiLinker, 1000, 0);
            return;
        }else{
            if(wifi_GetReconnProcessFlg() == false) wifiReconFlg = true;
            INFO("wifiReconFlg : %d \r\n",wifiReconFlg);
			INFO("wifiReconAck:  %d \r\n",wifi_GetReconnProcessFlg());
            INFO("MESH STATUS : %d \r\n",espconn_mesh_get_status());
            INFO("WIFI STATUS : CUR:%d ; LAST:%d\r\n",wifiStatus,lastWifiStatus);
            INFO("----------------\r\n");
        }
    #endif
    struct ip_info ipConfig;
    wifi_get_ip_info(STATION_IF, &ipConfig);
    wifiStatus = wifi_station_get_connect_status();
    if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0) {
		wifi_ClrReconnProcessFlg();
        INFO("wifiReconAck SET : %d \r\n",wifi_GetReconnProcessFlg());
        os_timer_arm(&WiFiLinker, 2000, 0);
    }
     else {
        if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD) {
            INFO("----------------\r\n");
            INFO("STATION_WRONG_PASSWORD\r\n");
            //wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_NO_AP_FOUND) {
            INFO("----------------\r\n");
            INFO("STATION_NO_AP_FOUND\r\n");
            //wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_CONNECT_FAIL) {
            INFO("----------------\r\n");
            INFO("STATION_CONNECT_FAIL\r\n");
            //wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_CONNECTING) {
            INFO("----------------\r\n");
            INFO("STATION_CONNECTING\r\n");
            //wifi_station_connect();
        }
        else if(wifi_station_get_connect_status() == STATION_IDLE) {
            INFO("----------------\r\n");
            INFO("STATION_IDLE\r\n");
            //INFO("TEST STATION STATUS: %d \r\n",wifi_station_get_connect_status());
        }else{
            INFO("STATUS ERROR\r\n");
        }
        os_timer_setfn(&WiFiLinker, (os_timer_func_t *)WIFI_CheckIp, NULL);
        os_timer_arm(&WiFiLinker, 1000, 0);
    }
    if((wifiStatus != lastWifiStatus) || wifiReconFlg){
        lastWifiStatus = wifiStatus;
        if(wifiCb){
            wifiCb(wifiStatus);
        }else{
            WIFI_StatusHandler(wifiStatus);
        }
    }
}

//connect to the router with given ssid and password
void ICACHE_FLASH_ATTR 
    WIFI_Connect(uint8_t* ssid, uint8_t* password, WifiCallback cb)
{
    struct station_config stationConf;
    INFO("WIFI_INIT\r\n");
    wifi_set_opmode(STATIONAP_MODE);//
    if(cb){
        wifiCb = cb;
    }else{
        wifiCb = WIFI_StatusHandler;
    }
    os_memset(&stationConf, 0, sizeof(struct station_config));
    os_sprintf(stationConf.ssid, "%s", ssid);
    os_sprintf(stationConf.password, "%s", password);
    wifi_station_set_config(&stationConf);
    os_timer_disarm(&WiFiLinker);
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)WIFI_CheckIp, NULL);
    os_timer_arm(&WiFiLinker, 1000, 0);
    wifi_station_connect();
}

//stop check connecting check timer
void ICACHE_FLASH_ATTR
    WIFI_StopCheckIp()
{
    os_timer_disarm(&WiFiLinker);
}

//start check connecting check
void ICACHE_FLASH_ATTR
    WIFI_StartCheckIp()
{
    lastWifiStatus = wifiStatus;
    os_timer_disarm(&WiFiLinker);
    os_timer_setfn(&WiFiLinker, (os_timer_func_t *)WIFI_CheckIp, NULL);
    WIFI_CheckIp(NULL);
}

