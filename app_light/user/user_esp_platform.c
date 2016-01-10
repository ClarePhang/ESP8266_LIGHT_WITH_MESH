/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_esp_platform.c
 *
 * Description: The client mode configration.
 *              Check your hardware connection with the host while use this mode.
 *
 * Modification history:
 *     2014/5/09, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_esp_platform.h"
#include "user_iot_version.h"
#include "upgrade.h"
#include "esp_send.h"

#if ESP_MESH_SUPPORT
    #include "mesh.h"
	#include "user_light_mesh.h"
	#include "user_espnow_action.h"
#endif
#include "user_webserver.h"
#include "user_json.h"
#include "user_simplepair.h"


#if ESP_PLATFORM

#if ESP_DEBUG_MODE
#include "user_debug.h"
#endif

#if STRING_SV_IN_FLASH
const static char ACTIVE_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"nonce\": %u,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\",\"mode\":\"v\"}}\n";
const static char RESPONSE_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"status\": 200,\"nonce\": %u, \"datapoint\": {\"x\": %d,\"y\": %d,\"z\": %d,\"k\": %d,\"l\": %d},\"deliver_to_device\":true}\n";
const static char FIRST_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"nonce\": %u, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"mode\":\"v\",\"Authorization\": \"token %s\"}}\n";
const static char MESH_UPGRADE[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"nonce\": %u, \"get\": {\"action\": \"download_rom_base64\", \"version\": \"%s\", \"filename\": \"%s\", \"offset\": %d, \"size\": %d}, \"meta\": {\"Authorization\": \"token %s\"}, \"path\": \"/v1/device/rom/\", \"method\": \"GET\"}\r\n";
const static char UPGRADE_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n";
const static char BEACON_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n";
const static char RPC_RESPONSE_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true}\n";
const static char RPC_RESPONSE_ELEMENT[] ICACHE_RODATA_ATTR STORE_ATTR = "\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true";
const static char TIMER_FRAME[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n";
#if 1
#define pheadbuffer "Connection: keep-alive\r\n\
	Cache-Control: no-cache\r\n\
	User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
	Accept: */*\r\n\
	Authorization: token %s\r\n\
	Accept-Encoding: gzip,deflate,sdch\r\n\
	Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"
#endif
const static char OTA_URL[] ICACHE_RODATA_ATTR STORE_ATTR = "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nContent-Length: 23\r\nHost: "IPSTR":%d\r\n"pheadbuffer"\n";
const static char BCAST_MESH_SENDING_STATUS[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"mdev_mac\": \"000000000000\", \"mesh_send_sta\": %d , \"src_mac_t\":\"%02X%02X%02X%02X%02X%02X\", \"response\": 0 , \"src_ip_t\":\"%d.%d.%d.%d\"}\r\n";
const static char BCAST_URL[] ICACHE_RODATA_ATTR STORE_ATTR = "/config?command=meshSendStatus";
const static char UNICAST_MESH_SENDING_STATUS[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"mdev_mac\": \"%02X%02X%02X%02X%02X%02X\", \"src_mac\":\"%02X%02X%02X%02X%02X%02X\", \"mesh_send_sta\": %d , \"response\": 0, \"src_ip_t\":\"%d.%d.%d.%d\"}\r\n";
const static char PARENT_INFO[] ICACHE_RODATA_ATTR STORE_ATTR = "\"parent_mdev_mac\":\"%02X%02X%02X%02X%02X%02X\",\"parent_rssi\": %d,\"child_num\": %d";
const static char BUTTON_CONFIG[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"status\":200,\"path\":\"/device/button/configure\"}";


#else
#define ACTIVE_FRAME    "{\"nonce\": %u,\"path\": \"/v1/device/activate/\", \"method\": \"POST\", \"body\": {\"encrypt_method\": \"PLAIN\", \"token\": \"%s\", \"bssid\": \""MACSTR"\",\"rom_version\":\"%s\"}, \"meta\": {\"Authorization\": \"token %s\",\"mode\":\"v\"}}\n"
#define RESPONSE_FRAME  "{\"status\": 200,\"nonce\": %u, \"datapoint\": {\"x\": %d,\"y\": %d,\"z\": %d,\"k\": %d,\"l\": %d},\"deliver_to_device\":true}\n"
#define FIRST_FRAME     "{\"nonce\": %u, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"mode\":\"v\",\"Authorization\": \"token %s\"}}\n"
#define MESH_UPGRADE    "{\"nonce\": %u, \"get\": {\"action\": \"download_rom_base64\", \"version\": \"%s\", \"filename\": \"%s\", \"offset\": %d, \"size\": %d}, \"meta\": {\"Authorization\": \"token %s\"}, \"path\": \"/v1/device/rom/\", \"method\": \"GET\"}\r\n"
#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"
#define BEACON_FRAME    "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n"
#define RPC_RESPONSE_FRAME  "{\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true}\n"
#define RPC_RESPONSE_ELEMENT  "\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true"
#define TIMER_FRAME     "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n"
#define OTA_URL "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nContent-Length: 23\r\nHost: "IPSTR":%d\r\n\n"
#define BCAST_MESH_SENDING_STATUS "{\"mdev_mac\": \"000000000000\", \"mesh_send_sta\": %d , \"src_mac_t\":\"%02X%02X%02X%02X%02X%02X\", \"response\": 0 , \"src_ip_t\":\"%d.%d.%d.%d\"}\r\n"
#define BCAST_URL "/config?command=meshSendStatus"
#define UNICAST_MESH_SENDING_STATUS "{\"mdev_mac\": \"%02X%02X%02X%02X%02X%02X\", \"src_mac\":\"%02X%02X%02X%02X%02X%02X\", \"mesh_send_sta\": %d , \"response\": 0, \"src_ip_t\":\"%d.%d.%d.%d\"}\r\n"
#define PARENT_INFO "\"parent_mdev_mac\":\"%02X%02X%02X%02X%02X%02X\",\"parent_rssi\": %d ,\"child_num\": %d"
#define BUTTON_CONFIG "{\"status\":200,\"path\":\"/device/button/configure\"}"

#if 0
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"
#endif



#endif


#if PLUG_DEVICE
#include "user_plug.h"

#define RESPONSE_FRAME  "{\"status\": 200, \"datapoint\": {\"x\": %d}, \"nonce\": %u, \"deliver_to_device\": true}\n"
#define FIRST_FRAME     "{\"nonce\": %u, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"Authorization\": \"token %s\"}}\n"

#elif LIGHT_DEVICE
#include "user_light.h"
#include "user_light_mesh.h"
#include "user_light_hint.h"
#if 0
#define RESPONSE_FRAME  "{\"status\": 200,\"nonce\": %u, \"datapoint\": {\"x\": %d,\"y\": %d,\"z\": %d,\"k\": %d,\"l\": %d},\"deliver_to_device\":true}\n"
#define FIRST_FRAME     "{\"nonce\": %u, \"path\": \"/v1/device/identify\", \"method\": \"GET\",\"meta\": {\"mode\":\"v\",\"Authorization\": \"token %s\"}}\n"
#define MESH_UPGRADE    "{\"nonce\": %u, \"get\": {\"action\": \"download_rom_base64\", \"version\": \"%s\", \"filename\": \"%s\", \"offset\": %d, \"size\": %d}, \"meta\": {\"Authorization\": \"token %s\"}, \"path\": \"/v1/device/rom/\", \"method\": \"GET\"}\r\n"
#endif
#elif SENSOR_DEVICE
#include "user_sensor.h"

#if HUMITURE_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %u, \"path\": \"/v1/datastreams/tem_hum/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %s%d.%02d,\"y\": %d.%02d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#elif FLAMMABLE_GAS_SUB_DEVICE
#define UPLOAD_FRAME  "{\"nonce\": %u, \"path\": \"/v1/datastreams/flammable_gas/datapoint/\", \"method\": \"POST\", \
\"body\": {\"datapoint\": {\"x\": %d.%03d}}, \"meta\": {\"Authorization\": \"token %s\"}}\n"
#endif

LOCAL uint32 count = 0;
#endif

//#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
//\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"

#if PLUG_DEVICE || LIGHT_DEVICE
#if 0
#define BEACON_FRAME    "{\"path\": \"/v1/ping/\", \"method\": \"POST\",\"meta\": {\"Authorization\": \"token %s\"}}\n"
#define RPC_RESPONSE_FRAME  "{\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true}\n"
#define RPC_RESPONSE_ELEMENT  "\"status\": %d, \"nonce\": %u, \"deliver_to_device\": true"
#define TIMER_FRAME     "{\"body\": {}, \"get\":{\"is_humanize_format_simple\":\"true\"},\"meta\": {\"Authorization\": \"Token %s\"},\"path\": \"/v1/device/timers/\",\"post\":{},\"method\": \"GET\"}\n"
#define pheadbuffer "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Authorization: token %s\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"
#endif
LOCAL uint8 ping_status;
LOCAL os_timer_t beacon_timer;
#endif

#ifdef USE_DNS
ip_addr_t esp_server_ip;
#endif

LOCAL char *g_mac = NULL;
LOCAL struct espconn user_conn;
LOCAL struct _esp_tcp user_tcp;
LOCAL os_timer_t client_timer;
 struct esp_platform_saved_param esp_param;
LOCAL uint8 device_status;
LOCAL uint8 device_recon_count = 0;
LOCAL uint32 active_nonce = 0xffffffff;
LOCAL uint8 iot_version[IOT_BIN_VERSION_LEN + 1] = {0};
LOCAL uint8_t g_mesh_version[IOT_BIN_VERSION_LEN + 1] = {0};
LOCAL uint32  g_mesh_nonce = 0;
LOCAL uint8_t g_devkey[TOKEN_SIZE] = {0};//
#if ESP_MESH_SUPPORT
char ota_dst_addr[ESP_MESH_ADDR_LEN];
#endif

uint8 time_stamp_str[32];
struct rst_info rtc_info;

void user_esp_platform_check_ip(uint8 reset_flag);
LOCAL void user_esp_platform_sent_beacon(struct espconn *pespconn);

uint32 ota_start_time=0,ota_finished_time=0;
#if ESP_MESH_SUPPORT
bool mesh_json_add_elem(char *pbuf, size_t buf_size, char *elem, size_t elem_size);
uint32 get_ota_start_time()
{
    return ota_start_time;
}
uint32 get_ota_finish_time()
{
    return ota_start_time;
}
#endif

#if STRING_SV_IN_FLASH
void ICACHE_FLASH_ATTR
    load_string_from_flash(char* buf,uint16 buf_len,void* flash_str)
{
    memset(buf,0,buf_len);
	system_get_string_from_flash(flash_str,buf,buf_len);
}


#endif

char* ICACHE_FLASH_ATTR user_json_find_section(const char *pbuf,uint16 len,const char* section)
{
#if 0
    return espconn_json_find_section(pbuf,len,section);
#else
    char* sect = NULL,c;
    bool json_split = false;
    if(!pbuf || !section)
        return NULL;

    sect = (char*)os_strstr(pbuf,section);
    /*
    * the formal json format
    *{"elem":"value"}\r\n
    *{"elem":value}\r\n
    */
    if(sect){
        sect += os_strlen(section);
        while((uint32)sect<(uint32)(pbuf + len)){
            c = *sect++;
            if(c == ':' && !json_split){
                json_split = true;
                continue;
            }
            //{"elem":"value"}\r\n
            if(c == '"'){
                break;
            }
            //{"elem":value}\r\n
            //{"elem":  value}\r\n
            //{"elem":\tvalue}\r\n
            //{"elem":\nvalue}\r\n
            //{"elem":\rvalue}\r\n
            if(c != ' ' && c!='\n' && c!='\r'){
                sect--;
                break;
            }
        }
        return sect == NULL || (uint32)sect - (uint32)pbuf >= len?NULL:sect ;
    }

#endif
}

int ICACHE_FLASH_ATTR
    user_JsonGetValueInt(const char* pbuffer,uint16 buf_len,const uint8* json_key)
{
    char val[20];
    uint32 result = 0;
    char* pstr = NULL,*pparse = NULL;
    uint16 val_size=0;

    pstr = user_json_find_section(pbuffer,buf_len,json_key);
    if(pstr){
        //find the end char of value
        if( (pparse = (char*)os_strstr(pstr,","))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"\""))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"}"))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"]"))!= NULL){
            val_size = pparse - pstr;
        }else{
            return 0;
        }
        os_memset(val,0,sizeof(val));
        if(val_size > sizeof(val))
            os_memcpy(val,pstr,sizeof(val));
        else
            os_memcpy(val,pstr,val_size);
        result = atol(val);
    }
    return result;
}

bool ICACHE_FLASH_ATTR
    user_JsonGetValueStr(const char* pbuffer,uint16 buf_len,const uint8* json_key,char* val_buf,uint16 val_buf_len)
{
    //char val[20];
    uint32 result = 0;
    char* pstr = NULL,*pparse = NULL;
    uint16 val_size=0;
    pstr = user_json_find_section(pbuffer,buf_len,json_key);
    if(pstr){
        //find the end char of value
        if( (pparse = (char*)os_strstr(pstr,","))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"\""))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"}"))!= NULL ||
            (pparse = (char*)os_strstr(pstr,"]"))!= NULL){
            val_size = pparse - pstr - 1;
        }else{
            return false;
        }
        //os_memset(val,0,sizeof(val));

        if(val_size > val_buf_len)
            os_memcpy(val_buf,pstr,val_buf_len);
        else
            os_memcpy(val_buf,pstr,val_size);

    }
    return true;
}


