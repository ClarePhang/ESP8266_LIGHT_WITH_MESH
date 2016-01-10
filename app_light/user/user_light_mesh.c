#include "user_light_mesh.h"
#include "user_light_hint.h"
#include "user_interface.h"

#if ESP_MESH_SUPPORT
#include "mesh.h"
#include "user_espnow_action.h"
LIGHT_MESH_PROC LightMeshProc;
os_timer_t mesh_check_t;
os_timer_t mesh_tout_t;
os_timer_t mesh_user_t;

uint8 mesh_last_status=MESH_DISABLE;
LOCAL bool mesh_init_flag = true;
LOCAL char* mdev_mac = NULL;
uint8 mesh_src_addr[ESP_MESH_ADDR_LEN];


#if STRING_SV_IN_FLASH
const static char BCAST_CMD[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"mdev_mac\": \"000000000000\", \"period\": %d, \"rgb\": {\"red\": %d, \"green\": %d, \"blue\": %d, \"cwhite\": %d, \"wwhite\": %d},\"response\":%d}\r\n";
const static char UCAST_CMD[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"mdev_mac\": \"%02x%02x%02x%02x%02x%02x\", \"period\": %d, \"rgb\": {\"red\": %d, \"green\": %d, \"blue\": %d, \"cwhite\": %d, \"wwhite\": %d},\"response\":%d}\r\n";
const static char LIGHT_CMD_URL[] ICACHE_RODATA_ATTR STORE_ATTR = "/config?command=light";
const static char UPGRADE_BROADCAST_CMD[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"nonce\": %lu, \"mdev_mac\": \"000000000000\", \"get\": {\"action\": \"sys_upgrade\", \"version\": \"%s\"}, \"meta\": {\"Authorization\": \"token d94c149739e985c3d40b06381444fd807fe63f29\", \"Time-Zone\": \"Asia/Kashgar\"}, \"path\": \"/v1/device/rpc/\", \"method\": \"GET\", \"deliver_to_device\": true}\r\n";
const static char MDEV_MAC_STR[] ICACHE_RODATA_ATTR STORE_ATTR = "\"mdev_mac\":\"%02X%02X%02X%02X%02X%02X\"";

#else
#define BCAST_CMD "{\"mdev_mac\": \"000000000000\", \"period\": %d, \"rgb\": {\"red\": %d, \"green\": %d, \"blue\": %d, \"cwhite\": %d, \"wwhite\": %d},\"response\":%d}\r\n"
#define UCAST_CMD "{\"mdev_mac\": \"%02x%02x%02x%02x%02x%02x\", \"period\": %d, \"rgb\": {\"red\": %d, \"green\": %d, \"blue\": %d, \"cwhite\": %d, \"wwhite\": %d},\"response\":%d}\r\n"
#define LIGHT_CMD_URL "/config?command=light"
#define UPGRADE_BROADCAST_CMD "{\"nonce\": %lu, \"mdev_mac\": \"000000000000\", \"get\": {\"action\": \"sys_upgrade\", \"version\": \"%s\"}, \"meta\": {\"Authorization\": \"token d94c149739e985c3d40b06381444fd807fe63f29\", \"Time-Zone\": \"Asia/Kashgar\"}, \"path\": \"/v1/device/rpc/\", \"method\": \"GET\", \"deliver_to_device\": true}\r\n"
#define MDEV_MAC_STR "\"mdev_mac\":\"%02X%02X%02X%02X%02X%02X\""
#endif



void ICACHE_FLASH_ATTR
mesh_MacIdInit()
{
    if(mdev_mac){
        MESH_INFO("Mesh mdev_mac: %s \r\n",mdev_mac);
        return;
    }
    mdev_mac = (char*)os_zalloc(ESP_MESH_JSON_DEV_MAC_ELEM_LEN+1);	
    uint8 mac_sta[6] = {0};
    wifi_get_macaddr(STATION_IF, mac_sta);
	#if STRING_SV_IN_FLASH
    char mdev_mac_str[50];
	load_string_from_flash(mdev_mac_str,sizeof(mdev_mac_str),(void*)MDEV_MAC_STR);
    os_sprintf(mdev_mac,mdev_mac_str,MAC2STR(mac_sta));	
	#else
    os_sprintf(mdev_mac,MDEV_MAC_STR,MAC2STR(mac_sta));
	#endif
    MESH_INFO("Disp mdev_mac: %s\r\n",mdev_mac);
    os_memcpy(mesh_src_addr,mac_sta,ESP_MESH_ADDR_LEN);
}

char* ICACHE_FLASH_ATTR
mesh_GetMdevMac()
{
    if(mdev_mac){
        return mdev_mac;
    }else{
        mesh_MacIdInit();
        return mdev_mac;
    }
}

/******************************************************************************
 * FunctionName : mesh_StopCheckTimer
 * Description  : Stop the mesh initialization status check
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_StopCheckTimer()
{
    os_timer_disarm(&mesh_check_t);
    os_timer_disarm(&mesh_tout_t);
}


/******************************************************************************
 * FunctionName : mesh_SetSoftap
 * Description  : If the device failed to join mesh network,
                  open the SoftAP interface for webserver
                  The SSID should not be the same form as that of the device in mesh network
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_SetSoftap()
{
    MESH_INFO("----------------------\r\n");
    MESH_INFO("MESH ENABLE SOFTAP \r\n");
    MESH_INFO("----------------------\r\n");
    struct softap_config config_softap;
    char ssid[33]={0};
    wifi_softap_get_config(&config_softap);
    os_memset(config_softap.password, 0, sizeof(config_softap.password));
    os_memset(config_softap.ssid, 0, sizeof(config_softap.ssid));
    os_sprintf(ssid,"ESP_%06X",system_get_chip_id());
    os_memcpy(config_softap.ssid, ssid, os_strlen(ssid));
    config_softap.ssid_len = os_strlen(ssid);
    config_softap.ssid_hidden = 0;
    config_softap.channel = wifi_get_channel();
    #ifdef SOFTAP_ENCRYPT
        char password[33];
        char macaddr[6];
        os_sprintf(password, MACSTR "_%s", MAC2STR(macaddr), PASSWORD);
        os_memcpy(config_softap.password, password, os_strlen(password));
        config_softap.authmode = AUTH_WPA_WPA2_PSK;
    #else
        os_memset(config_softap.password,0,sizeof(config_softap.password));
        config_softap.authmode = AUTH_OPEN;
    #endif
    wifi_set_opmode(STATIONAP_MODE);
    wifi_softap_set_config(&config_softap);
    wifi_set_opmode(STATIONAP_MODE);
    wifi_softap_get_config(&config_softap);
    MESH_INFO("SSID: %s \r\n",config_softap.ssid);
    MESH_INFO("CHANNEL: %d \r\n",config_softap.channel);
    MESH_INFO("-------------------------\r\n");
}

/******************************************************************************
 * FunctionName : mesh_EnableCb
 * Description  : callback func for espconn_mesh_enable
                  enable callback will only be called if the device has already joined a mesh network
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_EnableCb(sint8 status)
{
    if(status == MESH_ONLINE_SUC){
		#if ESP_DEBUG_MODE
        if(mesh_root_if()){
            light_set_color(0,0,0,22222,22222,1000);
        }
		#endif
        MESH_INFO("debug: mesh_EnableCb: %d \r\n",status);
    	espSendSetStatus(ESP_SERVER_ACTIVE);
        mesh_StopCheckTimer();
        MESH_INFO("TEST IN MESH ENABLE CB\r\n");
        MESH_INFO("%s\n", __func__);
        MESH_INFO("HEAP: %d \r\n",system_get_free_heap_size());
        if(LightMeshProc.mesh_suc_cb){
            LightMeshProc.mesh_suc_cb(NULL);
        }
    }
}


/******************************************************************************
 * FunctionName : mesh_DisableCb
 * Description  : callback func when mesh is disabled
                  In this case, do nothing but display some info.
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_DisableCb(sint8 status)
{
    MESH_INFO("TEST IN MESH DISABLE CB,status: %d\r\n",status);
    MESH_INFO("%s\n", __func__);
    MESH_INFO("HEAP: %d \r\n",system_get_free_heap_size());
}

/******************************************************************************
 * FunctionName : mesh_SuccessCb
 * Description  : callback func when mesh init finished successfully
                  In this demo , we run the platform code after mesh initialization
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_SuccessCb(void* arg)
{
    MESH_INFO("mesh log: mesh success!\r\n");
    MESH_INFO("CONNECTED, DO RUN ESP PLATFORM...\r\n");
    MESH_INFO("mesh status: %d\r\n",espconn_mesh_get_status());
    
    //show light status at the first time mesh enables
    #if ESP_DEBUG_MODE
    	struct ip_info sta_ip;
		wifi_get_ip_info(STATION_IF,&sta_ip);
		if( espconn_mesh_local_addr(&sta_ip.ip)){
			MESH_INFO("THIS IS A MESH SUB NODE..\r\n");
			uint32 mlevel = sta_ip.ip.addr&0xff;
			light_ShowDevLevel(mlevel,5000);//debug
		}else{
			MESH_INFO("THIS IS A MESH ROOT..\r\n");
			light_ShowDevLevel(1,5000);//debug
		}
	#else
    if(mesh_init_flag){
        struct ip_info sta_ip;
        wifi_get_ip_info(STATION_IF,&sta_ip);
        if( espconn_mesh_local_addr(&sta_ip.ip)){
            MESH_INFO("THIS IS A MESH SUB NODE..\r\n");
            uint32 mlevel = sta_ip.ip.addr&0xff;
            light_ShowDevLevel(mlevel,5000);//debug
        }else{
            MESH_INFO("THIS IS A MESH ROOT..\r\n");
            light_ShowDevLevel(1,5000);//debug
        }
        mesh_init_flag = false;
    }else{
    
    }
	#endif
    
    #if ESP_NOW_SUPPORT
        //init ESP-NOW ,so that light can be controlled by ESP-NOW SWITCHER.
        light_EspnowInit();
    #endif
    WIFI_StartCheckIp();//debug
    //run esp-platform procedure,register to server.
    user_esp_platform_connect_ap_cb();//debug
    return;
}

/******************************************************************************
 * FunctionName : mesh_FailCb
 * Description  : callback func when mesh init failed(both timeout and failed)
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_FailCb(void* arg)
{
    MESH_INFO("mesh fail\r\n");
    mesh_StopCheckTimer();
    #if ESP_NOW_SUPPORT
        //initialize ESP-NOW
        light_EspnowInit();
    #endif
    MESH_INFO("CALL MESH DISABLE HERE...\r\n");
    //stop or disable mesh
    espconn_mesh_disable(mesh_DisableCb);
    //start Esptouch
    #if ESP_TOUCH_SUPPORT
    if(false == esptouch_getAckFlag()){
        esptouch_FlowStart();
        return;
    }
	#endif
	wifi_RestartMeshScan(10000);
    return;
}

/******************************************************************************
 * FunctionName : mesh_TimeoutCb
 * Description  : callback func when mesh init timeout
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_TimeoutCb(void* arg)
{
    if(espconn_mesh_get_status() == MESH_DISABLE || espconn_mesh_get_status() == MESH_WIFI_CONN){
        MESH_INFO("MESH INIT TIMEOUT, STOP MESH\r\n");
        mesh_FailCb(NULL);
    }else{
        MESH_INFO("mesh connecting\r\n");
    }
    return;
}

/******************************************************************************
 * FunctionName : mesh_GetStartMs
 * Description  : Get the time expire since the beginning of mesh initialization
                  Count in Ms
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
mesh_GetStartMs()
{
    return (system_get_time()-LightMeshProc.start_time)/1000;
}

/******************************************************************************
 * FunctionName : mesh_InitStatusCheck
 * Description  : Only used in mesh init, to check the current status of mesh initialization,
                  and handle different situation accordingly
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_InitStatusCheck()
{
    os_timer_disarm(&mesh_check_t);
    sint8 mesh_status = espconn_mesh_get_status();
    MESH_INFO("--------------\r\n");
    MESH_INFO("mesh status: %d ; %d\r\n",mesh_status,system_get_free_heap_size());
    MESH_INFO("--------------\r\n");
    
    if(mesh_status == MESH_DISABLE){
        MESH_INFO("MESH DISABLE , RUN FAIL CB ,retry:%d \r\n",LightMeshProc.init_retry);
        if(LightMeshProc.init_retry<MESH_INIT_RETRY_LIMIT && (mesh_GetStartMs()<MESH_INIT_TIME_LIMIT)){
            LightMeshProc.init_retry+=1;
            espconn_mesh_enable(mesh_EnableCb, MESH_ONLINE);
            #if ESP_DEBUG_MODE
			light_shadeStart(HINT_YELLOW,500,6,2,NULL);
            #endif
            MESH_INFO("MESH RETRY : %d \r\n",LightMeshProc.init_retry);
        }else{
            mesh_StopCheckTimer();
            MESH_INFO("MESH INIT RETRY FAIL...\r\n");
            if(LightMeshProc.mesh_fail_cb){
                LightMeshProc.mesh_fail_cb(NULL);
            }
            LightMeshProc.init_retry = 0;
            return;
        }
    }
    else if(mesh_status==MESH_NET_CONN){
        MESH_INFO("MESH WIFI CONNECTED\r\n");
        mesh_StopCheckTimer();
    }
	
    os_timer_arm(&mesh_check_t,MESH_STATUS_CHECK_MS,0);
}

/******************************************************************************
 * FunctionName : mesh_ReconCheck
 * Description  : in case that some router would record the DNS info.
                  If we got the IP addr from DNS , and still can't connect to esp-server
                  Call this check func to try enable and connect again every 20s
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_ReconCheck()
{
    MESH_INFO("---------\r\n");
    MESH_INFO("mesh_ReconCheck\r\n");
    MESH_INFO("---------\r\n");
    os_timer_disarm(&mesh_user_t);
    sint8 mesh_status = espconn_mesh_get_status();
    MESH_INFO("MESH STATUS CHECK: %d \r\n",mesh_status);
    
    if(mesh_status == MESH_ONLINE_AVAIL){
        MESH_INFO("MESH ONLINE AVAIL\r\n");
        //user_esp_platform_sent_data();
        user_esp_platform_connect_ap_cb();
    }else{
        espconn_mesh_enable(mesh_EnableCb, MESH_ONLINE);
        #if ESP_DEBUG_MODE
		light_shadeStart(HINT_YELLOW,500,6,2,NULL);
        #endif
        os_timer_setfn(&mesh_user_t,mesh_ReconCheck,NULL);
        os_timer_arm(&mesh_user_t,20000,1);    
    }
}


/******************************************************************************
 * FunctionName : mesh_StopReconnCheck
 * Description  : If we receive a packet from the server,stop the check task
                  called in platform to restart mesh if can not get any response from esp-server.
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_StopReconnCheck()
{
    os_timer_disarm(&mesh_user_t);
}


/******************************************************************************
 * FunctionName : mesh_StartReconnCheck
 * Description  : start a timer to check mesh status
                  called in platform(dns found) to restart mesh if can not get any response from esp-server.
*******************************************************************************/
void ICACHE_FLASH_ATTR
mesh_StartReconnCheck(uint32 t)
{
    os_timer_disarm(&mesh_user_t);
    os_timer_setfn(&mesh_user_t,mesh_ReconCheck,NULL);
    os_timer_arm(&mesh_user_t,t,0);
}


//called when a sub-node joined in.
//Share the current status:light color/sending status/button key functions
void ICACHE_FLASH_ATTR
	mesh_NodeJoinCb(void* param)
{
    uint8* dev_mac = (uint8*)param;
    MESH_INFO("------------------\r\n");
	MESH_INFO("node joined: "MACSTR"\r\n",MAC2STR(dev_mac));
	MESH_INFO("------------------\r\n\n\n\n\n");	
	user_esp_platform_update_sending_status((struct espconn*)user_GetUserPConn(),espSendGetStatus(),MESH_UNICAST,dev_mac);
	user_esp_platform_button_function_status((struct espconn*)user_GetUserPConn(),dev_mac,MESH_UNICAST);
	light_SendMeshUnicastCmd(light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
		                     light_param_target.pwm_duty[3],light_param_target.pwm_duty[4],light_param_target.pwm_period,dev_mac);
}

void ICACHE_FLASH_ATTR
	mesh_NodeConnReadyCb(void* param)
{
    //MESH_INFO("call mesh conn cb init...\r\n");
	//user_esp_platform_mesh_conn_init();//debug 151201
}

/******************************************************************************
 * FunctionName : user_MeshStart
 * Description  : mesh procedure init function
 * Parameters   : none
 * Returns      : none
 * Comments     :
                 Set callback function and start mesh.
                 MESH_ONLINE: CAN BE CONTROLLED BY WAN SERVER,AS WELL AS LAN APP VIA ROUTER.
                 MESH_LOCAL: CAN ONLY BE CONTROLLED BY LOCAL NETWORK.
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_MeshParamInit()
{
    user_MeshSetInfo();
    mesh_MacIdInit();
    LightMeshProc.mesh_suc_cb=mesh_SuccessCb;
    LightMeshProc.mesh_fail_cb=mesh_FailCb;
    LightMeshProc.mesh_init_tout_cb=mesh_TimeoutCb;
	LightMeshProc.mesh_node_join_cb = mesh_NodeJoinCb;
    LightMeshProc.start_time = system_get_time();
    LightMeshProc.init_retry = 0;
	
    //set max hops of mesh
    espconn_mesh_set_max_hops(MESH_MAX_HOP_NUM);
	MESH_INFO("===============\r\n");
	MESH_INFO("debug: REGISTER usr cb\r\n");
	MESH_INFO("===============\r\n\r\n");
	espconn_mesh_regist_usr_cb(mesh_NodeJoinCb);
	espconn_mesh_regist_conn_ready_cb(mesh_NodeConnReadyCb);

	#if MESH_SERVER_CONF
    	ip_addr_t ipaddr;
    	os_memcpy((uint8*)&(ipaddr.addr),esp_server_ip,4);
		MESH_INFO("SERVER IP: %d.%d.%d.%d  ;; %d.%d.%d.%d\r\n",IP2STR(&ipaddr),IP2STR(esp_server_ip));
    	if( espconn_mesh_server_init(&ipaddr, ESP_SERVER_PORT))
    		MESH_INFO("SERVER INIT OK...\r\n");
    	else
    		MESH_INFO("SERVER INIT FAIL...\r\n");
    #endif
}


//mesh scan start function
void ICACHE_FLASH_ATTR
user_MeshStart(void* arg)
{
    MESH_INFO("test: %s\n", __func__);
    //user_MeshSetInfo();
    espconn_mesh_enable(mesh_EnableCb, MESH_ONLINE);
    #if ESP_DEBUG_MODE
	MESH_INFO("MESH START SHADE!!!\r\n");
	light_shadeStart(HINT_YELLOW,500,6,2,NULL);
    #endif
	
    wifi_ClrReconnProcessFlg();
	#if MESH_INIT_TIMEOUT_SET
	if(arg){
		MESH_INFO("start timeout count.\r\n");
        if(LightMeshProc.mesh_init_tout_cb){
            os_timer_disarm(&mesh_tout_t);
            os_timer_setfn(&mesh_tout_t,LightMeshProc.mesh_init_tout_cb,NULL);
            os_timer_arm(&mesh_tout_t,MESH_TIME_OUT_MS,0);
        }
	}
	#endif
    os_timer_disarm(&mesh_check_t);
    os_timer_setfn(&mesh_check_t,mesh_InitStatusCheck,NULL);
    os_timer_arm(&mesh_check_t,MESH_STATUS_CHECK_MS,0);

}

/******************************************************************************
 * FunctionName : user_MeshSetInfo
 * Description  : set mesh node SSID,AUTH_MODE and PASSWORD
 * Parameters   : none
 * Returns      : none
 * Comments     : In this initial version, the MESH device is grouped by the specific SSID
				  Users can change the SSID and PASSWORD for SoftAP interface for the mesh nodes
 * NOTE         : Only call it once and before espconn_mesh_enable() 
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_MeshSetInfo()
{
    #if MESH_SET_GROUP_ID  //set mesh group id
        espconn_mesh_group_id_init((uint8*)MESH_GROUP_ID,6);
        MESH_INFO("==========================\r\n");
        MESH_INFO("SET GROUP ID: "MACSTR"",MAC2STR(MESH_GROUP_ID));
        MESH_INFO("==========================\r\n");
    #endif	
    
    if( espconn_mesh_set_ssid_prefix(MESH_SSID_PREFIX,os_strlen(MESH_SSID_PREFIX))){
        MESH_INFO("SSID PREFIX SET OK..\r\n");
    }else{
        MESH_INFO("SSID PREFIX SET ERROR..\r\n");
    }
    
    if(espconn_mesh_encrypt_init(MESH_AUTH,MESH_PASSWORD,os_strlen(MESH_PASSWORD))){
        MESH_INFO("SOFTAP ENCRYPTION SET OK..\r\n");
    }else{
        MESH_INFO("SOFTAP ENCRYPTION SET ERROR..\r\n");
    }
}

//send user sending status 
void ICACHE_FLASH_ATTR
	light_SendMeshBroadcastUpgrade(uint32 nonce,char* version,int pkt_len)
{
    struct espconn* pconn = (struct espconn*)user_GetUserPConn();

	uint8* pkt_upgrade = (uint8*)os_zalloc(pkt_len);
	if(pkt_upgrade == NULL) return;

	#if STRING_SV_IN_FLASH
    char upgrade_broadcast_cmd[300];
	load_string_from_flash(upgrade_broadcast_cmd,sizeof(upgrade_broadcast_cmd),(void*)UPGRADE_BROADCAST_CMD);
	os_sprintf(pkt_upgrade,upgrade_broadcast_cmd,nonce,version);	
	#else
	os_sprintf(pkt_upgrade,UPGRADE_BROADCAST_CMD,nonce,version);
	#endif
	
    #if MESH_HEADER_MODE
		uint8 dst[6],src[6];
		os_memset(dst,0,6);
		wifi_get_macaddr(STATION_IF,src);		
    #endif

	user_JsonDataSend(pconn, pkt_upgrade, os_strlen(pkt_upgrade),0,src,dst);
	os_free(pkt_upgrade);
	pkt_upgrade = NULL;
}


//send broadcast command in mesh
void ICACHE_FLASH_ATTR
	light_SendMeshBroadcastCmd(uint32 r,uint32 g,uint32 b,uint32 cw,uint32 ww,uint32 period)
{
    struct espconn* pconn = (struct espconn*)user_GetUserPConn();
	char* bcast_data = (char*)os_zalloc(300);
	if(bcast_data==NULL) return;
    #if STRING_SV_IN_FLASH
    char pbuf_tmp[300];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)BCAST_CMD);
	os_sprintf(bcast_data,pbuf_tmp,period,r,g,b,cw,ww,0);	
	#else
	os_sprintf(bcast_data,BCAST_CMD,period,r,g,b,cw,ww,0);
	#endif
	
    #if MESH_HEADER_MODE
		uint8 dst[6],src[6];
		os_memset(dst,0,6);
		wifi_get_macaddr(STATION_IF,src);		
    #endif

	#if STRING_SV_IN_FLASH
    char light_cmd_url[100];
	load_string_from_flash(light_cmd_url,sizeof(light_cmd_url),(void*)LIGHT_CMD_URL);
	data_send_buf(pconn, true, bcast_data,os_strlen(bcast_data),light_cmd_url,os_strlen(light_cmd_url),src,dst,false);	
    #else	
	data_send_buf(pconn, true, bcast_data,os_strlen(bcast_data),LIGHT_CMD_URL,os_strlen(LIGHT_CMD_URL),src,dst,false);
    #endif
	os_free(bcast_data);
	bcast_data = NULL;
}

//send unicast light command to sub-node dev
void ICACHE_FLASH_ATTR
	light_SendMeshUnicastCmd(uint32 r,uint32 g,uint32 b,uint32 cw,uint32 ww,uint32 period,char* dst_mac)
{
    struct espconn* pconn = (struct espconn*)user_GetUserPConn();
	char* ucast_data = (char*)os_zalloc(300);
	if(ucast_data==NULL) return;
	#if STRING_SV_IN_FLASH
	char pbuf_tmp[300];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)UCAST_CMD);
	os_sprintf(ucast_data,pbuf_tmp,MAC2STR(dst_mac),period,r,g,b,cw,ww,0);	
	#else
	os_sprintf(ucast_data,UCAST_CMD,MAC2STR(dst_mac),period,r,g,b,cw,ww,0);
	#endif
	
    #if MESH_HEADER_MODE
		uint8 src[6];
		wifi_get_macaddr(STATION_IF,src);		
    #endif

	#if STRING_SV_IN_FLASH
    char light_cmd_url[100];
	load_string_from_flash(light_cmd_url,sizeof(light_cmd_url),(void*)LIGHT_CMD_URL);
	data_send_buf(pconn, true, ucast_data,os_strlen(ucast_data),light_cmd_url,os_strlen(light_cmd_url),src,dst_mac,true);    
	#else
	data_send_buf(pconn, true, ucast_data,os_strlen(ucast_data),LIGHT_CMD_URL,os_strlen(LIGHT_CMD_URL),src,dst_mac,true);
    #endif
	os_free(ucast_data);
	ucast_data = NULL;
}
#endif

