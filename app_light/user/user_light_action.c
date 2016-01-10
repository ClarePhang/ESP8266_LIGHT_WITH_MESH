#include "os_type.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "osapi.h"
#include "ets_sys.h"
#include "mem.h"
#include "user_light_action.h"
#include "user_light_adj.h"
#include "user_light.h"
#include "user_interface.h"
#include "espnow.h"
#include "user_esp_platform.h"
#include "spi_flash.h"
#include "user_light_hint.h"
#include "user_espnow_action.h"
#if ESP_NOW_SUPPORT
#include "user_simplepair.h"
#if (ESP_MESH_SUPPORT&&ESP_DEBUG_MODE)
#include "mesh.h"
#include "user_debug.h"
#endif
//The espnow packet struct used for light<--->button(switch/controller) intercommunications
typedef struct{
    uint16 battery_voltage_mv;
    uint16 battery_status;
}EspnowBatStatus;

typedef enum{
    COLOR_SET = 0,
    COLOR_CHG ,
    COLOR_TOGGLE,
    COLOR_LEVEL,
    LIGHT_RESET,
    LIGHT_TIMER,
}EspnowCmdCode;


typedef struct {
    uint16 pwm_num;
    uint32 period;
    uint32 duty[8];
    uint32 cmd_code;
    EspnowBatStatus batteryStat;
} EspnowLightCmd;

typedef struct{
    uint8 ip_addr[4];
    uint8 channel;
    uint8 meshStatus;
    uint8 meshLevel;
    uint8 platformStatus;
    uint8 espCloudConnIf;
    uint8 espCloudRegIf;
    uint8 devKey[40];
}EspnowLightRsp;

typedef struct{
    uint8 switchChannel;
    uint8 lightChannel;
    EspnowBatStatus batteryStat;
}EspnowLightSync;

typedef enum{
    ACT_IDLE = 0,
    ACT_ACK = 1,
    ACT_REQ = 2,
    ACT_TIME_OUT =3,
    ACT_RSP =4,
}EspnowMsgStatus;

typedef enum{
    ACT_TYPE_DATA = 0,
    ACT_TYPE_ACK = 1,//NOT USED, REPLACED BY MAC ACK
    ACT_TYPE_SYNC = 2,
    ACT_TYPE_RSP = 3,
    ACT_TYPE_PAIR_REQ = 4,
    ACT_TYPE_PAIR_ACK = 5,
    ACT_TYPE_SERVER = 6,
    ACT_TYPE_SIMPLE_CMD = 7,
}EspnowMsgType;


typedef enum {
    ACT_BAT_NA = 0,
    ACT_BAT_OK = 1,
    ACT_BAT_EMPTY = 2
}ACT_BAT_TYPE;

typedef struct{
    uint8 csum;
    uint8 type;
    uint32 token;//for encryption, add random number
    uint16 cmd_index;
    uint8 wifiChannel;
    uint16 sequence;
    uint16 io_val;
    uint8 rsp_if;
    
    union{
        EspnowLightCmd lightCmd;
        EspnowLightRsp lightRsp;
        EspnowLightSync lightSync;
    };
}EspnowProtoMsg;

typedef struct{
    uint8 csum;//checksum
    uint8 type;//frame type
    uint32 token;//random value
    uint16 cmd_index;//for retry
	uint8 wifiChannel;//self channel
	uint16 sequence;//sequence
	uint16 io_val;//gpio read value
	uint8 rsp_if;//need response
    EspnowBatStatus batteryStat;
}EspnowSimpleCmd;

typedef struct {
    char mac[6];
    int status;
    int battery_mv;
} SwitchBatteryStatus;

//Just save the state of one switch for now.
SwitchBatteryStatus switchBattery;

/*set battery values*/
static void ICACHE_FLASH_ATTR light_EspnowStoreBatteryStatus(char *mac, int status, int voltage_mv) {
    os_memcpy(switchBattery.mac, mac, 6);
    switchBattery.status=status;
    switchBattery.battery_mv=voltage_mv;
}

//ToDo: add support for more switches
int ICACHE_FLASH_ATTR light_EspnowGetBatteryStatus(int idx, char *mac, int *status, int *voltage_mv) {
    if (idx>0) return 0;
    os_memcpy(mac, switchBattery.mac, 6);
    *status=switchBattery.status;
    *voltage_mv=switchBattery.battery_mv;
}