sint8 ICACHE_FLASH_ATTR user_JsonDataSend(struct espconn *pconn, uint8_t *buf, uint16_t len,uint8 force,uint8 *src_addr,uint8* dst_addr)
{
    sint8 res;
    bool queue_empty = espSendQueueIsEmpty(espSendGetRingbuf());
    ESP_DBG("send in user_JsonDataSend\r\n");

    #if MESH_HEADER_MODE
	    if((dst_addr == NULL) && (src_addr == NULL)){
			if(force){
				res = espSendEnq(buf, len, pconn, ESP_DATA_FORCE,TO_SERVER,espSendGetRingbuf());
			}else{
				res = espSendEnq(buf, len, pconn, ESP_DATA,TO_SERVER,espSendGetRingbuf());
			}

		}else{
    		struct mesh_header_format* mesh_header = NULL;// (struct mesh_header_format*)espconn->reverse;;
    		//uint8* src_addr = NULL,*dst_addr = NULL;
    		//espconn_mesh_get_src_addr(mesh_header,&src_addr);
    		//espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
        	ESP_DBG("src: "MACSTR"\r\n",MAC2STR(src_addr));
        	ESP_DBG("dst: "MACSTR"\r\n",MAC2STR(dst_addr));
        	mesh_header = (struct mesh_header_format*)espconn_mesh_create_packet( dst_addr,//uint8_t *dst_addr
    			                                                                  src_addr,//uint8_t *src_addr,
    			                                                                  false, //bool p2p,
    			                                                                  true,  //bool piggyback_cr
    			                                                                  M_PROTO_JSON, //enum mesh_usr_proto_type proto
    			                                                                  len, //uint16_t data_len
    			                                                                  false,  //bool option
    			                                                                  0, //uint16_t ot_len
    			                                                                  false, //bool frag
    			                                                                  0,  //enum mesh_option_type frag_type
    			                                                                  false, //bool mf
    			                                                                  0, //uint16_t frag_idx
    			                                                                  0 //uint16_t frag_id
    			                                                                  ); 
        	//mesh_header = (struct mesh_header_format*)espconn_mesh_create_packet( src_addr,dst_addr,false,true,M_PROTO_JSON,len, false,0,false,0, false,0,0); 
        	if(!mesh_header){ 
        		ESP_DBG("alloc resp packet fail,heap: %d\n",system_get_free_heap_size()); 
            	return; 
        	}
        	
        	if(!espconn_mesh_set_usr_data(mesh_header,buf,len)){ 
        		ESP_DBG("setuserdatafail\n"); 
        		os_free(mesh_header); 
        		return; 
        	}
			
			if(force){
				res = espSendEnq(mesh_header, mesh_header->len, pconn, ESP_DATA_FORCE,TO_SERVER,espSendGetRingbuf());
			}else{
				res = espSendEnq(mesh_header, mesh_header->len, pconn, ESP_DATA,TO_SERVER,espSendGetRingbuf());
			}
			
			if(mesh_header) os_free(mesh_header);
			
	    }
		
    #else
        if(force){
        	res = espSendEnq(buf, len, pconn, ESP_DATA_FORCE,TO_SERVER,espSendGetRingbuf());
        }else{
        	res = espSendEnq(buf, len, pconn, ESP_DATA,TO_SERVER,espSendGetRingbuf());
        }
    #endif


    if(res==-1){
        ESP_DBG("espconn send error , no buf in app...\r\n");
    }

    /*send the packet if platform sending queue is empty*/
    /*if not, espconn sendcallback would post another sending event*/
    if(queue_empty){
        system_os_post(ESP_SEND_TASK_PRIO, 0, (os_param_t)espSendGetRingbuf());
    }
	
    return res;
}

#if 0
sint8 ICACHE_FLASH_ATTR user_JsonDataSend(struct espconn *pconn, uint8_t *buf, uint16_t len,uint8 force)
{
    sint8 res;
    bool queue_empty = espSendQueueIsEmpty(espSendGetRingbuf());
    ESP_DBG("send in user_JsonDataSend\r\n");
	if(force){
		res = espSendEnq(buf, len, pconn, ESP_DATA_FORCE,TO_SERVER,espSendGetRingbuf());
	}else{
        res = espSendEnq(buf, len, pconn, ESP_DATA,TO_SERVER,espSendGetRingbuf());
	}
    if(res==-1){
        ESP_DBG("espconn send error , no buf in app...\r\n");
    }

    /*send the packet if platform sending queue is empty*/
    /*if not, espconn sendcallback would post another sending event*/
    if(queue_empty){
        system_os_post(ESP_SEND_TASK_PRIO, 0, (os_param_t)espSendGetRingbuf());
    }
    return res;
}
#endif
/******************************************************************************
 * FunctionName : user_esp_platform_get_token
 * Description  : get the espressif's device token
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_get_token(uint8_t *token)
{
    if (token == NULL) {
        return;
    }
    os_memcpy(token, esp_param.token, sizeof(esp_param.token));
}

void ICACHE_FLASH_ATTR
user_esp_platform_get_devkey(uint8_t *devkey)
{
    if (devkey == NULL) {
        return;
    }
    os_memcpy(devkey, esp_param.devkey, sizeof(esp_param.devkey));
}

void ICACHE_FLASH_ATTR
	user_esp_set_upgrade_finish_flag()
{
    #if ESP_DEBUG_MODE
    esp_param.ota_flag_sv = 1;
	system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
	#endif
}

void ICACHE_FLASH_ATTR
	user_esp_clr_upgrade_finish_flag()
{
    #if ESP_DEBUG_MODE
    esp_param.ota_flag_sv = 0;
	system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
    #endif
}

uint32 ICACHE_FLASH_ATTR
	user_esp_get_upgrade_finish_flag()
{
	return esp_param.ota_flag_sv;
}
/******************************************************************************
 * FunctionName : user_esp_platform_set_token
 * Description  : save the token for the espressif's device
 * Parameters   : token -- the parame point which write the flash
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_token(uint8_t *token)
{
    if (token == NULL || os_strlen(token) > sizeof(esp_param.token)) {
        return;
    }
    esp_param.activeflag = 0;
    os_memcpy(esp_param.token, token, os_strlen(token));

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_active
 * Description  : set active flag
 * Parameters   : activeflag -- 0 or 1
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_active(uint8 activeflag)
{
    esp_param.activeflag = activeflag;

    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
}

void ICACHE_FLASH_ATTR
user_esp_platform_set_connect_status(uint8 status)
{
    device_status = status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_connect_status
 * Description  : get each connection step's status
 * Parameters   : none
 * Returns      : status
*******************************************************************************/
uint8 ICACHE_FLASH_ATTR
user_esp_platform_get_connect_status(void)
{
    uint8 status = wifi_station_get_connect_status();

    if (status == STATION_GOT_IP) {
        status = (device_status == 0) ? DEVICE_CONNECTING : device_status;
    }

    ESP_DBG("status %d\n", status);
    return status;
}

/******************************************************************************
 * FunctionName : user_esp_platform_parse_nonce
 * Description  : parse the device nonce
 * Parameters   : pbuffer -- the recivce data point
 * Returns      : the nonce
*******************************************************************************/
int ICACHE_FLASH_ATTR
user_esp_platform_parse_nonce(char *pbuffer, uint16_t buf_len)
{
    return user_JsonGetValueInt(pbuffer, buf_len, "\"nonce\"");
}

/******************************************************************************
 * FunctionName : user_esp_platform_get_info
 * Description  : get and update the espressif's device status
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_get_info(struct espconn *pconn, uint8 *pbuffer)
{
    char pbuf[800];
    int nonce = 0;
    os_memset(pbuf, 0, sizeof(pbuf));

    nonce = user_esp_platform_parse_nonce(pbuffer, 1460);

    if (pbuf != NULL) {
#if PLUG_DEVICE
        os_sprintf(pbuf, RESPONSE_FRAME, user_plug_get_status(), nonce);
#elif LIGHT_DEVICE
        uint32 white_val;
        white_val = (PWM_CHANNEL>LIGHT_COLD_WHITE?light_param_target.pwm_duty[3]:0);
		#if STRING_SV_IN_FLASH
        char pbuf_tmp[800];
		load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)RESPONSE_FRAME);
		os_sprintf(pbuf, pbuf_tmp, nonce, light_param_target.pwm_period,
				   light_param_target.pwm_duty[0], light_param_target.pwm_duty[1],
				   light_param_target.pwm_duty[2],white_val );//50);		
		#else
        os_sprintf(pbuf, RESPONSE_FRAME, nonce, light_param_target.pwm_period,
                   light_param_target.pwm_duty[0], light_param_target.pwm_duty[1],
                   light_param_target.pwm_duty[2],white_val );//50);
		#endif
#endif

#if ESP_MESH_SUPPORT
        if(g_mac){
            mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
        }else{
            g_mac = (char*)mesh_GetMdevMac();
            mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
        }
#endif
        ESP_DBG("user_esp_platform_get_info %s\n", pbuf);

    #if MESH_HEADER_MODE
		struct mesh_header_format* mesh_header = (struct mesh_header_format*)pconn->reverse;
		uint8* src_addr = NULL;//,*dst_addr = NULL;
		espconn_mesh_get_src_addr(mesh_header,&src_addr);
		//espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
        user_JsonDataSend(pconn, pbuf, os_strlen(pbuf),0,mesh_src_addr,src_addr);//swap dst and src
    #else
	user_JsonDataSend(pconn, pbuf, os_strlen(pbuf),0,NULL,NULL);//swap dst and src
    #endif

    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_set_info
 * Description  : prossing the data and controling the espressif's device
 * Parameters   : pespconn -- the espconn used to connect with host
 *                pbuffer -- prossing the data point
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_set_info(struct espconn *pconn, uint8 *pbuffer)
{
    char *pstr = NULL;
#if PLUG_DEVICE
    pstr = (char *)os_strstr(pbuffer, "plug-status");

    if (pstr != NULL) {
        pstr = (char *)os_strstr(pbuffer, "body");
        if (pstr != NULL) {
            if (os_strncmp(pstr + 27, "1", 1) == 0) {
                user_plug_set_status(0x01);
            } else if (os_strncmp(pstr + 27, "0", 1) == 0) {
                user_plug_set_status(0x00);
            }
        }
    }
#elif LIGHT_DEVICE
    //char *pdata = NULL;
    char *pbuf = NULL;
    //char recvbuf[10];    
    uint16 length = 0;
    uint32 data = 0;
    static uint32 rr,gg,bb,cw,ww,period=1000;
    static uint8 ctrl_mode = 0;
    ww=0;
    cw=0;
    extern uint8 light_sleep_flg;

    ESP_DBG("UNICAST CMD...\r\n");
        pstr = (char *)os_strstr(pbuffer, "{\"datapoint\": ");
        if (pstr != NULL) {
            pbuf = (char *)os_strstr(pbuffer, "}}");
            length = pbuf - pstr;
            length += 2;
            period = user_JsonGetValueInt(pstr, length, "\"x\"");
            rr = user_JsonGetValueInt(pstr, length, "\"y\"");
            gg = user_JsonGetValueInt(pstr, length, "\"z\"");
            bb = user_JsonGetValueInt(pstr, length, "\"k\"");
            cw = user_JsonGetValueInt(pstr, length, "\"l\"");
            ww = cw;
        }
    if((rr|gg|bb|cw|ww) == 0){
        if(light_sleep_flg==0){
            
        }
        
    }else{
        if(light_sleep_flg==1){
            ESP_DBG("modem sleep en\r\n");
            wifi_set_sleep_type(MODEM_SLEEP_T);
            light_sleep_flg =0;
        }
    }
    //light_set_aim(rr,gg,bb,cw,ww,period,ctrl_mode);
	light_set_color(rr,gg,bb,cw,ww,period);

#endif
    user_esp_platform_get_info(pconn, pbuffer);
    ESP_DBG("end of platform_set_info\r\n");
}

/******************************************************************************
 * FunctionName : user_esp_platform_reconnect
 * Description  : reconnect with host after get ip
 * Parameters   : pespconn -- the espconn used to reconnect with host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_reconnect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_reconnect\n");
    if (wifi_get_opmode() == SOFTAP_MODE)
        return;
    user_esp_platform_check_ip(0);
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon_cb
 * Description  : disconnect successfully with the host
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon_cb(void *arg)
{
    struct espconn *pespconn = arg;
    struct ip_info ipconfig;
    struct dhcp_client_info dhcp_info;
    ESP_DBG("user_esp_platform_discon_cb\n");

#if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);
#endif

    if (pespconn == NULL) {
        return;
    }
    pespconn->proto.tcp->local_port = espconn_port();

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
#endif

#if SENSOR_DEVICE
#ifdef SENSOR_DEEP_SLEEP

    if (wifi_get_opmode() == STATION_MODE) {
        /***add by tzx for saving ip_info to avoid dhcp_client start****/
        wifi_get_ip_info(STATION_IF, &ipconfig);

        dhcp_info.ip_addr = ipconfig.ip;
        dhcp_info.netmask = ipconfig.netmask;
        dhcp_info.gw = ipconfig.gw ;
        dhcp_info.flag = 0x01;
        ESP_DBG("dhcp_info.ip_addr = %d\n",dhcp_info.ip_addr);
        system_rtc_mem_write(64,&dhcp_info,sizeof(struct dhcp_client_info));
        user_sensor_deep_sleep_enter();
    } else {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
        os_timer_arm(&client_timer, SENSOR_DEEP_SLEEP_TIME / 1000, 0);
    }

#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
#endif
#else
    user_esp_platform_reconnect(pespconn);
#endif
}