#if LIGHT_DEVICE
LOCAL uint32 last_token = 0;  //to avoid excute the command twice
/* Espnow data receive callback*/ 
void ICACHE_FLASH_ATTR light_EspnowRcvCb(u8 *macaddr, u8 *data, u8 len)
{
    #if ESP_DEBUG_MODE
	os_printf("debug:data: %s \r\n",data);
    if(os_strncmp(data,"debug:",6)==0){
		char response[200];
		uint8 error = 0;
		uint8 level = 0;
		os_memset(response,0,sizeof(response));
		
        os_printf("receive debug command:\r\n");
		char *pstart = (char*)os_strchr(data,':')+1;
		os_printf("cmd: %s \r\n",pstart);
		char *ptmp = (char*)os_strchr(pstart,';');
		char cmd[50];		
		uint16 cmd_len = ptmp - pstart;
		os_memset(cmd,0,sizeof(cmd));		
		os_sprintf(cmd,"NULL");
		
        char param0[50];
		os_memset(param0,0,sizeof(param0));
		os_sprintf(param0,"NULL");
        char param1[50];
		os_memset(param1,0,sizeof(param1));
		os_sprintf(param1,"NULL");
        char param2[50];
		os_memset(param2,0,sizeof(param2));
		os_sprintf(param2,"NULL");



		if(ptmp){
		    os_memcpy(cmd,pstart,cmd_len);
			os_printf("cmd: %s \r\n",cmd);			
			pstart = ptmp+1;
			ptmp = (char*)os_strchr(pstart,';');
		}else{
		    os_sprintf(response,"missing ';'\r\n");
            os_printf("missing ';' ,return...\r\n");
			//return;
			error = 1;
			goto exit;
		}

		if(ptmp){
		    cmd_len = ptmp - pstart;
			os_memcpy(param0,pstart,cmd_len);
			os_printf("param0: %s \r\n",param0);
			level = 1;
			
			pstart = ptmp+1;
			ptmp = (char*)os_strchr(pstart,';');
		}else{
		    os_sprintf(response,"missing ';'\r\n");
            os_printf("missing ';' ,return...\r\n");
			//error =1;
			//return;  
			//goto exit;
		}

		if(ptmp){
		    cmd_len = ptmp - pstart;
			os_memcpy(param1,pstart,cmd_len);
			os_printf("param1: %s \r\n",param1);
			level = 2;
			
			pstart = ptmp+1;
			ptmp = (char*)os_strchr(pstart,';');
		}else{
		    os_sprintf(response,"missing ';'\r\n");
            os_printf("missing ';' ,return...\r\n");
			//error =1;
			//return; 
			//goto exit;
		}
		
		if(ptmp){
		    cmd_len = ptmp - pstart;
			os_memcpy(param2,pstart,cmd_len);
			os_printf("param2: %s \r\n",param2);
			level = 3;
		}else{
		    os_sprintf(response,"missing ';'\r\n");
            os_printf("missing ';' ,return...\r\n");
			//error =1;
			//return;  
			//goto exit;
		}


		os_sprintf(response,"cmd: %s, %s ,%s , %s \r\n",cmd,param0,param1,param2);

		if(error == 0){
			if(os_strcmp(cmd,"read_reg")==0 && level ==1){
                uint32 addr = strtol(param0,NULL,16);
				os_sprintf(response,"read_reg: 0x%08x = 0x%08x \r\n",addr,READ_PERI_REG(addr));
			}
			else if(os_strcmp(cmd,"set_debug_mode")==0 && level==2){
				bool en ;
				if(strtol(param1,NULL,16)>0) en = true;
				else en=false;
				debug_SetEspnowDebugEn(en);

				char ptmp[10];
				int idx;
				char *pmac = (uint8*)debug_GetEspnowTargetMac();
				for(idx=0;idx<6;idx++){
					#if 0
					os_memset(ptmp,0,sizeof(ptmp));
					os_memcpy(ptmp,param0+2*idx,2);
                    pmac[idx] = strtol(ptmp,NULL,16)&0xff;
					os_printf("idx: %d : %02x \r\n",idx,strtol(ptmp,NULL,16)&0xff);
					#else
                    pmac[idx]=macaddr[idx];
					#endif
				}

				os_printf("TARGET MAC: "MACSTR"\r\n",MAC2STR((char*)debug_GetEspnowTargetMac()));
				os_printf("ESPNOW DEBUG EN: %d \r\n",debug_GetEspnowDebugEn());
				os_sprintf(response,"TGT MAC: "MACSTR"\r\nDBG EN: %d\r\n",MAC2STR((char*)debug_GetEspnowTargetMac()),
					                                                      debug_GetEspnowDebugEn());

			}
			else if(os_strcmp(cmd,"get_status")==0 && level==0){

				struct softap_config ap_config;
				struct station_config sta_config;
				wifi_softap_get_config(&ap_config);
				wifi_station_get_config(&sta_config);

				struct ip_info sta_ip,softap_ip;
				wifi_get_ip_info(SOFTAP_IF, &softap_ip);
				wifi_get_ip_info(STATION_IF,&sta_ip);
                os_sprintf(response,"mesh status: %d,heap:%d,channel:%d,sta_ssid:%s,sta_ip:"IPSTR"ap_ssid:%s,ap_ip:"IPSTR"",espconn_mesh_get_status(),
					                                            system_get_free_heap_size(),
					                                            wifi_get_channel(),
					                                            sta_config.ssid,
					                                            IP2STR(&sta_ip.ip.addr),
					                                            ap_config.ssid,
					                                            IP2STR(&softap_ip.ip.addr)
					                                            );
			}
			else if(os_strcmp(cmd,"get_status2")==0 && level==0){

                uint16 cnt_t;
				uint16 cnt;
				sint8 rssi;
				uint8* info_mesh,*info_mesh2;
				uint8 err_parent=0,err_child=0;
				uint16 offset=0;
				int i;
				
				os_printf("MESH GROUP ID:\r\n");
                os_printf(MACSTR"\r\n",MAC2STR(MESH_GROUP_ID));
                if (espconn_mesh_get_node_info(MESH_NODE_PARENT, &info_mesh, &cnt_t)) {
					cnt = cnt_t&0xff;
					int8 rssi = (cnt_t>>8) &0xff;
					os_printf("CNT: %04X \r\n",cnt_t);
                    os_printf("get parent info success\n");
					offset += os_sprintf(response+offset,"prnt_n:%d,",cnt);
					
                    if (cnt == 0) {
                        os_printf("no parent\n");
						
                    } else {
                       for(i=0;i<cnt;i++){
                           os_printf("MAC[%d]:"MACSTR"\r\n",i,MAC2STR(info_mesh+i*6));
						   os_printf("rssi: %d \r\n",rssi);
						   offset += os_sprintf(response+offset,"prnt[%d]:"MACSTR",rssi:%d,",i,MAC2STR(info_mesh+i*6),rssi);
                       }
                    }
                } else {
                    os_printf("get parent info fail\n");
					offset += os_sprintf(response+offset,"get_prnt_err,");
                } 

				

                if (espconn_mesh_get_node_info(MESH_NODE_CHILD, &info_mesh2, &cnt)) {
                    os_printf("get child info success\n");
					offset += os_sprintf(response+offset,"cnt:%d,",cnt);
                    if (cnt == 0) {
                        os_printf("no child\n");
                    } else {
                       struct mesh_sub_node_info *p_node_info;
					   //struct mesh_sub_node_info node_info;
                       for(i=0;i<cnt;i++){
						   p_node_info = (struct mesh_sub_node_info*)(info_mesh+i*sizeof(struct mesh_sub_node_info));
						   os_printf("p info_mesh : %p\r\n",info_mesh);
						   os_printf("p_node_info:  %p \r\n",p_node_info);
                           os_printf("MAC[%d]:"MACSTR" ; SUB NODE NUM: %d\r\n",i,MAC2STR(p_node_info->mac),p_node_info->sub_count);
						   offset += os_sprintf(response+offset,"child[%d]:"MACSTR",sub_num[%d]:%d;",i,MAC2STR(p_node_info->mac),i,p_node_info->sub_count);
                       }
                    }
                } else {
                    os_printf("get child info fail\n");
					offset += os_sprintf(response+offset,"get_child_err,");
                }
				os_printf("RESP: %s \r\n",response);
			}
			else if(os_strcmp(cmd,"get_status3")==0 && level==0){
                uint8 val_u8 = 0;
				uint16 val_u16=0;
				char str_tmp[100];
				sint8 val_s8=0;
				uint16 offset = 0;
				
				
				espnow_get_debug_data(M_FREQ_CAL,(uint8*)&val_s8);
				offset+= os_sprintf(response+offset,"f_err:%d,",val_s8);
			
				
				espnow_get_debug_data(WIFI_STATUS,(uint8*)&val_u8);
				offset+= os_sprintf(response+offset,"WiFi_s:%d,",val_u8);
				
				espnow_get_debug_data(FREE_HEAP_SIZE,(uint8*)&val_u16);
				offset+= os_sprintf(response+offset,"heap:%d,",val_u16);
				
				espnow_get_debug_data(CHILD_NUM,(uint8*)&val_u8);
				offset+= os_sprintf(response+offset,"child:%d,",val_u8);
				
				espnow_get_debug_data(SUB_DEV_NUM,(uint8*)&val_u16);
				offset+= os_sprintf(response+offset,"sub_dev:%d,",val_u16);
				
				espnow_get_debug_data(MESH_STATUS,(uint8*)&val_s8);
				offset+= os_sprintf(response+offset,"M_STATUS:%d,",val_s8);
				
				espnow_get_debug_data(MESH_VERSION,(uint8*)str_tmp);
				offset+= os_sprintf(response+offset,"VER:%s,",str_tmp);
				
				//espnow_get_debug_data(MESH_ROUTER,(uint8*)val_u8);
				//offset+= os_sprintf(response,"f_err:%d,",val_u8);
				
				espnow_get_debug_data(MESH_LAYER,(uint8*)&val_u8);
				offset+= os_sprintf(response+offset,"layer:%d,",val_u8);
				
				espnow_get_debug_data(MESH_ASSOC,(uint8*)&val_u8);
				offset+= os_sprintf(response+offset,"ASSOC:%d,",val_u8);
				
				espnow_get_debug_data(MESH_CHANNEL,(uint8*)&val_u8);
				offset+= os_sprintf(response+offset,"CHANNEL:%d,",val_u8);

				os_printf("RESP: %s \r\n",response);

			}
		}
     exit:
	 	os_printf("send: %s \r\n",response);
		os_printf("SEND TO MAC: "MACSTR"\r\n",MAC2STR(macaddr));
		esp_now_send(macaddr, (uint8*)response, os_strlen(response)); 
        

		return;
		
    }
	#endif
	
    int i;
    uint32 duty_set[PWM_CHANNEL] = {0};
    uint32 period=1000;
    LOCAL uint8 color_bit=1;
    LOCAL uint8 toggle_flg = 0;

	#if ESPNOW_SIMPLE_CMD_MODE
    EspnowSimpleCmd EspnowMsg;
	int size = sizeof(EspnowSimpleCmd);
	#else
    EspnowProtoMsg EspnowMsg;
	int size = sizeof(EspnowProtoMsg);
	#endif

    os_memcpy((uint8*)(&EspnowMsg), data,size);
    ESPNOW_DBG("RCV DATA: \r\n");
    for(i=0;i<len;i++) ESPNOW_DBG("%02x ",data[i]);
    ESPNOW_DBG("\r\n-----------------------\r\n");
        
    if(config_ParamCsumCheck(&EspnowMsg, size) ){
        ESPNOW_DBG("cmd check sum ok\r\n");
        //ACT_TYPE_DATA: this is a set of data or command.
    #if ESPNOW_SIMPLE_CMD_MODE
	    if(EspnowMsg.type == ACT_TYPE_SIMPLE_CMD){
            if(EspnowMsg.token == last_token){
                ESPNOW_DBG("Duplicated cmd ... return...\r\n");
                ESPNOW_DBG("last token: %d ; cur token: %d ;\r\n",last_token,EspnowMsg.token);
                return;
            }
            light_EspnowStoreBatteryStatus(macaddr, EspnowMsg.batteryStat.battery_status, EspnowMsg.batteryStat.battery_voltage_mv);

			//use registered function to react to the command code.
	        //UserExecuteEspnowCmd(EspnowMsg.lightCmd.cmd_code);
			if( UserExecuteEspnowCmd(EspnowMsg.io_val)){
				ESPNOW_DBG("find customized definition...\r\n");
			}else{
                ESPNOW_DBG("find no proper function to the key code...\r\n");
			}
            last_token = EspnowMsg.token;
            if(EspnowMsg.rsp_if ==1){
                EspnowMsg.type=ACT_TYPE_RSP;
            }else{
                ESPNOW_DBG("do not send response\r\n");
                return;
            }
	    }
	#else
		if(EspnowMsg.type == ACT_TYPE_DATA){
            
            if(EspnowMsg.token == last_token){
                ESPNOW_DBG("duplicated cmd ... return...\r\n");
                ESPNOW_DBG("last token: %d ; cur token: %d ;\r\n",last_token,EspnowMsg.token);
                return;
            }
            
            ESPNOW_DBG("period: %d ; channel : %d \r\n",EspnowMsg.lightCmd.period,EspnowMsg.lightCmd.pwm_num);
            for(i=0;i<EspnowMsg.lightCmd.pwm_num;i++){
                ESPNOW_DBG(" duty[%d] : %d \r\n",i,EspnowMsg.lightCmd.duty[i]);
                duty_set[i] = EspnowMsg.lightCmd.duty[i];
            }
            
            ESPNOW_DBG("SOURCE MAC CHEK OK \r\n");
            ESPNOW_DBG("cmd channel : %d \r\n",EspnowMsg.wifiChannel);
            ESPNOW_DBG("SELF CHANNEL: %d \r\n",wifi_get_channel());
            ESPNOW_DBG("Battery status %d voltage %dmV\r\n", EspnowMsg.lightCmd.batteryStat.battery_status, EspnowMsg.lightCmd.batteryStat.battery_voltage_mv);
            light_EspnowStoreBatteryStatus(macaddr, EspnowMsg.lightCmd.batteryStat.battery_status, EspnowMsg.lightCmd.batteryStat.battery_voltage_mv);
            
            ESPNOW_DBG("***************\r\n");
            ESPNOW_DBG("EspnowMsg.lightCmd.colorChg: %d \r\n",EspnowMsg.lightCmd.cmd_code);

			//use registered function to react to the command code.
			
	        //UserExecuteEspnowCmd(EspnowMsg.lightCmd.cmd_code);
			if( UserExecuteEspnowCmd(EspnowMsg.io_val)){
				ESPNOW_DBG("find customized definition...\r\n");
			}

			else{
			    if(EspnowMsg.lightCmd.cmd_code==COLOR_SET){
                    light_hint_abort();
                    light_TimerAdd(NULL,20000,true,false);
                    ESPNOW_DBG("SHUT DOWN ADD 20 SECONDS...\r\n");
                }else if(EspnowMsg.lightCmd.cmd_code==COLOR_CHG){
                    light_hint_abort();
                    light_TimerStop();
                    toggle_flg= 1;
                    ESPNOW_DBG("light change color cmd \r\n");
                    for(i=0;i<PWM_CHANNEL;i++){
                        if(i<3)
                            duty_set[i] = ((color_bit>>i)&0x1)*22222;
                        else
                            duty_set[i] = ((color_bit>>i)&0x1)*8000;
                    }
                    ESPNOW_DBG("set color : %d %d %d %d %d %d\r\n",duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],EspnowMsg.lightCmd.period);
                    //light_set_aim( duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],1000,0); 
					light_set_color( duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],1000,0); 
    				//===========SEND BROAD CAST IN MESH================
    				//light_SendMeshBroadcastCmd(duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],1000);
    				//==================================================
                    color_bit++;
                    if(color_bit>=32) color_bit=1;
                }else if(EspnowMsg.lightCmd.cmd_code==COLOR_TOGGLE){
                    light_hint_abort();
                    light_TimerStop();
                    color_bit = 1;
                    //toggle_flg= 1;
                    if(toggle_flg == 0){
                        light_set_color( 0,0,0,0,0,1000); 
    					//===========SEND BROAD CAST IN MESH================
    					//light_SendMeshBroadcastCmd(0,0,0,0,0,1000);
    					//==================================================
                        toggle_flg = 1;
                    }else{
                        light_set_color( 0,0,0,22222,22222,1000); 
    					//===========SEND BROAD CAST IN MESH================
    					//light_SendMeshBroadcastCmd(0,0,0,22222,22222,1000);
    					//==================================================
                        toggle_flg = 0;
                    }
                }else if(EspnowMsg.lightCmd.cmd_code==COLOR_LEVEL){
                    light_hint_abort();
                    light_TimerStop();
                    
                    struct ip_info sta_ip;
                    wifi_get_ip_info(STATION_IF,&sta_ip);
                    #if ESP_MESH_SUPPORT                        
                        if( espconn_mesh_local_addr(&sta_ip.ip)){
                            ESPNOW_DBG("THIS IS A MESH SUB NODE..\r\n");
                            uint32 mlevel = sta_ip.ip.addr&0xff;
                            light_ShowDevLevel(mlevel,500);
                        }else if(sta_ip.ip.addr!= 0){
                            ESPNOW_DBG("THIS IS A MESH ROOT..\r\n");
                            light_ShowDevLevel(1,500);
                        }else{
                            ESPNOW_DBG("THIS IS A MESH ROOT..\r\n");
                            light_ShowDevLevel(0,500);
                        }
                    #else
                        if(sta_ip.ip.addr!= 0){
                            ESPNOW_DBG("wifi connected..\r\n");
                            light_ShowDevLevel(1,500);
                        }else{
                            ESPNOW_DBG("wifi not connected..\r\n");
                            light_ShowDevLevel(0,500);
                        }
                    #endif
                    
                
                }else if(EspnowMsg.lightCmd.cmd_code==LIGHT_RESET){
                    system_restore();
                    extern struct esp_platform_saved_param esp_param;
                    esp_param.reset_flg=0;
                    esp_param.activeflag = 0;
                    os_memset(esp_param.token,0,40);
                    system_param_save_with_protect(ESP_PARAM_START_SEC, &esp_param, sizeof(esp_param));
                    ESPNOW_DBG("--------------------\r\n");
                    ESPNOW_DBG("SYSTEM PARAM RESET !\r\n");
                    ESPNOW_DBG("RESET: %d ;  ACTIVE: %d \r\n",esp_param.reset_flg,esp_param.activeflag);
                    ESPNOW_DBG("--------------------\r\n\n\n");
                    UART_WaitTxFifoEmpty(0,3000);
                    system_restart();
                }else if(EspnowMsg.lightCmd.cmd_code==LIGHT_TIMER){
                    light_TimerAdd(EspnowMsg.lightCmd.duty,20000,true,false);
                    ESPNOW_DBG("SHUT DOWN ADD 20 SECONDS...\r\n");
                }
			}

			
            last_token = EspnowMsg.token;
            
            if(EspnowMsg.rsp_if ==1){
                EspnowMsg.type=ACT_TYPE_RSP;
            }else{
                ESPNOW_DBG("do not send response\r\n");
                return;
            }
        }
        //ACT_TYPE_SYNC: AFTER LIGHT CONFIGURED TO ROUTER, ITS CHANNEL CHANGES
        //               NEED TO SET THE CHANNEL FOR EACH LIGHT.
        else if(EspnowMsg.type == ACT_TYPE_SYNC && (EspnowMsg.lightSync.switchChannel == wifi_get_channel())){
            EspnowMsg.type = ACT_TYPE_SYNC;
            ESPNOW_DBG("cmd rcv channel : %d \r\n",EspnowMsg.wifiChannel);
            EspnowMsg.wifiChannel = wifi_get_channel();
            EspnowMsg.lightSync.lightChannel = wifi_get_channel();
            ESPNOW_DBG("send sync, self channel : %d  \r\n",wifi_get_channel());
            light_Espnow_ShowSyncSuc();
        }else if(EspnowMsg.type == ACT_TYPE_SERVER){
            light_shadeStart(HINT_WHITE,500,3,1,NULL);
            //user_platform_send_sms();
        }else{
            ESPNOW_DBG(" data type %d \r\n",EspnowMsg.type);
            ESPNOW_DBG("data channel :%d \r\n",EspnowMsg.wifiChannel);
            ESPNOW_DBG("data self channel: %d \r\n",wifi_get_channel());
            ESPNOW_DBG("data type or channel error\r\n");
            return;
        }
	#endif
			
        
        EspnowMsg.rsp_if=0;
        EspnowMsg.token=os_random();
        //light_EspnowSetCsum(&EspnowMsg);
		config_ParamCsumSet(&EspnowMsg, (uint8*)&(EspnowMsg.csum) ,size);
        
        #if ESPNOW_DEBUG
            int j;
            for(j=0;j<sizeof(EspnowLightCmd);j++) ESPNOW_DBG("%02x ",*((uint8*)(&EspnowMsg)+j));
            ESPNOW_DBG("\r\n");
        #endif
        ESPNOW_DBG("SEND TO MAC: "MACSTR"\r\n",MAC2STR(macaddr));
        esp_now_send(macaddr, (uint8*)(&EspnowMsg), sizeof(EspnowProtoMsg)); //send ack
        //}
    }else{
        ESPNOW_DBG("cmd check sum error\r\n");
    }
}