/******************************************************************************
 * FunctionName : user_esp_platform_discon
 * Description  : A new incoming connection has been disconnected.
 * Parameters   : espconn -- the espconn used to disconnect with host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_discon(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_discon\n");

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
#endif


#if ESP_MESH_SUPPORT
    espconn_mesh_disconnect(pespconn);
#else
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_disconnect(pespconn);
#else
    espconn_disconnect(pespconn);
#endif
#endif
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent_cb
 * Description  : Data has been sent successfully and acknowledged by the remote host.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent_cb(void *arg)
{
    struct espconn *pespconn = arg;
    ESP_DBG("user_esp_platform_sent_cb\n");
    os_timer_disarm(&beacon_timer);
    os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
    os_timer_arm(&beacon_timer, BEACON_TIME, 0);
}

/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent(struct espconn *pespconn)
{
    uint8 devkey[TOKEN_SIZE] = {0};
    os_memcpy(devkey,esp_param.devkey,sizeof(esp_param.devkey));
    uint32 nonce;
    char pbuf[512];

    ESP_DBG("%s, aflag = %u\n", __func__, esp_param.activeflag);
    os_memset(pbuf, 0, sizeof(pbuf));
    if (esp_param.activeflag != 0x1) {
        esp_param.activeflag = 0;
    }
    ESP_DBG("\r\n\r\n==============================\r\n");
    ESP_DBG("%s :\r\n",__func__);
    ESP_DBG("MESH DEBUG ACTIVE: %d \r\n",esp_param.activeflag);
    ESP_DBG("---------------------------\r\n\r\n");

    if (pbuf != NULL) {
        if (esp_param.activeflag == 0) {
            uint8 token[TOKEN_SIZE] = {0};
            uint8 bssid[6];
            active_nonce = os_random()&0x7fffffff;//rand();// os_random();


            os_memset(token, 0, sizeof(token));

            wifi_get_macaddr(STATION_IF, bssid);
            
            if ((esp_param.token[0] >= '0' && esp_param.token[0] <= '9') || 
                (esp_param.token[0] >= 'a' && esp_param.token[0] <= 'z') ||
                (esp_param.token[0] >= 'A' && esp_param.token[0] <= 'Z')) {
                os_memcpy(token, esp_param.token, 40);
            } else {
                uint8_t i, j;
                for (i = 0, j = 0; i < 38; i = i + 2, j ++) {
                    if (j == 6)
                        j = 0;
                    os_sprintf(esp_param.token + i, "%02X", bssid[j]);
                }
                esp_param.token[i ++] = 'F';
                esp_param.token[i ++] = 'F';
                os_memcpy(token, esp_param.token, 40);
            }

			#if STRING_SV_IN_FLASH
			char pbuf_tmp[512];
			os_memset(pbuf_tmp,0,sizeof(pbuf_tmp));
			system_get_string_from_flash((void*)ACTIVE_FRAME,pbuf_tmp,sizeof(pbuf_tmp));
            os_sprintf(pbuf, pbuf_tmp, active_nonce, token, MAC2STR(bssid),iot_version, devkey);
			#else
            os_sprintf(pbuf, ACTIVE_FRAME, active_nonce, token, MAC2STR(bssid),iot_version, devkey);
            #endif
			ESP_DBG("PBUF1 MAX:512; LEN: %d \r\n",os_strlen(pbuf));
        }

#if SENSOR_DEVICE
    #if HUMITURE_SUB_DEVICE
        else {
            uint16 tp, rh;
            uint8 *data;
            uint32 tp_t, rh_t;
            data = (uint8 *)user_mvh3004_get_poweron_th();

            rh = data[0] << 8 | data[1];
            tp = data[2] << 8 | data[3];
            tp_t = (tp >> 2) * 165 * 100 / (16384 - 1);
            rh_t = (rh & 0x3fff) * 100 * 100 / (16384 - 1);

            if (tp_t >= 4000) {
                os_sprintf(pbuf, UPLOAD_FRAME, count, "", tp_t / 100 - 40, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            } else {
                tp_t = 4000 - tp_t;
                os_sprintf(pbuf, UPLOAD_FRAME, count, "-", tp_t / 100, tp_t % 100, rh_t / 100, rh_t % 100, devkey);
            }
        }

    #elif FLAMMABLE_GAS_SUB_DEVICE
        else {
            uint32 adc_value = system_adc_read();

            os_sprintf(pbuf, UPLOAD_FRAME, count, adc_value / 1024, adc_value * 1000 / 1024, devkey);
        }

    #endif
#else
        else {
            nonce = os_random()&0x7fffffff;
			#if STRING_SV_IN_FLASH
            char pbuf_tmp[512];
			load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)FIRST_FRAME);
			os_sprintf(pbuf, pbuf_tmp, nonce , devkey);
			#else
            os_sprintf(pbuf, FIRST_FRAME, nonce , devkey);
			#endif
			user_esp_platform_sent_beacon(pespconn);
        }
#endif

uint8 *dst_t = NULL, *src_t = NULL;
#if ESP_MESH_SUPPORT
	#if MESH_HEADER_MODE
	    ESP_DBG("dbg2\r\n");
		uint8 dst[6],src[6];
		if(pespconn && pespconn->proto.tcp){
			os_memcpy(dst,pespconn->proto.tcp->remote_ip,4);
			os_memcpy(dst+4,&(pespconn->proto.tcp->remote_port),2);
		}
		wifi_get_macaddr(STATION_IF,src);	
		ESP_DBG("dbg: src: "MACSTR"\r\n",MAC2STR(src));
		ESP_DBG("dbg: dst: "MACSTR"\r\n",MAC2STR(dst));
		dst_t = dst;
		src_t = src;
	#else
	    if(g_mac){
            mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
        }else{
            g_mac = (char*)mesh_GetMdevMac();
            mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
        }
    #endif
#endif
        user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,src_t,dst_t);
    }
}

#if PLUG_DEVICE || LIGHT_DEVICE
/******************************************************************************
 * FunctionName : user_esp_platform_sent_beacon
 * Description  : sent beacon frame for connection with the host is activate
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_sent_beacon(struct espconn *pespconn)
{
    struct ip_info ipconfig;
    if (pespconn == NULL) {
        return;
    }
    wifi_get_ip_info(STATION_IF, &ipconfig);
    ESP_DBG("===============\r\n");
    ESP_DBG("ESPCONN STATE: %d \r\n",pespconn->state);
    ESP_DBG("===============\r\n");
    if(pespconn->state == ESPCONN_CONNECT) { 
        if (esp_param.activeflag == 0) {
            ESP_DBG("please check device is activated.\n");
            user_esp_platform_sent(pespconn);
        } else {
            uint8 devkey[TOKEN_SIZE] = {0};
            os_memcpy(devkey, esp_param.devkey, 40);

            ESP_DBG("%s %u\n", __func__, system_get_time());

            if (ping_status == 0) {
                ESP_DBG("user_esp_platform_sent_beacon sent fail!\n");
                #if ESP_MESH_SUPPORT==0
                    user_esp_platform_discon(pespconn);
                #else
                    ESP_DBG("send beacon in timer now...\r\n");
                    ping_status = 1;
                    os_timer_arm(&beacon_timer, 1000, 0);
                #endif
            } else {
                //char *pbuf = (char *)os_zalloc(packet_size);
                char pbuf[512];
                os_memset(pbuf, 0, sizeof(pbuf));

                //if (pbuf != NULL) {
                    #if STRING_SV_IN_FLASH
                    char pbuf_tmp[512];
					load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)BEACON_FRAME);
					os_sprintf(pbuf, pbuf_tmp, devkey);
					#else
                    os_sprintf(pbuf, BEACON_FRAME, devkey);
					#endif
                    #if ESP_MESH_SUPPORT
                    if(g_mac){
                        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
                    }else{
                        g_mac = (char*)mesh_GetMdevMac();
                        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
                    }
                    
                    uint8* info_mesh = NULL;
                    uint8 g_prnt_mac[80];
                    uint16 cnt=0,i=0;
					uint16 cnt_t;
					int8 rssi;
					uint16 child_num=0xff;
                    if (espconn_mesh_get_node_info(MESH_NODE_CHILD, &info_mesh, &child_num)) {
                        os_printf("child num \r\n");
                    }else{
                        child_num = 0xff;
                    }



					
                    if (espconn_mesh_get_node_info(MESH_NODE_PARENT, &info_mesh, &cnt_t)) {
						cnt = cnt_t&0xff;
						rssi = (cnt_t>>8) &0xff;
						ESP_DBG("RSSI: %d; CNT: %d\r\n",rssi,cnt);
                        ESP_DBG("get parent info success\n");
                        if (cnt == 0) {
                            ESP_DBG("no parent\n");
                        } else {
                            // children represents the count of children.
                            // you can read the child-information from child_info.
                            for(i=0;i<cnt;i++){
                                //ESP_DBG("ptr[%d]: %p \r\n",i,info_mesh+i*6);
                                ESP_DBG("MAC[%d]:"MACSTR"\r\n",i,MAC2STR(info_mesh+i*6));
                            }
                            os_memset(g_prnt_mac,0,sizeof(g_prnt_mac));
							#if STRING_SV_IN_FLASH
							char parent_info[100];
							load_string_from_flash(parent_info,sizeof(parent_info),(void*)PARENT_INFO);
							os_sprintf(g_prnt_mac,parent_info,MAC2STR(info_mesh),rssi,child_num);
							#else						
                            os_sprintf(g_prnt_mac,PARENT_INFO,MAC2STR(info_mesh),rssi,child_num);
							#endif
                            ESP_DBG("g_prnt_mac: %s \r\n",g_prnt_mac);
                        }
                    } else {
                        ESP_DBG("get parent info fail\n");
                        info_mesh = NULL;
                        cnt = 0;
                        //mesh_json_add_elem(pbuf, sizeof(pbuf), "\"parent_mdev_mac\":\"000000000000\"", ESP_MESH_JSON_DEV_PRNT_MAC_ELEM_LEN);
                    } 

                    if(info_mesh!=NULL && cnt>0){
                        mesh_json_add_elem(pbuf, sizeof(pbuf), g_prnt_mac, os_strlen(g_prnt_mac));
                    }else{
                        mesh_json_add_elem(pbuf, sizeof(pbuf), "\"parent_mdev_mac\":\"000000000000\"", ESP_MESH_JSON_DEV_PRNT_MAC_ELEM_LEN);
                    }

                    if (!espconn_mesh_local_addr(&ipconfig.ip)){
                        ping_status = 0;
                    }
                    #endif

					
                    #if MESH_HEADER_MODE
						uint8 dst[6],src[6];
						if(pespconn && pespconn->proto.tcp){
							os_memcpy(dst,pespconn->proto.tcp->remote_ip,4);
							os_memcpy(dst+4,&pespconn->proto.tcp->remote_port,2);
						}
						wifi_get_macaddr(STATION_IF,src);
						user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,src,dst);
					#else
					user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,NULL,NULL);
                    #endif
					
                    
                    ESP_DBG("PBUF2 MAX:256; LEN: %d \r\n",os_strlen(pbuf));

                    os_timer_disarm(&beacon_timer);
                    os_timer_arm(&beacon_timer, BEACON_TIME, 0);
                    //os_free(pbuf);
                //}
            }
        }
    } else {
        ESP_DBG("user_esp_platform_sent_beacon sent fail! TCP NOT CONNECT:%d\n",pespconn->state == ESPCONN_CONNECT);
        #if ESP_MESH_SUPPORT==0
        user_esp_platform_discon(pespconn);
        #endif
    }
}

/******************************************************************************
 * FunctionName : user_platform_rpc_set_rsp
 * Description  : response the message to server to show setting info is received
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                nonce -- mark the message received from server
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_platform_rpc_set_rsp(struct espconn *pespconn, int nonce, int status)
{
    //char *pbuf = (char *)os_zalloc(packet_size);
    char pbuf[256];

    os_memset(pbuf, 0, sizeof(pbuf));
    if (pespconn == NULL) {
        return;
    }
    #if STRING_SV_IN_FLASH
    char pbuf_tmp[256];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)RPC_RESPONSE_FRAME);
	os_sprintf(pbuf, pbuf_tmp, status, nonce);
	#else
    os_sprintf(pbuf, RPC_RESPONSE_FRAME, status, nonce);
    #endif
	#if ESP_MESH_SUPPORT
    if(g_mac){
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }
    #endif

	#if MESH_HEADER_MODE
	struct mesh_header_format* mesh_header = (struct mesh_header_format*)pespconn->reverse;
	uint8* src_addr = NULL;//,*dst_addr = NULL;
	espconn_mesh_get_src_addr(mesh_header,&src_addr);
    user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,mesh_src_addr,src_addr);//swap src and dst
    #else
    user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,NULL,NULL);
	#endif
	
    ESP_DBG("PBUF3 MAX:256; LEN: %d \r\n",os_strlen(pbuf));
    //os_free(pbuf);
}

LOCAL bool ICACHE_FLASH_ATTR
user_platform_rpc_build_rsp( char* pdata,int data_max_size,int nonce, int status)
{
    char pbuf[256];
    os_memset(pbuf, 0, sizeof(pbuf));

	#if STRING_SV_IN_FLASH
    char pbuf_tmp[256];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)RPC_RESPONSE_ELEMENT);
	os_sprintf(pbuf, pbuf_tmp, status, nonce);
	#else
    os_sprintf(pbuf, RPC_RESPONSE_ELEMENT, status, nonce);
	#endif
    
    if(!mesh_json_add_elem(pdata,data_max_size, pbuf, os_strlen(pbuf))){
        ESP_DBG("user_platform_rpc_build_rsp error...\r\n");
        return false;
    }

    while(1){
        char* ptmp = os_strchr(pdata,'\n');
        if(ptmp){
            *ptmp = ' ';
        }else{
            break;
        }

    }
    int idx = os_strlen(pdata);
    if(pdata[idx-1]!='\n'){
        pdata[idx]='\n';
    }
    ESP_DBG("debug:\r\nbuild rsp:%s\r\n",pdata);
    return true;
}


LOCAL void ICACHE_FLASH_ATTR
user_platform_rpc_battery_rsp(struct espconn *pespconn, char* data_buf,int data_buf_len, int nonce, int status)
{
    if (pespconn == NULL) {
        return;
    }

    #if ESP_MESH_SUPPORT
    if(g_mac){
        ESP_DBG("G_MAC: %s\r\n",g_mac);
        mesh_json_add_elem(data_buf, data_buf_len, g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        ESP_DBG("G_MAC: %s\r\n",g_mac);
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(data_buf, data_buf_len, g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }

    if(! user_platform_rpc_build_rsp( data_buf,data_buf_len,nonce, status)){
        return;
    }
    #endif

    #if MESH_HEADER_MODE
    	struct mesh_header_format* mesh_header = (struct mesh_header_format*)pespconn->reverse;
    	uint8* src_addr = NULL;//,*dst_addr = NULL;
    	espconn_mesh_get_src_addr(mesh_header,&src_addr);
    	//espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
		user_JsonDataSend(pespconn, data_buf, os_strlen(data_buf),0,mesh_src_addr,src_addr);//swap src and dst
    #else	
    user_JsonDataSend(pespconn, data_buf, os_strlen(data_buf),0,NULL,NULL);//swap src and dst
    #endif
	
    ESP_DBG("PBUF4 MAX:256; LEN: %d \r\n",os_strlen(data_buf));
    //os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_platform_timer_get
 * Description  : get the timers from server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_platform_timer_get(struct espconn *pespconn)
{
    uint8 devkey[TOKEN_SIZE] = {0};
    os_memcpy(devkey, esp_param.devkey, 40);
    //char *pbuf = (char *)os_zalloc(packet_size);
    char pbuf[512];

    os_memset(pbuf, 0, sizeof(pbuf));
    if (pespconn == NULL) {
        return;
    }
    #if STRING_SV_IN_FLASH
    char pbuf_tmp[512];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)TIMER_FRAME);
	os_sprintf(pbuf, pbuf_tmp, devkey);
	#else
    os_sprintf(pbuf, TIMER_FRAME, devkey);
	#endif
#if ESP_MESH_SUPPORT
    if(g_mac){
        ESP_DBG("G_MAC: %s\r\n",g_mac);
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        ESP_DBG("G_MAC: %s\r\n",g_mac);
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }
#endif

    ESP_DBG("========================\r\n");
    ESP_DBG("%s \r\n",__func__);
    ESP_DBG("PBUF5 MAX:512; LEN: %d \r\n",os_strlen(pbuf));
    ESP_DBG("------------------\r\n");
    ESP_DBG("%s\n", pbuf);
    ESP_DBG("========================\r\n");

	
    #if MESH_HEADER_MODE
		uint8 dst[6],src[6];
		if(pespconn && pespconn->proto.tcp){
			os_memcpy(dst,pespconn->proto.tcp->remote_ip,4);
			os_memcpy(dst+4,&pespconn->proto.tcp->remote_port,2);
		}
		wifi_get_macaddr(STATION_IF,src);		
		user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,src,dst);
	#else	
    user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),0,NULL,NULL);
    #endif

    //os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    struct espconn *pespconn = server->pespconn;
    uint8 devkey[41] = {0};
    os_memcpy(devkey, esp_param.devkey, 40);
    char *action = NULL;
    
    uint8 pbuf[512];
    os_memset(pbuf, 0, sizeof(pbuf));

    uint8 *src = NULL,*dst = NULL;
    #if MESH_HEADER_MODE
		uint8 dst_t[6],src_t[6];
		if(pespconn && pespconn->proto.tcp){
			os_memcpy(dst_t,pespconn->proto.tcp->remote_ip,4);
			os_memcpy(dst_t+4,&pespconn->proto.tcp->remote_port,2);
		}
		wifi_get_macaddr(STATION_IF,src_t);		
		src = src_t;
		dst = dst_t;
    #endif

    if (server->upgrade_flag == true) {
        ESP_DBG("user_esp_platform_upgarde_successfully\n");
        action = "device_upgrade_success";
        //os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
        //ESP_DBG("%s\n",pbuf);
    } else {
        ESP_DBG("user_esp_platform_upgrade_failed\n");
        action = "device_upgrade_failed";
        //os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
        //ESP_DBG("%s\n",pbuf);
    }
	#if STRING_SV_IN_FLASH
    char pbuf_tmp[512];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)UPGRADE_FRAME);
	os_sprintf(pbuf, pbuf_tmp, devkey, action,server->pre_version, server->upgrade_version);
	#else
	os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
	#endif
	user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),1,src,dst);//SEND TO SERVER
	//#if ESP_MESH_SUPPORT
	//ESP_DBG("response to upgrde phone\r\n");
	//user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),1,mesh_src_addr,ota_dst_addr);//SEND TO PHONE IN LOCAL MODE
	//#endif

    if (server->url)
        os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_upgrade_begin(struct espconn *pespconn, struct upgrade_server_info *server)
{
    uint8 devkey[41] = {0};
    os_memcpy(devkey, esp_param.devkey, 40);
    uint8 user_bin[10] = {0};
    const char *usr1 = "user1.bin";
    const char *usr2 = "user2.bin";

    server->pespconn = pespconn;

    os_memcpy(server->ip, pespconn->proto.tcp->remote_ip, 4);

#ifdef UPGRADE_SSL_ENABLE
    server->port = 8443;
#else
    server->port = 8000;
#endif

    server->check_cb = user_esp_platform_upgrade_rsp;
    server->check_times = 120000;

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(1024);
    }
    if (server->url == NULL) {
        return;
    }

    os_memset(user_bin, 0, sizeof(user_bin));
    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, usr2, os_strlen(usr2));
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, usr1, os_strlen(usr1));
    }

	#if STRING_SV_IN_FLASH
    char pbuf_tmp[512];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)OTA_URL);
    os_sprintf(server->url,pbuf_tmp,server->upgrade_version, user_bin, 
		       IP2STR(server->ip),server->port, devkey);
	#else
    //os_sprintf(server->url, "GET /v1/device/rom/?action=download_rom&version=%s&filename=%s HTTP/1.0\r\nContent-Length: 23\r\nHost: "IPSTR":%d\r\n" pheadbuffer "\n",
    //           server->upgrade_version, user_bin, IP2STR(server->ip),
    //           server->port, devkey);
    os_sprintf(server->url, OTA_URL,
               server->upgrade_version, user_bin, IP2STR(server->ip),
               server->port, devkey);
	#endif
    ESP_DBG("%s\n",server->url);

#ifdef UPGRADE_SSL_ENABLE

    if (system_upgrade_start_ssl(server) == false) {
#else

    if (system_upgrade_start(server) == false) {
#endif
        ESP_DBG("upgrade is already started\n");
    }
}
#endif

#if ESP_MESH_SUPPORT
struct espconn *g_mesh_esp = NULL;
os_timer_t g_mesh_upgrade_timer;

void ICACHE_FLASH_ATTR
    mesh_upgrade_check_func(void *arg)
{
    uint8_t pbuf[512] = {0};
    struct espconn *pespconn = arg;
    struct upgrade_server_info *server = NULL;
    os_timer_disarm(&g_mesh_upgrade_timer);
    system_upgrade_deinit();
    if (!pespconn)
        pespconn = g_mesh_esp;
    os_memset(pbuf, 0, sizeof(pbuf));

	char* action = NULL;
    if( system_upgrade_flag_check() == UPGRADE_FLAG_FINISH ) {
		action = "device_upgrade_success";
        //os_sprintf(pbuf, UPGRADE_FRAME, g_devkey, "device_upgrade_success", iot_version, g_mesh_version);
        #if LIGHT_DEVICE&&ESP_DEBUG_MODE
            light_set_aim(0,22222,0,10000,10000,1000,0);
		    //light_set_color(0,22222,0,10000,10000,1000);
        #endif
    } else {
        action = "device_upgrade_failed";
        //os_sprintf(pbuf, UPGRADE_FRAME, g_devkey, "device_upgrade_failed", iot_version, g_mesh_version);
        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
        #if LIGHT_DEVICE&&ESP_DEBUG_MODE
            light_set_aim(22222,0,0,0,0,1000,0);
		    //light_set_color(22222,0,0,0,0,1000);
        #endif
    }

	#if STRING_SV_IN_FLASH
	char pbuf_tmp[512];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)UPGRADE_FRAME);
	os_sprintf(pbuf, pbuf_tmp, g_devkey, "device_upgrade_failed", iot_version, g_mesh_version);
	#else
	os_sprintf(pbuf, UPGRADE_FRAME, g_devkey, "device_upgrade_failed", iot_version, g_mesh_version);
	#endif
		
    if(g_mac){
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(pbuf, sizeof(pbuf), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }

    #if MESH_HEADER_MODE
		uint8 dst[6],src[6];
		if(pespconn && pespconn->proto.tcp){
			os_memcpy(dst,pespconn->proto.tcp->remote_ip,4);
			os_memcpy(dst+4,&pespconn->proto.tcp->remote_port,2);
		}
		wifi_get_macaddr(STATION_IF,src);		
    #endif
    user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),1,src,dst);
	
	#if ESP_MESH_SUPPORT
	ESP_DBG("response to upgrde phone\r\n");
	user_JsonDataSend(pespconn, pbuf, os_strlen(pbuf),1,mesh_src_addr,ota_dst_addr);//SEND TO PHONE IN LOCAL MODE
	#endif

    if (g_mesh_esp)
        os_free(g_mesh_esp);
    g_mesh_esp = NULL;
    os_memset(g_mesh_version, 0, sizeof(g_mesh_version));
}

LOCAL bool ICACHE_FLASH_ATTR
user_mesh_upgrade_build_pkt(uint8_t *pkt_buf, int nonce, char *version, uint32_t offset, uint16_t sect_len)
{
    char *bin_file = NULL;
    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        bin_file = "user2.bin";
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        bin_file = "user1.bin";
    }
    if (!bin_file)
        return false;
    if(!version)
		return false;

	#if STRING_SV_IN_FLASH
    char pbuf_tmp[512];
	load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)MESH_UPGRADE);
    os_sprintf(pkt_buf, pbuf_tmp, nonce, version, bin_file, offset, sect_len, g_devkey);	
	#else
    os_sprintf(pkt_buf, MESH_UPGRADE, nonce, version, bin_file, offset, sect_len, g_devkey);
	#endif
    return true;
}

os_timer_t mesh_upgrade_delay_t;
LOCAL void ICACHE_FLASH_ATTR
	user_mesh_upgrade_start_func()
{
	os_timer_disarm(&mesh_upgrade_delay_t);
    if (NULL == g_mesh_esp){
        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
		user_esp_clr_upgrade_finish_flag();
        os_timer_disarm(&g_mesh_upgrade_timer);
        ESP_DBG("upgrade error...\r\n");
    }
	user_esp_clr_upgrade_finish_flag();//clear flag to start ota
	system_upgrade_init();
    system_upgrade_flag_set(UPGRADE_FLAG_START);

    char *pstr = NULL;
    char buffer[512] = {0};
    os_memset(buffer, 0, sizeof(buffer));

    if (!user_mesh_upgrade_build_pkt(buffer, g_mesh_nonce, g_mesh_version, 0, MESH_UPGRADE_SEC_SIZE)) {
        //user_platform_rpc_set_rsp(g_mesh_esp, g_mesh_nonce, 400);
        ESP_DBG("ota build pkt error 02 \r\n");
		user_platform_rpc_set_rsp(&user_conn, g_mesh_nonce, 400);
        return;
    }
    if (g_mac){
        mesh_json_add_elem(buffer, sizeof(buffer), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(buffer, sizeof(buffer), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }

    #if 0 // MESH_HEADER_MODE
    uint8 dst[6],src[6];
	if(user_conn.proto.tcp){
		ESP_DBG("remote ip: "IPSTR"\r\n",IP2STR(mesh_ota_dst));
		ESP_DBG("remote port: %d \r\n",mesh_ota_dst+4);
        os_memcpy(dst,(uint8*)(user_conn.proto.tcp->remote_ip),4);
		os_memcpy(dst+4,(uint8*)&(user_conn.proto.tcp->remote_port),2);
	}
	wifi_get_macaddr(STATION_IF,src);
	#endif
	
	ESP_DBG("remote ip: "IPSTR"\r\n",IP2STR(ota_dst_addr));
	ESP_DBG("remote port: %d \r\n",*((uint16*)(ota_dst_addr+4)));
    user_JsonDataSend(&user_conn, buffer, os_strlen(buffer),1,mesh_src_addr,ota_dst_addr);

}
LOCAL void ICACHE_FLASH_ATTR
user_mesh_upgrade_delay_start(uint32 delay)
{
    os_timer_disarm(&mesh_upgrade_delay_t);
	os_timer_setfn(&mesh_upgrade_delay_t,user_mesh_upgrade_start_func,NULL);
	os_timer_arm(&mesh_upgrade_delay_t,delay,0);

}

LOCAL void ICACHE_FLASH_ATTR
user_mesh_upgrade_continue(struct espconn *esp, char *pkt, size_t pkt_len,
                           uint32_t offset, uint16_t sect_len)
{
    ESP_DBG("IN user_mesh_upgrade_continue\r\n");
    char *pstr = NULL;
    char buffer[512] = {0};
    char *version = "\"version\":";
    int nonce = user_JsonGetValueInt(pkt, pkt_len, "\"nonce\":");
    os_memset(buffer, 0, sizeof(buffer));
    if (g_mesh_version[0] == 0) {
        version = user_json_find_section(pkt, pkt_len, version);
        if (version)
            os_memcpy(g_mesh_version, version, IOT_BIN_VERSION_LEN);
    }
    if (!user_mesh_upgrade_build_pkt(buffer, nonce, g_mesh_version, offset, sect_len)) {
		ESP_DBG("build packet error in user_mesh_upgrade_continue\r\n");
        user_platform_rpc_set_rsp(esp, nonce, 400);
        return;
    }
    if (g_mac){
        mesh_json_add_elem(buffer, sizeof(buffer), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }else{
        g_mac = (char*)mesh_GetMdevMac();
        mesh_json_add_elem(buffer, sizeof(buffer), g_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    }
	
#if 0// MESH_HEADER_MODE
	struct mesh_header_format* mesh_header = (struct mesh_header_format*)esp->reverse;
	uint8* src_addr = NULL;//,*dst_addr = NULL;
	espconn_mesh_get_src_addr(mesh_header,&src_addr);
	//espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
#endif

    user_JsonDataSend(esp, buffer, os_strlen(buffer),1,mesh_src_addr,ota_dst_addr);//swap src and dst
}


void ICACHE_FLASH_ATTR
	user_esp_mesh_upgrade_reboot()
{
    #if ESP_MESH_SUPPORT
		ESP_DBG("deinit for OTA\r\n");
		espconn_mesh_disable(NULL);
		wifi_station_disconnect();
		wifi_set_opmode(NULL_MODE);
    #endif
	
	user_esp_clr_upgrade_finish_flag();//clear flag and reboot
	system_upgrade_reboot();
}



LOCAL void ICACHE_FLASH_ATTR
user_esp_mesh_upgrade(struct espconn *esp, char *pkt, size_t pkt_len, char *base64_bin)
{
    uint8_t *bin_file = NULL;
    uint16_t sect_len, base64_len, bin_len;
    uint32_t total_len = 0, offset = 0; 
	static bool upgrade_erase_flg = false;
    int idx;
    sect_len = user_JsonGetValueInt(pkt, pkt_len, "\"size\":");
    base64_len = user_JsonGetValueInt(pkt, pkt_len, "\"size_base64\":");
    offset = user_JsonGetValueInt(pkt, pkt_len, "\"offset\":");
    total_len = user_JsonGetValueInt(pkt, pkt_len, "\"total\":");
    if(offset==0 && total_len>0){
        ESP_DBG("TOTAL LEN: %d \r\n",total_len);
		if(upgrade_erase_flg == false){
			ESP_DBG("ERASE FLASH FOR UPGRADE \r\n");
			system_upgrade_erase_flash(total_len);
            user_mesh_upgrade_delay_start(5000);
			upgrade_erase_flg = true;
			ESP_DBG("RETURN;\r\n");
			return;
		}
    }else if(offset>0){
        upgrade_erase_flg = false;
    }
    if (offset + sect_len <= total_len) {
        char c = base64_bin[base64_len];
        base64_bin[base64_len] = '\0';
        bin_len = base64Decode(base64_bin, base64_len + 1, &bin_file);
        ESP_DBG("-----------\r\n");
        ESP_DBG("test bin: len: %d \r\n",bin_len);
        if(UPGRADE_FLAG_START != system_upgrade_flag_check()){
            ESP_DBG("-------------------\r\n");
            ESP_DBG("UPGRADE ALREADY STOPPED...ERROR OR TIMEOUT...status:%d\r\n",system_upgrade_flag_check());
            ESP_DBG("-------------------\r\n");
            return;
        }
        system_upgrade(bin_file, bin_len);
        if (bin_file)
            os_free(bin_file);
        base64_bin[base64_len] = c;
        if (offset + sect_len < total_len) {
            ESP_DBG("%d / %d \r\n",offset + sect_len,total_len);
            user_mesh_upgrade_continue(esp, pkt, pkt_len, offset + sect_len, MESH_UPGRADE_SEC_SIZE);
			ESP_DBG("after upgrade continue...\r\n");
			return;
        }
        system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
		esp_param.ota_flag_sv = 1;//set flag wait for reboot
        ota_finished_time = system_get_time();
        esp_param.ota_finish_time = ota_finished_time;
        system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));

        ESP_DBG("=================\r\n");
        ESP_DBG("upgrade finish...SET FLAG : %d\r\n",system_upgrade_flag_check());
        ESP_DBG("=================\r\n");
        os_timer_disarm(&g_mesh_upgrade_timer);
        mesh_upgrade_check_func(esp);
    } else {  // upgrade fail
        user_platform_rpc_set_rsp(esp, user_JsonGetValueInt(pkt, pkt_len, "\"nonce\":"), 400);
        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
		user_esp_clr_upgrade_finish_flag();
        os_timer_disarm(&g_mesh_upgrade_timer);
        mesh_upgrade_check_func(esp);
    }
}

//mesh upgrade init , start in 5 seconds
LOCAL void ICACHE_FLASH_ATTR
user_esp_mesh_upgrade_init(struct espconn *esp, int nonce, uint8_t *pkt, uint16_t pkt_len)
{
    ESP_DBG("in user_esp_mesh_upgrade_init\r\n");
    if (!g_mesh_esp)
        g_mesh_esp = (struct espconn *)os_zalloc(sizeof(struct espconn));
    os_memcpy(g_mesh_esp, esp, sizeof(*esp));
    g_mesh_esp->reverse = (void *)nonce;
    char* version = "\"version\":";
	os_memset(g_mesh_version, 0, sizeof(g_mesh_version));
	version = user_json_find_section(pkt, pkt_len, version);
    if (version){
        os_memcpy(g_mesh_version, version, IOT_BIN_VERSION_LEN);
    }
	g_mesh_nonce = user_JsonGetValueInt(pkt, pkt_len, "\"nonce\":");

	uint8 broadcast = 0;
	char* pBcst = NULL;
	char broadcast_cmd[20];
	os_bzero(broadcast_cmd,sizeof(broadcast_cmd));
	if((pBcst = (char*)user_json_find_section(pkt,pkt_len,"\"broadcast\":")) != NULL){
		broadcast = user_JsonGetValueInt(pkt, pkt_len, "\"broadcast\":");
		//user_JsonGetValueStr(pkt,pkt_len,"\"broadcast\":",broadcast_cmd,sizeof(broadcast_cmd));
		ESP_DBG("Broadcast flag: %s \r\n",broadcast_cmd);
		//broadcast = strtol(broadcast_cmd,NULL,10);
	}
	if(broadcast == 1){
        //call upgrade broadcast
        ESP_DBG("call upgrade broadcast;\r\n");
		light_SendMeshBroadcastUpgrade(nonce,g_mesh_version,pkt_len);
	}
	user_mesh_upgrade_delay_start(5000);
	
	ESP_DBG("debug : send ota resp\r\n");
    user_platform_rpc_set_rsp(esp, nonce, 200);
    os_timer_disarm(&g_mesh_upgrade_timer);
    os_timer_setfn(&g_mesh_upgrade_timer, (os_timer_func_t *)mesh_upgrade_check_func, g_mesh_esp);
    os_timer_arm(&g_mesh_upgrade_timer, 3600000, 0); //20 min
    #if ESP_DEBUG_MODE
        //light_set_aim(0,0,22222,5000,5000,1000,0);
    	light_shadeStart(HINT_BLUE,500,5,1,NULL);
    #endif
}

#endif


LOCAL void ICACHE_FLASH_ATTR
user_esp_upgrade_begin(struct espconn *pespconn, int nonce, char *version)
{
    struct upgrade_server_info *server = NULL;
    user_platform_rpc_set_rsp(pespconn, nonce, 200);
    server = (struct upgrade_server_info *)os_zalloc(sizeof(struct upgrade_server_info));
    os_memcpy(server->upgrade_version, version, IOT_BIN_VERSION_LEN);
    server->upgrade_version[IOT_BIN_VERSION_LEN] = '\0';
    os_sprintf(server->pre_version,"%s%d.%d.%dt%d(%s)", VERSION_TYPE,IOT_VERSION_MAJOR,
            IOT_VERSION_MINOR,IOT_VERSION_REVISION, device_type,UPGRADE_FALG);
    user_esp_platform_upgrade_begin(pespconn, server);
}
/*
csc add for simple pair
*/
LOCAL int ICACHE_FLASH_ATTR
button_status_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
#if 1
    ESP_DBG("---button set----\n");
    int type;
    uint8 mdev_mac_address[13]={0};
    uint32 replace=0;
    uint8 tempkey[33]={0};
    uint8 button_address[13]={0};
    uint8 mac_len=0;
    uint8 mac[13]={0};
    mac[12]=0;
    mdev_mac_address[12]=0;
    tempkey[32]=0;
    button_address [12]=0; 
    while ((type = jsonparse_next(parser)) != 0) {
        if (type == JSON_TYPE_PAIR_NAME) {
            if(jsonparse_strcmp_value(parser, "mdev_mac") == 0){
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, mdev_mac_address, sizeof(mdev_mac_address));
                ESP_DBG("mdev_mac_address: %s \n",mdev_mac_address);
            } 
            else if (jsonparse_strcmp_value(parser, "temp_key") == 0) {
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, tempkey, sizeof(tempkey));
                //os_memcpy(buttonPairingInfo.tempkey_p,tempkey,sizeof(buttonPairingInfo.tempkey_p));
                int j;
                char tmp[17];
                char* ptmp = tempkey;
                for(j=0;j<16;j++){
                    os_bzero(tmp,sizeof(tmp));
                    os_memcpy(tmp,ptmp,2);
                    uint8 val = strtol(tmp,NULL,16);
                    ESP_DBG("val[%d]: %02x \r\n",j,val);
                    buttonPairingInfo.tempkey[j] = val;
                    ptmp+=2;
                }
                ESP_DBG("tempkey: %s\n",tempkey,js_ctx->depth);
            } 
            else if (jsonparse_strcmp_value(parser, "button_mac") == 0) {
                #if 1
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, button_address, sizeof(button_address));
                ESP_DBG("button_mac: %s \n",button_address);
                int j;
                char tmp[4];
                char* ptmp = button_address;
                for(j=0;j<6;j++){
                    os_bzero(tmp,sizeof(tmp));
                    os_memcpy(tmp,ptmp,2);
                    uint8 val = strtol(tmp,NULL,16);
                    ESP_DBG("val[%d]: %02x \r\n",j,val);
                    buttonPairingInfo.button_mac[j] = val;
                    ptmp+=2;
                }
                #endif
            }
            else if  (jsonparse_strcmp_value(parser, "replace") == 0) {
                jsonparse_next(parser);
                jsonparse_next(parser);
                replace = jsonparse_get_value_as_int(parser);
                ESP_DBG("replace=%d\n",replace);
            }
            else if (jsonparse_strcmp_value(parser, "mac_len") == 0) {
                jsonparse_next(parser);
                jsonparse_next(parser);
                
                mac_len = jsonparse_get_value_as_int(parser);
                ESP_DBG("mac_len=%d\n",mac_len);
            
            }else if(jsonparse_strcmp_value(parser, "test") == 0){
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, mac, sizeof(mac));
                ESP_DBG("test: %s \n",mac);
            }
        
        }
    }
    ESP_DBG("Type=%d\n",type);
    return 1;
#endif
}
LOCAL int ICACHE_FLASH_ATTR
button_status_get(struct jsontree_context *js_ctx)
{
   ESP_DBG("button get\n");
   return 1;
}

LOCAL struct jsontree_callback button_callback =
    JSONTREE_CALLBACK(button_status_get, button_status_set);

JSONTREE_OBJECT(button_remove_tree,
              JSONTREE_PAIR("mac_len", &button_callback),
              JSONTREE_PAIR("test", &button_callback),
                 );

JSONTREE_OBJECT(button_new_tree,
              JSONTREE_PAIR("temp_key", &button_callback),
              JSONTREE_PAIR("button_mac", &button_callback),
                 );

JSONTREE_OBJECT(buttonPairingInfom,
                JSONTREE_PAIR("mdev_mac", &button_callback),
                JSONTREE_PAIR("button_new", &button_new_tree),
                 JSONTREE_PAIR("replace", &button_callback),
                 JSONTREE_PAIR("button_remove", &button_remove_tree),
                //JSONTREE_PAIR("switches", &switch_tree)
                );
JSONTREE_OBJECT(button_info_tree,
               JSONTREE_PAIR("button_info", &buttonPairingInfom));


void ICACHE_FLASH_ATTR
user_EspPlatformPorcessJSONMsg(struct espconn* pespconn,char *pusrdata, unsigned short length)
{

	ESP_DBG("JSON RCV: \r\n%s\r\n",pusrdata);
	ESP_DBG("-------------------\r\n");
	char *pstr = NULL;
    #if ESP_MESH_SUPPORT
		//char pbuffer[1024 * 2] = {0};//It is supposed that in mesh solution, we will receive the entire packet each time
		char* pbuffer = pusrdata;
    #else
		LOCAL char pbuffer[1024 * 2] = {0};
    #endif
	
    uint8* src_addr = NULL,*dst_addr = NULL;
    #if MESH_HEADER_MODE
	struct mesh_header_format* mesh_header = (struct mesh_header_format*)pespconn->reverse;;
	espconn_mesh_get_src_addr(mesh_header,&src_addr);
	espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
	ESP_DBG("GET SRC/DST in user_EspPlatformPorcessJSONMsg\r\n");
	ESP_DBG("SRC: "MACSTR"\r\n",MAC2STR(src_addr));
	ESP_DBG("DST: "MACSTR"\r\n",MAC2STR(dst_addr));
	#endif	
	//struct espconn *pespconn = arg;
	if (length >= 1460) { //BUG : RCV 1640
    #if ESP_MESH_SUPPORT
		ESP_DBG("--------------------\r\n"); 
		ESP_DBG("GO TO MESH PARSE\r\n");
		ESP_DBG("--------------------\r\n");
		goto PARSE;
    #endif
		os_memcpy(pbuffer, pusrdata, length);
	} else {

	
	PARSE:
    #if !ESP_MESH_SUPPORT
		os_memcpy(pbuffer + os_strlen(pbuffer), pusrdata, length);
    #endif

		
    #if (PLUG_DEVICE || LIGHT_DEVICE)
			//os_timer_disarm(&beacon_timer);
        #if ESP_MESH_SUPPORT
			mesh_StopReconnCheck();
        #endif

		#if ESP_DEBUG_MODE
			char* ptime = (char*)os_strstr(pbuffer,"datetime");
		
			if(ptime){
				os_memset(time_stamp_str,0,sizeof(time_stamp_str));
				os_memcpy(time_stamp_str,ptime+os_strlen("\"datetime\": "),19);
				ESP_DBG("GET TIME: %s \r\n",time_stamp_str);
			}
		#endif
    #endif
		
    #if ESP_SIMPLE_PAIR_SUPPORT
			 if(os_strstr(pbuffer,PAIR_START_PATH))
			{
				ESP_DBG("~~~~~~~~~~\r\n");
				ESP_DBG("start pairing mode \r\n");
				ESP_DBG("~~~~~~~~~~\r\n");
	
        #if ESP_MESH_SUPPORT //data from iot platfom,not mesh ?
		        #if MESH_HEADER_MODE
				//extern uint8 pair_src_addr[ESP_MESH_ADDR_LEN];
				extern uint8 pair_dst_addr[ESP_MESH_ADDR_LEN];
                //if(src_addr) os_memcpy(pair_src_addr,src_addr,ESP_MESH_ADDR_LEN);
				if(src_addr) os_memcpy(pair_dst_addr,src_addr,ESP_MESH_ADDR_LEN);
				#else
				extern char* sip;
				extern char* sport;
				sip = (char *)os_strstr(pusrdata, ESP_MESH_SIP_STRING);
				sport = (char *)os_strstr(pusrdata, ESP_MESH_SPORT_STRING);
				if(sip) {
					os_bzero(pair_sip,sizeof(pair_sip));
					os_memcpy(pair_sip,sip,ESP_MESH_JSON_IP_ELEM_LEN);
					ESP_DBG("pair_sip,%d: %s \r\n",os_strlen(pair_sip),pair_sip);
				}
				if(sport){
					os_bzero(pair_sport,sizeof(pair_sport));
					os_memcpy(pair_sport,sport,ESP_MESH_JSON_PORT_ELEM_LEN);
					ESP_DBG("pair_sport,%d: %s \r\n",os_strlen(pair_sport),pair_sport);
				}
				
                #endif
        #endif
				ESP_DBG("Recv Button inform\n");
				struct jsontree_context js;
				jsontree_setup(&js, (struct jsontree_value *)&button_info_tree, json_putchar);
				json_parse(&js, pbuffer);
				//response_send(ptrespconn, true);
				char data_resp[100];
				os_bzero(data_resp,sizeof(data_resp));
				#if STRING_SV_IN_FLASH
				char button_config[100];
				load_string_from_flash(button_config,sizeof(button_config),(void*)BUTTON_CONFIG);
				os_sprintf(data_resp, button_config);
				#else
				os_sprintf(data_resp, BUTTON_CONFIG);
				#endif
				ESP_DBG("response send str...\r\n");
				
				//response_send_str(void *arg, bool responseOK,char* pJsonDataStr , int str_len,char* url,uint16 url_len,uint8 tag_if,uint8 header_if)
				response_send_str(pespconn, true, data_resp,os_strlen(data_resp),PAIR_START_PATH,os_strlen(PAIR_START_PATH),1,0,mesh_src_addr,pair_dst_addr);
				buttonPairingInfo.simple_pair_state=USER_PBULIC_BUTTON_INFO;
				sp_LightPairState();
				return;

			 }
			else if(os_strstr(pbuffer,PAIR_FOUND_REQUEST))
			{
				ESP_DBG("PAIR_FOUND_REQUEST \r\n");
				if((char*)os_strstr(pbuffer,"200")!=NULL){
					ESP_DBG("device can pair\n");
					buttonPairingInfo.simple_pair_state=USER_PERMIT_SIMPLE_PAIR;
				}
				else if((char*)os_strstr(pbuffer,"403")){
					ESP_DBG("device cannot pair\n");
					buttonPairingInfo.simple_pair_state=USER_REFUSE_SIMPLE_PAIR;
				}
				sp_LightPairState();
				return;
			}
			else if(os_strstr(pbuffer,PAIR_RESULT))
			{
				if((char*)os_strstr(pbuffer,"200")!=NULL){
					ESP_DBG("device pair stop\n");
					buttonPairingInfo.simple_pair_state=USER_CONFIG_STOP_SIMPLE_PAIR;
				}
				else if((char*)os_strstr(pbuffer,"100")){
					ESP_DBG("device pair continue\n");
					buttonPairingInfo.simple_pair_state=USER_CONFIG_CONTIUE_SIMPLE_PAIR;
				}
				return;
			}				
			else if(os_strstr(pbuffer,PAIR_KEEP_ALIVE)){
				ESP_DBG("PAIR KEEP ALIVE: \r\n");
				sp_LightPairReplyKeepAlive(pespconn);
				return;
			}
    #endif
	
        #if ESP_DEBUG_MODE
			if(NULL == (char*)os_strstr(pbuffer,"download_rom_base64")){
			//if(1){
				ESP_DBG("========================\r\n");
				ESP_DBG("user_esp_platform_recv_cb \r\n");
				ESP_DBG("RCV LEN: %d \r\n",length);
				ESP_DBG("------------------\r\n");
				ESP_DBG("%s \r\n",pbuffer);
				ESP_DBG("========================\r\n");
			}else{
				ESP_DBG("========================\r\n");
				ESP_DBG("platform recv: %d \r\n",length);
			}
        #endif
	
			if ((pstr = user_json_find_section(pbuffer, length, "\"activate_status\":")) != NULL &&
				user_esp_platform_parse_nonce(pbuffer, length) == active_nonce) {			 
				if (os_strncmp(pstr, "1", 1) == 0) {
					ESP_DBG("\r\n\r\n===========================\r\n");
					ESP_DBG("device activates successful.\n");
					ESP_DBG("===========================\r\n\r\n");
					device_status = DEVICE_ACTIVE_DONE;
					esp_param.activeflag = 1;
					system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
					user_esp_platform_sent(pespconn);
					user_esp_platform_sent_beacon(pespconn);
				} else {
					ESP_DBG("\r\n\r\n===========================\r\n");
					ESP_DBG("device activates failed.\n");
					ESP_DBG("\r\n\r\n===========================\r\n");
					device_status = DEVICE_ACTIVE_FAIL;
				}
			}
		
#if (PLUG_DEVICE || LIGHT_DEVICE)
			else if ((pstr = user_json_find_section(pbuffer, length, "\"action\"")) != NULL) {
				int nonce = user_esp_platform_parse_nonce(pbuffer, length);
				if (!os_memcmp(pstr, "sys_upgrade", os_strlen("sys_upgrade"))) {  // upgrade start
					if(UPGRADE_FLAG_START == system_upgrade_flag_check()){
						ESP_DBG("Under Upgrading...return...\r\n");
						user_platform_rpc_set_rsp(pespconn, nonce, 401);
						return;
					}
					ESP_DBG("upgrade start....!!!\r\n");
					ota_start_time = system_get_time();
					esp_param.ota_start_time = ota_start_time;
					esp_param.ota_finish_time = ota_start_time;
					system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
					if ((pstr = user_json_find_section(pbuffer, length, "\"version\"")) != NULL) {
                    #if ESP_MESH_SUPPORT
    					if(src_addr) os_memcpy(ota_dst_addr,src_addr,ESP_MESH_ADDR_LEN);
						if (espconn_mesh_get_status() == MESH_DISABLE) {  // normal upgrade start
							user_esp_upgrade_begin(pespconn, nonce, pstr);
						} else {  // mesh upgrade start
							user_esp_mesh_upgrade_init(pespconn, nonce, pbuffer, length);
						}
                    #else
							user_esp_upgrade_begin(pespconn, nonce, pstr);
                    #endif
					} else {
						user_platform_rpc_set_rsp(pespconn, nonce, 400);
					}
				}
				else if (!os_memcmp(pstr, "download_rom_base64", os_strlen("download_rom_base64"))) {
					if ((pstr = user_json_find_section(pbuffer, length, "\"device_rom\"")) != NULL &&
						(pstr = user_json_find_section(pstr, length, "\"rom_base64\"")) != NULL) {
						if(system_upgrade_flag_check()==UPGRADE_FLAG_START){
                        #if ESP_MESH_SUPPORT
							user_esp_mesh_upgrade(pespconn, pbuffer, length, pstr);
                        #endif
						}else{
							ESP_DBG("OTA STATUS ERROR: %d \r\n",system_upgrade_flag_check());
							user_platform_rpc_set_rsp(pespconn, nonce, 400);
						}
					} else {
						user_platform_rpc_set_rsp(pespconn, nonce, 400);
					}
				} 
				else if (!os_memcmp(pstr, "upgrade_cancel", os_strlen("upgrade_cancel"))) { 
					system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
					
#if ESP_MESH_SUPPORT
					os_timer_disarm(&g_mesh_upgrade_timer);
					mesh_upgrade_check_func(pespconn);
#endif
					ESP_DBG("Upgrade stopped...\r\n");
					user_platform_rpc_set_rsp(pespconn, nonce, 200);
				}
				else if (!os_memcmp(pstr, "sys_reboot", os_strlen("sys_reboot"))) {
					if(system_upgrade_flag_check() == UPGRADE_FLAG_FINISH){
						user_platform_rpc_set_rsp(pespconn, nonce, 200);
						ESP_DBG("upgrade reboot..");
						UART_WaitTxFifoEmpty(0,50000);
						//espconn_mesh_setup_timer(&client_timer, 100,(os_timer_func_t *)system_upgrade_reboot, NULL, 0);
                    #if ESP_MESH_SUPPORT
						os_timer_disarm(&client_timer);
						//os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
						os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_mesh_upgrade_reboot, NULL);
						os_timer_arm(&client_timer, 1000, 0);
					#else
						os_timer_disarm(&client_timer);
						os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
						//os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_mesh_upgrade_reboot, NULL);
						os_timer_arm(&client_timer, 1000, 0);
					#endif
					}else{
						user_platform_rpc_set_rsp(pespconn, nonce, 400);
						ESP_DBG("upgrade not finished..\r\n");
					}
				} else if(!os_memcmp(pstr, "get_switches", os_strlen("get_switches"))){
					ESP_DBG("GET BUTTON STATUS CMD:\r\n");
					char pbuf[512];
					os_memset(pbuf,0,sizeof(pbuf));
					json_build_packet(pbuf, BATTERY_STATUS);
					user_platform_rpc_battery_rsp(pespconn,pbuf,sizeof(pbuf), nonce, 200);
					ESP_DBG("debug cmd switches:\r\n %s \r\n",pbuf);
				} 
#if ESP_DEBUG_MODE
				else if(!os_memcmp(pstr, "force_reboot", os_strlen("force_reboot"))){
					ESP_DBG("force reboot...system_restart();\r\n");
					system_restart();
				}			
				else if(!os_memcmp(pstr, "mesh_version", os_strlen("mesh_version"))){
					ESP_DBG("MESH TEST VERSION: %d \r\n",MESH_TEST_VERSION);
					user_platform_rpc_set_rsp(pespconn, nonce, 200+MESH_TEST_VERSION);
				}
				else if(!os_memcmp(pstr, "ota_dbg_reboot", os_strlen("ota_dbg_reboot"))){
					if((system_upgrade_flag_check() == UPGRADE_FLAG_FINISH) || (1==user_esp_get_upgrade_finish_flag())){
						user_platform_rpc_set_rsp(pespconn, nonce, 200);
						ESP_DBG("debug upgrade reboot..");
						UART_WaitTxFifoEmpty(0,50000);
						light_shadeStart(HINT_GREEN,500,0,0,NULL);
						debug_OtaRebootStart(7000);
					}else{
						user_platform_rpc_set_rsp(pespconn, nonce, 400);
						ESP_DBG("upgrade not finished..\r\n");
					}
				}
				else if(!os_memcmp(pstr, "test", os_strlen("test"))){
					ESP_DBG("GET BUTTON STATUS CMD:\r\n");
					char pbuf[512];
					os_memset(pbuf,0,sizeof(pbuf));
					json_build_packet(pbuf, BATTERY_STATUS);
					user_platform_rpc_battery_rsp(pespconn,pbuf,sizeof(pbuf), nonce, 200);
					ESP_DBG("debug cmd test:\r\n %s \r\n",pbuf);
				}
#endif
				else if(!os_memcmp(pstr, "multicast", os_strlen("multicast"))){
					if((pstr = user_json_find_section(pbuffer, length, "\"method\"")) != NULL){
						ESP_DBG("test find section: %s\r\n",pstr);
						
						if (!os_memcmp(pstr, "GET", os_strlen("GET"))) {
							ESP_DBG("FIND GET...\r\n");
							user_esp_platform_get_info(pespconn, pbuffer);
						} else if (!os_memcmp(pstr, "POST", os_strlen("POST"))) {
							ESP_DBG("FIND POST...\r\n");
							user_esp_platform_set_info(pespconn, pbuffer);
						}
					}
				}else {
					user_platform_rpc_set_rsp(pespconn, nonce, 400);
				}
			} else if ((pstr = (char *)os_strstr(pbuffer, "/v1/device/timers/")) != NULL) {
				int nonce = user_esp_platform_parse_nonce(pbuffer, length);
				user_platform_rpc_set_rsp(pespconn, nonce, 200);
				os_timer_disarm(&client_timer);
				os_timer_setfn(&client_timer, (os_timer_func_t *)user_platform_timer_get, pespconn);
				os_timer_arm(&client_timer, 2000, 0);
			} else if ((pstr = (char *)os_strstr(pbuffer, "\"method\": ")) != NULL) {
				if (os_strncmp(pstr + 11, "GET", 3) == 0) {
					ESP_DBG("FIND GET...\r\n");
					user_esp_platform_get_info(pespconn, pbuffer);
				} else if (os_strncmp(pstr + 11, "POST", 4) == 0) {
					ESP_DBG("FIND POST...\r\n");
					user_esp_platform_set_info(pespconn, pbuffer);
				}
			} else if ((pstr = (char *)os_strstr(pbuffer, "ping success")) != NULL) {
				ESP_DBG("ping success\n");
				ping_status = 1;
			} else if ((pstr = (char *)os_strstr(pbuffer, "send message success")) != NULL) {
			} else if ((pstr = (char *)os_strstr(pbuffer, "timers")) != NULL) {
				user_platform_timer_start(pusrdata , pespconn); //need to check later
			}
	
#elif SENSOR_DEVICE
			else if ((pstr = (char *)os_strstr(pbuffer, "\"status\":")) != NULL) {
				if (os_strncmp(pstr + 10, "200", 3) != 0) {
					ESP_DBG("message upload failed.\n");
				} else {
					count++;
					ESP_DBG("message upload sucessful.\n");
				}
	
				os_timer_disarm(&client_timer);
				os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_discon, pespconn);
				os_timer_arm(&client_timer, 10, 0);
			}
	