/* Espnow data sent callback func*/
LOCAL void ICACHE_FLASH_ATTR espnow_send_cb(u8 *mac_addr, u8 status)
{
    ESPNOW_DBG("send_cb: mac[%02x:%02x:%02x:%02x:%02x:%02x], status: [%d]\n", MAC2STR(mac_addr),status); 
}

/* Module initialization for Espnow*/
/*ESPNOW INIT,if encryption is enabled, the slave device number is limited,typically 10 at most*/
void ICACHE_FLASH_ATTR light_EspnowInit()
{
    #if ESP_DEBUG_MODE
    ESPNOW_DBG("ESPNOW DEBUG MODE...\r\n");
	#endif
    ESPNOW_DBG("CHANNEL : %d \r\n",wifi_get_channel());
	//Init Action Function Map
    user_EspnowActionParamInit();
	ESPNOW_DBG("SIZEOF userActionParam: %d \r\n",sizeof(userActionParam));

	
    if (esp_now_init()==0) {
        ESPNOW_DBG("direct link  init ok\n");
        esp_now_register_recv_cb(light_EspnowRcvCb);
        esp_now_register_send_cb(espnow_send_cb);    
    } else {
        ESPNOW_DBG("esp-now already init\n");
    }
    //It is designed to send data from station interface of switch/controller to softap interface of light
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);  //role 1: switch   ;  role 2 : light;

    PairedButtonParam* sp_dev = (PairedButtonParam*)sp_GetPairedParam();
    
    
    int j,res;
    for(j=0;j<sp_dev->PairedNum;j++){
        #if ESPNOW_ENCRYPT
		    //do not enable KEY HASH if using simplepair
            #if (ESPNOW_KEY_HASH &&(ESP_SIMPLE_PAIR_SUPPORT==0))
            esp_now_set_kok((uint8*)(sp_dev->PairedList[j].key_t), ESPNOW_KEY_LEN);
            #endif
            res = esp_now_add_peer((uint8*)(sp_dev->PairedList[j].mac_t), (uint8)ESP_NOW_ROLE_CONTROLLER,(uint8)WIFI_DEFAULT_CHANNEL, (uint8*)(sp_dev->PairedList[j].key_t), (uint8)ESPNOW_KEY_LEN);
        #else
            //res = esp_now_add_peer((uint8*)(sp_dev->PairedList[j].mac_t), (uint8)ESP_NOW_ROLE_CONTROLLER,(uint8)WIFI_DEFAULT_CHANNEL, NULL, (uint8)ESPNOW_KEY_LEN);
        #endif
		
        if(res == 0){
            ESPNOW_DBG("INIT OK: MAC[%d]:"MACSTR"\r\n",j,MAC2STR(((uint8*)(sp_dev->PairedList[j].mac_t))));
        }else{
            ESPNOW_DBG("INIT OK: MAC[%d]:"MACSTR"\r\n",j,MAC2STR(((uint8*)(sp_dev->PairedList[j].mac_t))));
        }
    }
}

/*Espnow De-init*/
void ICACHE_FLASH_ATTR light_EspnowDeinit()
{
    ESPNOW_DBG("%s\r\n",__func__);
    esp_now_unregister_recv_cb(); 
    esp_now_deinit();
}






#endif

#endif