#endif
			else if ((pstr = (char *)os_strstr(pbuffer, "device")) != NULL) {
#if PLUG_DEVICE || LIGHT_DEVICE
				user_platform_timer_get(pespconn);
#elif SENSOR_DEVICE
#endif
			}
		}
}

//revc callback 
void ICACHE_FLASH_ATTR
user_esp_platform_recv_cb(void* arg,char* pdata,unsigned short len) 
{
	struct espconn* pespconn = (struct espconn*)arg;
    int i;
    uint8* usr_data = NULL;
	uint16 usr_data_len = 0;
    #if (MESH_HEADER_MODE&&ESP_MESH_SUPPORT)
    
    	struct mesh_header_format* mesh_header = NULL;
    	enum mesh_usr_proto_type proto = M_PROTO_NONE;
    	#if ESP_DEBUG_MODE
            #if MESH_HEADER_MODE
         		ESP_DBG("RCV MESH HEADER:\r\n");
         		int k=0;
         		for(k=0;k<8;k++) {
         			ESP_DBG("%02x %02x %02x %02x \r\n",pdata[k*4+0],pdata[k*4+1],pdata[k*4+2],pdata[k*4+3]);
         		}
            #endif
    	#endif
    
    	mesh_header = (struct mesh_header_format*)pdata;
    	if(!espconn_mesh_get_usr_data(mesh_header,&usr_data,&usr_data_len)){ 
    		ESP_DBG("get user data fail\n"); 
    		ESP_DBG("data length: %d \r\n",len);
    		for(i=0;i<len;i++){
                ESP_DBG("%02x ",pdata[i]);
    			if(i%8==0) ESP_DBG("\r\n");
    		}
    		return; 
    	}
    	
    	if(!espconn_mesh_get_usr_data_proto(mesh_header,&proto)){ 
    		ESP_DBG("get user data proto fail\n"); 
    		return; 
    	}
    	uint8 *src_addr = NULL;
    	uint8 *dst_addr = NULL;
        #if MESH_HEADER_MODE
    	espconn_mesh_get_src_addr(mesh_header,&src_addr);
    	espconn_mesh_get_dst_addr(mesh_header,&dst_addr);
    	
    	ESP_DBG("GET SRC/DST in user_esp_platform_recv_cb\r\n");
    	ESP_DBG("SRC: "MACSTR"\r\n",MAC2STR(src_addr));
    	ESP_DBG("DST: "MACSTR"\r\n",MAC2STR(dst_addr));
        #endif
    	pespconn->reverse = mesh_header;    
    	switch(proto){
    		case M_PROTO_JSON:
    			ESP_DBG("type M_PROTO_JSON\r\n");
    			user_EspPlatformPorcessJSONMsg(pespconn,usr_data, usr_data_len); //swap src and dst to response ;
    			break;
    
    		case M_PROTO_HTTP:
    			ESP_DBG("type M_PROTO_HTTP\r\n");
    			webserver_recv(pespconn, usr_data, usr_data_len);
    			break;
    			
    		default:
    			break;
    	}
    #else
       user_EspPlatformPorcessJSONMsg(pespconn,pdata, len); //swap src and dst to response ;
    #endif
}



#if AP_CACHE
/******************************************************************************
 * FunctionName : user_esp_platform_ap_change
 * Description  : add the user interface for changing to next ap ID.
 * Parameters   :
 * Returns      : none
*******************************************************************************/
//not used in MESH mode
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_ap_change(void)
{
    uint8 current_id;
    uint8 i = 0;
    ESP_DBG("user_esp_platform_ap_is_changing\n");

    current_id = wifi_station_get_current_ap_id();
    ESP_DBG("current ap id =%d\n", current_id);

    if (current_id == AP_CACHE_NUMBER - 1) {
       i = 0;
    } else {
       i = current_id + 1;
    }
    while (wifi_station_ap_change(i) != true) {
       i++;
       if (i == AP_CACHE_NUMBER - 1) {
           i = 0;
       }
    }

    /* just need to re-check ip while change AP */
    device_recon_count = 0;
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
    os_timer_arm(&client_timer, 100, 0);
}
#endif

//not used in MESH mode
LOCAL bool ICACHE_FLASH_ATTR
user_esp_platform_reset_mode(void)
{
    ESP_DBG("user_esp_platform_reset_mode\r\n");
    if (wifi_get_opmode() != STATIONAP_MODE) {
        wifi_set_opmode(STATIONAP_MODE);
    }
    #if AP_CACHE
    /* delay 5s to change AP */
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_ap_change, NULL);
    os_timer_arm(&client_timer, 5000, 0);
    return true;
    #endif
	
    return false;
}

//1.if there is a reconn event, mesh root device would tell all sub-devices
//2.if there is a sub-device joined in, tell him the sending status.
void ICACHE_FLASH_ATTR
	user_esp_platform_update_sending_status(struct espconn* pconn,ESP_SEND_STATUS status,MeshSendType sendType,uint8* dev_mac)
{
    char* bcast_data = (char*)os_zalloc(300);
    if(bcast_data==NULL) return;

    struct ip_info ipconfig;
    wifi_get_ip_info(STATION_IF, &ipconfig);

	
	uint8 mac_sta[6] = {0};
	wifi_get_macaddr(STATION_IF, mac_sta);


    bool p2p_msg = false;
    #if MESH_HEADER_MODE
		uint8 dst[6],src[6];
		os_memset(dst,0,6);
		wifi_get_macaddr(STATION_IF,src);	
	
    if(sendType == MESH_BROADCAST){
		#if STRING_SV_IN_FLASH
		char pbuf_tmp[300];
		load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)BCAST_MESH_SENDING_STATUS);
        os_sprintf(bcast_data,pbuf_tmp,status,MAC2STR(mac_sta),IP2STR(&ipconfig.ip));
		#else
        os_sprintf(bcast_data,BCAST_MESH_SENDING_STATUS,status,MAC2STR(mac_sta),IP2STR(&ipconfig.ip));
		#endif
	    p2p_msg = false;
    }else if((sendType == MESH_UNICAST) && (dev_mac!= NULL)){
        p2p_msg = true;
		os_memcpy(dst,dev_mac,ESP_MESH_ADDR_LEN);
		#if STRING_SV_IN_FLASH
        char unicast_mesh_status[200];
		load_string_from_flash(unicast_mesh_status,sizeof(unicast_mesh_status),(void*)unicast_mesh_status);
        os_sprintf(bcast_data,unicast_mesh_status,MAC2STR(dev_mac),MAC2STR(mac_sta),status,IP2STR(&ipconfig.ip));
		#else
        os_sprintf(bcast_data,UNICAST_MESH_SENDING_STATUS,MAC2STR(dev_mac),MAC2STR(mac_sta),status,IP2STR(&ipconfig.ip));
		#endif
    }else{
        ESP_DBG("wrong type...\r\n");
        return;
    }
	#else
	uint8* dst = NULL,*src=NULL;
    #endif

	#if STRING_SV_IN_FLASH
    char bcast_url[200];
	load_string_from_flash(bcast_url,sizeof(bcast_url),(void*)BCAST_URL);
    data_send_buf(pconn, true, bcast_data,os_strlen(bcast_data),bcast_url,os_strlen(bcast_url),src,dst,p2p_msg);	
	#else
    data_send_buf(pconn, true, bcast_data,os_strlen(bcast_data),BCAST_URL,os_strlen(BCAST_URL),src,dst,p2p_msg);
    #endif
	os_free(bcast_data);
    bcast_data = NULL;
}

#if 1

//P2P msg sending to sub-node that joined in.
//1.if sub-node device joined in,send p2p msg to the sub-node to share the button function
//2.if received a button configure command,broadcast to other devices in the same mesh group
void ICACHE_FLASH_ATTR
	user_esp_platform_button_function_status(struct espconn* pconn,uint8* dev_mac,MeshSendType sendType)
{
#if ESP_SIMPLE_PAIR_SUPPORT
    ESP_DBG("user_esp_platform_button_function_status\r\n");
	char data_buf[500];
    #if MESH_HEADER_MODE
		uint8 dst[6];
	    uint8 src[6];
		os_memset(dst,0,6);
		wifi_get_macaddr(STATION_IF,src);		
    #endif
	
	#if ESP_SIMPLE_PAIR_SUPPORT
	UserResponseFuncMap(data_buf,sizeof(data_buf));
	#endif
	char resp_element[50];
	os_sprintf(resp_element,"\"response\":0");
	if (!mesh_json_add_elem(data_buf, sizeof(data_buf), resp_element, os_strlen(resp_element))) {
		ESP_DBG("add response option error\r\n");
		return;
	}
    bool p2p_msg = false;
    if(sendType == MESH_BROADCAST ){
	    p2p_msg = false;
		ESP_DBG("BROADCAST MSG\r\n");
	}else if(sendType == MESH_UNICAST){
	    p2p_msg = true;
		os_memcpy(dst,dev_mac,ESP_MESH_ADDR_LEN);
		ESP_DBG("P2P MSG\r\n");
	}else{
        ESP_DBG("wrong type...\r\n");
        return;
	}
	ESP_DBG("debug data_buf: \r\n%s",data_buf);
    data_send_buf(pconn, true, data_buf,os_strlen(data_buf),USER_SET_CMD_ACTION,os_strlen(USER_SET_CMD_ACTION),src,dst,p2p_msg);
#endif
}
#endif

/******************************************************************************
 * FunctionName : user_esp_platform_recon_cb
 * Description  : The connection had an error and is already deallocated.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_recon_cb(void *arg, sint8 err)
{
    struct espconn *pespconn = (struct espconn *)arg;
    ESP_DBG("user_esp_platform_recon_cb\n");
	ESP_DBG("reconn time: %s \r\n",time_stamp_str);
    #if ESP_DEBUG_MODE
    //save reconn time and log
	debug_FlashSvMeshReconInfo(time_stamp_str,err);
    #endif

    #if ESP_MESH_SUPPORT
    //discard all mesh packets to server
	espconn_mesh_release_congest();
	//discard all user packet in the tx queue
	espSendFlush(espSendGetRingbuf());

    if(mesh_root_if()){
		ESP_DBG("mesh root, flush mesh ext queue, update status\r\n");
		//switch to local mode, will not send data to server
		espSendSetStatus(ESP_LOCAL_ACTIVE);
        //broadcast to all other devices
		user_esp_platform_update_sending_status(pespconn,ESP_LOCAL_ACTIVE,MESH_BROADCAST,(char*)mesh_GetMdevMac());
    }else{
        ESP_DBG("mesh subnode, do nothing...\r\n");
    }
    #endif
    #if (PLUG_DEVICE || LIGHT_DEVICE)
    os_timer_disarm(&beacon_timer);
    #endif

    #if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_output(1);
    #endif

    if (++device_recon_count == 5) {
        device_status = DEVICE_CONNECT_SERVER_FAIL;
        #if ESP_MESH_SUPPORT==0
        if (user_esp_platform_reset_mode()) {
            return;
        }
        #endif
    }

#if SENSOR_DEVICE
#ifdef SENSOR_DEEP_SLEEP
    if (wifi_get_opmode() == STATION_MODE) {
        user_esp_platform_reset_mode();
        //user_sensor_deep_sleep_enter();
    } else {
        os_timer_disarm(&client_timer);
        os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
        os_timer_arm(&client_timer, 1000, 0);
    }

#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
#endif
#else
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_reconnect, pespconn);
    os_timer_arm(&client_timer, 1000, 0);
#endif
}

/******************************************************************************
 * FunctionName : user_esp_platform_connect_cb
 * Description  : A new incoming connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect_cb(void *arg)
{
    struct espconn *pespconn = arg;
    ping_status = 1;
    ESP_DBG("user_esp_platform_connect_cb\n");
    ESP_DBG("=======================\r\n");
    ESP_DBG("test in connect cb\r\n");
    ESP_DBG("==============\r\n");
    ESP_DBG("CONN STATUS : %d , %p\r\n",pespconn->state , pespconn);
    ESP_DBG("=======================\r\n");

    #if ESP_MESH_SUPPORT
	mesh_StopReconnCheck();
	espconn_mesh_release_congest();
	espSendFlush(espSendGetRingbuf());
	
    if(mesh_root_if()){
		ESP_DBG("mesh root, flush mesh ext queue, update status\r\n");
		espSendSetStatus(ESP_SERVER_ACTIVE);
		user_esp_platform_update_sending_status(pespconn,ESP_SERVER_ACTIVE,MESH_BROADCAST,NULL);
    }else{
        //espconn_mesh_release_congest();
        ESP_DBG("mesh subnode, do nothing...\r\n");
    }
	#endif
		
    if (wifi_get_opmode() ==  STATIONAP_MODE ) {
    }
    #if ESP_DEBUG_MODE
    debug_UploadExceptionInfo(pespconn);
    #endif

#if (PLUG_DEVICE || SENSOR_DEVICE)
    user_link_led_timer_done();
#endif
    device_recon_count = 0;
    user_esp_platform_sent(pespconn);
}


void ICACHE_FLASH_ATTR
    user_esp_platform_sent_data()
{
    user_esp_platform_sent(&user_conn);

}
/******************************************************************************
 * FunctionName : user_esp_platform_connect
 * Description  : The function given as the connect with the host
 * Parameters   : espconn -- the espconn used to connect the connection
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_connect(struct espconn *pespconn)
{
    ESP_DBG("user_esp_platform_connect\n");

    #if ESP_MESH_SUPPORT
	
    #if (PLUG_DEVICE || LIGHT_DEVICE)
        ping_status = 1;
    #endif
    ESP_DBG("==============\r\n");
    ESP_DBG("CONN STATUS : %d , %p\r\n",pespconn->state , pespconn);
    ESP_DBG("mesh connect server IP: "IPSTR"\r\n",IP2STR(pespconn->proto.tcp->remote_ip));
	ESP_DBG("mesh connect server PORT: %d \r\n",pespconn->proto.tcp->remote_port);
	
	ESP_DBG("MESH STATUS : %d \r\n",espconn_mesh_get_status());
	if(MESH_ONLINE_AVAIL != espconn_mesh_get_status()){
		ESP_DBG("--------------\r\n");
		ESP_DBG("TRY TO ENABLE MESH ONLINE MODE\r\n");
		ESP_DBG("--------------\r\n");
		mesh_StartReconnCheck(1000);
		return;
	}else{
        ESP_DBG("USER CALL espconn_mesh_connect,MESH STATUS: %d\r\n",espconn_mesh_get_status());
        espconn_mesh_connect(pespconn);
		mesh_StartReconnCheck(20000);
	}
    #else
        #ifdef CLIENT_SSL_ENABLE
            espconn_secure_connect(pespconn);
        #else
            espconn_connect(pespconn);
        #endif
    #endif
}

#ifdef USE_DNS
/******************************************************************************
 * FunctionName : user_esp_platform_dns_found
 * Description  : dns found callback
 * Parameters   : name -- pointer to the name that was looked up.
 *                ipaddr -- pointer to an ip_addr_t containing the IP address of
 *                the hostname, or NULL if the name could not be found (or on any
 *                other error).
 *                callback_arg -- a user-specified callback argument passed to
 *                dns_gethostbyname
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    static int mesh_reinit_flg = 0;

    ESP_DBG("IN DNS FOUND:\r\n esp_server_ip.addr: %08x \r\n",esp_server_ip.addr);
    if (ipaddr == NULL) {
        ESP_DBG("user_esp_platform_dns_found NULL,RECON CNT:%d \r\n",device_recon_count);
        if (++device_recon_count == 5) {
            device_status = DEVICE_CONNECT_SERVER_FAIL;
            #if ESP_MESH_SUPPORT==0
            user_esp_platform_reset_mode();
            #endif
        }
        return;
    }
    ESP_DBG("user_esp_platform_dns_found %d.%d.%d.%d\n",
            *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
            *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));

    if (esp_server_ip.addr == 0 && ipaddr->addr != 0) {
        os_timer_disarm(&client_timer);
        esp_server_ip.addr = ipaddr->addr;
        os_memcpy(pespconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
        pespconn->proto.tcp->local_port = espconn_port();
    #ifdef CLIENT_SSL_ENABLE
        pespconn->proto.tcp->remote_port = 8443;
    #else
        pespconn->proto.tcp->remote_port = ESP_SERVER_PORT;
    #endif

    #if (PLUG_DEVICE || LIGHT_DEVICE)
        ping_status = 1;
    #endif
        user_esp_platform_connect(pespconn);
    }
}

/******************************************************************************
 * FunctionName : user_esp_platform_dns_check_cb
 * Description  : 1s time callback to check dns found
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_check_cb(void *arg)
{
#if ESP_MESH_SUPPORT
    struct ip_info ipconfig;
    struct espconn *pespconn = arg;
    wifi_get_ip_info(STATION_IF, &ipconfig);
    os_timer_disarm(&client_timer);
    ESP_DBG("user_esp_platform_dns_check_cb, ip:" IPSTR "server:" IPSTR "\n", IP2STR(&ipconfig.ip.addr), IP2STR(&esp_server_ip.addr));
    ESP_DBG("IPADDR_ANY : %08x ; %d\r\n",IPADDR_ANY,(ipconfig.ip.addr == IPADDR_ANY));
    
    if (ipconfig.ip.addr == IPADDR_ANY || !espconn_mesh_local_addr(&ipconfig.ip)) {
        ESP_DBG("TEST IN NORMAL BRANCH\r\n");
        if (esp_server_ip.addr == IPADDR_ANY) {
            ESP_DBG("SERVER IP ZERO\r\n");
            espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);
            os_timer_disarm(&client_timer);
            if(MESH_DISABLE == espconn_mesh_get_status()){
                ESP_DBG("MESH DISABLE , RETURN...\r\n");
                return;
            }
            if(device_recon_count<5){
                os_timer_arm(&client_timer, 5000, 0);
            }else{
                os_timer_arm(&client_timer, 20000, 0);
            }
        } else {
            user_esp_platform_dns_found(ESP_DOMAIN, &esp_server_ip, pespconn);
        }
    } else { 
        ESP_DBG("TEST IN IP GW\r\n");
        user_esp_platform_dns_found(ESP_DOMAIN, &ipconfig.gw, pespconn);
    }
#else
    struct espconn *pespconn = arg;
    ESP_DBG("user_esp_platform_dns_check_cb\n");
    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);
    os_timer_arm(&client_timer, 1000, 0);
#endif
}

//connect esp-server domain
LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_start_dns(struct espconn *pespconn)
{
    struct ip_info ipconfig;
    esp_server_ip.addr = 0;
    ESP_DBG("user_esp_platform_start_dns\n");
    wifi_get_ip_info(STATION_IF, &ipconfig);
    #if ESP_MESH_SUPPORT
    if (esp_server_ip.addr == 0 && !espconn_mesh_local_addr(&ipconfig.ip)) {
        esp_server_ip.addr = 0;
        espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);
    }
    #else
    espconn_gethostbyname(pespconn, ESP_DOMAIN, &esp_server_ip, user_esp_platform_dns_found);
    #endif

    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_dns_check_cb, pespconn);
    if(device_recon_count<5){
        os_timer_arm(&client_timer, 1000, 0);
    }else{
        //if failed 5 times, 
        os_timer_arm(&client_timer, 10000, 0);
    }
}
#endif


#if ESP_MDNS_SUPPORT

#if LIGHT_DEVICE
struct mdns_info *mdns_info=NULL;
//initialize for mdns server
void ICACHE_FLASH_ATTR
    user_mdns_conf()
{
    ESP_DBG("%s\r\n",__func__);
    wifi_set_broadcast_if(STATIONAP_MODE);
    espconn_mdns_close();
    struct ip_info ipconfig;
    wifi_get_ip_info(STATION_IF, &ipconfig);
    if(mdns_info == NULL){
        mdns_info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
    }else{
        os_memset(mdns_info,0,sizeof(struct mdns_info));
    }
    mdns_info->host_name = "ESP_LIGHT";
    mdns_info->ipAddr= ipconfig.ip.addr; //sation ip
    mdns_info->server_name = "ESP_MESH";
    mdns_info->server_port = SERVER_PORT;
    mdns_info->txt_data[0] = "version = 1.2.0";
    mdns_info->txt_data[1] = "vendor = espressif";
    mdns_info->txt_data[2] = "mesh_support = 1";
    mdns_info->txt_data[3] = "PWM channel = 5";
    espconn_mdns_init(mdns_info);
    espconn_mdns_server_register();
}
#endif

#endif
/******************************************************************************
 * FunctionName : user_esp_platform_check_ip
 * Description  : espconn struct parame init when get ip addr
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_esp_platform_check_ip(uint8 reset_flag)
{
    struct ip_info ipconfig;
    uint8_t wifi_status = STATION_IDLE;//
    os_timer_disarm(&client_timer);
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    #if (PLUG_DEVICE || SENSOR_DEVICE)
        user_link_led_timer_init();
    #endif
    #if LIGHT_DEVICE
       #if ESP_MDNS_SUPPORT
            user_mdns_conf();
        #endif
    #endif
    #if ESP_MESH_SUPPORT
	//connected to router , enable mesh or connect server
        ESP_DBG("STA STATUS: %d ;  ip: %d \r\n",wifi_station_get_connect_status(),ipconfig.ip.addr);
        if(MESH_DISABLE == espconn_mesh_get_status() ){
            ESP_DBG("======================\r\n");
            ESP_DBG("CONNECTED TO ROUTER, ENABLE MESH\r\n");
            ESP_DBG("======================\r\n");
    		espconn_mesh_enable(mesh_EnableCb,MESH_ONLINE);
			#if ESP_DEBUG_MODE
			light_shadeStart(HINT_YELLOW,500,6,2,NULL);
			#endif
        }else{
            ESP_DBG("--------------------\r\n");
            ESP_DBG("CONNECT AP CB \r\n");
            ESP_DBG("--------------------\r\n");
            user_esp_platform_connect_ap_cb();
        }
    #else
        user_conn.proto.tcp = &user_tcp;
        user_conn.type = ESPCONN_TCP;
        user_conn.state = ESPCONN_NONE;
        device_status = DEVICE_CONNECTING;
        if (reset_flag) {
            device_recon_count = 0;
        }
        #if (PLUG_DEVICE || LIGHT_DEVICE)
        os_timer_disarm(&beacon_timer);
        os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
        #endif
        #ifdef USE_DNS
        user_esp_platform_start_dns(&user_conn);
        #else
        os_memcpy(user_conn.proto.tcp->remote_ip, esp_server_ip, 4);
        user_conn.proto.tcp->local_port = espconn_port();
            #ifdef CLIENT_SSL_ENABLE
            user_conn.proto.tcp->remote_port = 8443;
            #else
            user_conn.proto.tcp->remote_port = ESP_SERVER_PORT;
            #endif
        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        user_esp_platform_connect(&user_conn);
        #endif
    #endif
    } else {
        /* if there are wrong while connecting to some AP, then reset mode */
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) {
            #if ESP_MESH_SUPPORT==0
            user_esp_platform_reset_mode();
            #endif
            os_timer_arm(&client_timer, 100, 0);
        } else {
            os_timer_setfn(&client_timer, (os_timer_func_t *)user_esp_platform_check_ip, NULL);
            os_timer_arm(&client_timer, 100, 0);
        }
    }

}

#if ESP_DEBUG_MODE
/******************************************************************************
 * FunctionName : user_esp_platform_init
 * Description  : device parame init based on espressif platform
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
//set flag for next time chip reboots
void ICACHE_FLASH_ATTR
    user_esp_platform_set_reset_flg(uint32 rst)
{
    esp_param.reset_flg = rst&0xff;
    system_param_save_with_protect(ESP_PARAM_START_SEC,&esp_param,sizeof(esp_param));
}

//reset params, discard station connecting info and server active status
void ICACHE_FLASH_ATTR
    user_esp_platform_reset_default()
{
#if ESP_MESH_SUPPORT
    espconn_mesh_disable(NULL);
#endif
    esp_param.reset_flg=0;
    esp_param.activeflag = 0;
    ESP_DBG("--------------------\r\n");
    ESP_DBG("SYSTEM PARAM RESET !\r\n");
    ESP_DBG("RESET: %d ;  ACTIVE: %d \r\n",esp_param.reset_flg,esp_param.activeflag);
    ESP_DBG("--------------------\r\n\n\n");
    UART_WaitTxFifoEmpty(0,3000);
    os_memset(esp_param.token,0,40);
    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
    system_restore();
    system_restart();
}
#endif

//esp platform init code

void ICACHE_FLASH_ATTR
user_esp_platform_init_peripheral(void)
{
	#if ESP_DEBUG_MODE
	//init flash debug info record function.
    debug_FlashBufInit(&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
	debug_FlashPrintParamInit();
	#endif
	//user sending queue init.
    espSendQueueInit();
    #if ESP_MESH_SUPPORT
	    //mesh dev_mac field init
        g_mac = (char*)mesh_GetMdevMac();
    #endif

	//show iot version
    os_memset(iot_version, 0, sizeof(iot_version));
    os_sprintf(iot_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
    IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
    ESP_DBG("IOT VERSION = %s\n",iot_version);

    //load platform params for esp-server, device key/token/active flag...
    system_param_load(ESP_PARAM_START_SEC, 0, &esp_param, sizeof(esp_param));
    os_memset(g_devkey, 0, sizeof(g_devkey));
    os_memcpy(g_devkey, esp_param.devkey, sizeof(esp_param.devkey));

    ESP_DBG("------------------\r\n");
    ESP_DBG("test flag: %d \r\n");
    ESP_DBG("-----------------\r\n");
    if (esp_param.activeflag != 1) {
    #ifdef SOFTAP_ENCRYPT
        struct softap_config config;
        char password[33];
        char macaddr[6];
        wifi_softap_get_config(&config);
        wifi_get_macaddr(SOFTAP_IF, macaddr);
        os_memset(config.password, 0, sizeof(config.password));
        os_sprintf(password, MACSTR "_%s", MAC2STR(macaddr), PASSWORD);
        os_memcpy(config.password, password, os_strlen(password));
        config.authmode = AUTH_WPA_WPA2_PSK;
        wifi_softap_set_config(&config);
    #endif
        ESP_DBG("user_esp_platform_init_peripheral\r\n");
        wifi_set_opmode(STATIONAP_MODE);
    }

#if PLUG_DEVICE
    user_plug_init();
#elif LIGHT_DEVICE
    //in this mesh demo, we only support light demo
    user_light_init();
#elif SENSOR_DEVICE
    user_sensor_init(esp_param.activeflag);
#endif

    //get chip reset info from RTC register
    struct rst_info *rtc_info = system_get_rst_info();
    ESP_DBG("reset reason: %x\n", rtc_info->reason);
    if (rtc_info->reason == REASON_WDT_RST ||
    	rtc_info->reason == REASON_EXCEPTION_RST ||
    	rtc_info->reason == REASON_SOFT_WDT_RST) {
    	if (rtc_info->reason == REASON_EXCEPTION_RST) {
    		ESP_DBG("Fatal exception (%d):\n", rtc_info->exccause);
    	}
    	#if ESP_DEBUG_MODE
        	if(rtc_info->reason == REASON_WDT_RST) {
				//PURE RED represents a WATCHDOG_RESET
        		light_shadeStart(HINT_PURE_RED,500,4,3,NULL);
        	}else if(rtc_info->reason == REASON_EXCEPTION_RST) {
        	    //PURE GREEN represents a EXCEPTION_RESET
        		light_shadeStart(HINT_PURE_GREEN,500,4,3,NULL);
        	}else if(rtc_info->reason == REASON_SOFT_WDT_RST) {
        	    //PURE BLUE represents a SOFT_WATCHDOG_RESET
        		light_shadeStart(HINT_PURE_BLUE,500,4,3,NULL);
        	}
		//write exception info into flash
    	debug_FlashSvExceptInfo(rtc_info);
    	#endif
    	ESP_DBG("epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\n",
    			rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
    }
}

//init callbacks for espconn
void ICACHE_FLASH_ATTR
	user_esp_platform_mesh_conn_init()
{
    espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
    espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
    espconn_regist_disconcb(&user_conn, user_esp_platform_discon_cb);
    espconn_regist_recvcb(&user_conn, user_esp_platform_recv_cb);
    espconn_regist_sentcb(&user_conn, user_esp_platform_sent_cb);
}


//platform callback after connected to a router.
//1.disable esptouch.
//2.connect ESP-server
void ICACHE_FLASH_ATTR
user_esp_platform_connect_ap_cb()
{
    struct ip_info ipconfig;
    #if ESP_TOUCH_SUPPORT
    esptouch_setAckFlag(true);
    #endif
	#if (LIGHT_DEVICE&ESP_DEBUG_MODE)
	//ESP_DBG("AP CONNECTED CB,SET WHITE\r\n");
    //light_set_color(0,0,0,22222,22222,1000);
    //light_set_aim(0,0,0,22222,22222,1000,0);
	#endif
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
        os_timer_disarm(&client_timer);        
        ESP_DBG("PLATFORM DEBUG: CONNECTED TO AP,RUN PLATFORM CODES.\r\n");
        #if (PLUG_DEVICE || SENSOR_DEVICE)
        #endif
    //***************************
    #if LIGHT_DEVICE
    #if ESP_MDNS_SUPPORT
        #if ESP_MESH_SUPPORT
		//in mesh mode, only root device connects directly with the router
        if(mesh_root_if()){
            user_mdns_conf();
        }else{
            //if it is a sub node, close mdns server
            ESP_DBG("mDNS close...\r\n");
            espconn_mdns_close();
        }
        #else
        user_mdns_conf();
        #endif
    #endif
    
    #endif
    //***************************
    user_conn.proto.tcp = &user_tcp;
    user_conn.type = ESPCONN_TCP;
    user_conn.state = ESPCONN_NONE;
    device_status = DEVICE_CONNECTING;    
    device_recon_count = 0;
    #if (PLUG_DEVICE || LIGHT_DEVICE)
        os_timer_disarm(&beacon_timer);
        os_timer_setfn(&beacon_timer, (os_timer_func_t *)user_esp_platform_sent_beacon, &user_conn);
    #endif

    #ifdef USE_DNS
        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        espconn_regist_disconcb(&user_conn, user_esp_platform_discon_cb);
		espconn_regist_recvcb(&user_conn, user_esp_platform_recv_cb);
		espconn_regist_sentcb(&user_conn, user_esp_platform_sent_cb);
        if(mesh_root_if()){
            user_esp_platform_start_dns(&user_conn);
        }else{
            ip_addr_t ipaddr;
			const char esp_server_ip[4] = {114, 215, 177, 97};
			os_memcpy((uint8*)&(ipaddr.addr),esp_server_ip,4);
			user_esp_platform_dns_found(NULL, &ipaddr,&user_conn);
        }
    #else
        os_memcpy(user_conn.proto.tcp->remote_ip, esp_server_ip, 4);
        user_conn.proto.tcp->local_port = espconn_port();
    #ifdef CLIENT_SSL_ENABLE
        user_conn.proto.tcp->remote_port = 8443;
    #else
        user_conn.proto.tcp->remote_port = ESP_SERVER_PORT;
    #endif
        espconn_regist_connectcb(&user_conn, user_esp_platform_connect_cb);
        espconn_regist_reconcb(&user_conn, user_esp_platform_recon_cb);
        espconn_regist_disconcb(&user_conn, user_esp_platform_discon_cb);
		espconn_regist_recvcb(&user_conn, user_esp_platform_recv_cb);
		espconn_regist_sentcb(&user_conn, user_esp_platform_sent_cb);
        user_esp_platform_connect(&user_conn);
    #endif
    }
}


void ICACHE_FLASH_ATTR _LINE_DESP()
{
	static const char* _line_sep = "===============\r\n";
    ESP_DBG("%s",_line_sep);
}

//return conn param.
void* ICACHE_FLASH_ATTR user_GetUserPConn()
{
    return (&user_conn);
}

#endif
