
#include"user_espnow_action.h"
#include "osapi.h"
#if ESP_NOW_SUPPORT

#include "ets_sys.h"
#include "mem.h"
#include "user_light_action.h"
#include "user_light_adj.h"
#include "user_light.h"
#include "user_interface.h"
#include "espnow.h"
#include "user_esp_platform.h"
#include "spi_flash.h"

void ICACHE_FLASH_ATTR UserPushFunction1(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction2(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction3(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction4(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction5(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction6(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction7(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction8(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction9(void*arg);
void ICACHE_FLASH_ATTR UserPushFunction10(void*arg);


//Static Table 
FunctionTag fun_tag[MAX_COMMAND_COUNT]={
    {"func1",UserPushFunction1},
    {"func2",UserPushFunction2},
    {"func3",UserPushFunction3},
    {"func4",UserPushFunction4},
    {"func5",UserPushFunction5},
    {"func6",UserPushFunction6},
    {"func7",UserPushFunction7},
    {"func8",UserPushFunction8},
    //{"func9",UserPushFunction9},
    //{"func10",UserPushFunction10},
 };

UserActionParam userActionParam;

#define FUNC_PARAM_FORMAT0 "\"%d,%d,%d,%d\""
#define FUNC_PARAM_FORMAT1 "\"%d,%d\""
#define FUNC_PARAM_FORMAT2 "\"%d,%d\""
#define FUNC_PARAM_FORMAT3 "\"%d,%d\""
//#define FORMAT_SEL(X)  FUNC_PARAM_FORMAT##X 
//#define FORMAT_SEL_FUNC(x) (x==0)? FUNC_PARAM_FORMAT0:\
//	                         (x==1)?FUNC_PARAM_FORMAT1:\
//	                         (x==2)?FUNC_PARAM_FORMAT2:\
//	                         (x==3)?FUNC_PARAM_FORMAT3:NULL
	                                 
#define FUNC_PARAM0(j,k) userActionParam.userAction[j].press_arg[k].broadcast_if,userActionParam.userAction[j].press_arg[k].R, userActionParam.userAction[j].press_arg[k].G, userActionParam.userAction[j].press_arg[k].B
#define FUNC_PARAM1(j,k) userActionParam.userAction[j].press_arg[k].broadcast_if,userActionParam.userAction[j].press_arg[k].on_off
#define FUNC_PARAM2(j,k) userActionParam.userAction[j].press_arg[k].broadcast_if,userActionParam.userAction[j].press_arg[k].timer_ms
#define FUNC_PARAM3(j,k) userActionParam.userAction[j].press_arg[k].broadcast_if,userActionParam.userAction[j].press_arg[k].brightness

//#define PARAM_SEL(X,j,k) FUNC_PARAM_FORMAT##X(j,k) 
//PwmParam Pwm_Param={0,0,0,0,0,0};


#if STRING_SV_IN_FLASH
const static char PRESS_FUNC_STR_0[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT0;
const static char PRESS_FUNC_STR_1[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT1;
const static char PRESS_FUNC_STR_2[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT2;
const static char PRESS_FUNC_STR_3[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT3;

const static char TAP_FUNC_STR_0[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT0;
const static char TAP_FUNC_STR_1[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT1;
const static char TAP_FUNC_STR_2[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT2;
const static char TAP_FUNC_STR_3[] ICACHE_RODATA_ATTR STORE_ATTR = ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT3;

//#define PRESS_FUNC_SEL(X) PRESS_FUNC_STR_##X
//#define TAP_FUNC_SEL(X)   TAP_FUNC_STR_##X
#else
#define PRESS_FUNC_STR_0 ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT0
#define PRESS_FUNC_STR_1 ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT1
#define PRESS_FUNC_STR_2 ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT2
#define PRESS_FUNC_STR_3 ",\"press%c\":\"%s\",\"arg%cpress\":"FUNC_PARAM_FORMAT3

#define TAP_FUNC_STR_0 ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT0
#define TAP_FUNC_STR_1 ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT1
#define TAP_FUNC_STR_2 ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT2
#define TAP_FUNC_STR_3 ",\"tap%c\":\"%s\",\"arg%ctap\":"FUNC_PARAM_FORMAT3


#endif

void ICACHE_FLASH_ATTR
user_EspnowActionParamLoad(UserActionParam* pParam)
{
	config_ParamLoadWithProtect(LIGHT_ESPNOW_ACTION_MAP_PARAM_ADDR,0,(uint8*)pParam,sizeof(UserActionParam));
}

void ICACHE_FLASH_ATTR
user_EspnowActionParamSave(UserActionParam* pParam)
{
    pParam->magic = ACTION_PARAM_MAGIC;
	config_ParamCsumSet(pParam,&(pParam->csum),sizeof(UserActionParam));
	config_ParamSaveWithProtect(LIGHT_ESPNOW_ACTION_MAP_PARAM_ADDR,(uint8*)pParam,sizeof(UserActionParam));
}

void ICACHE_FLASH_ATTR
user_EespnowActionParamReset(UserActionParam* pParam)
{
    uint16 i;
	for(i=0;i<MAX_COMMAND_COUNT;i++){
		pParam->userAction[i].key_value = key_tag[i];
		pParam->userAction[i].user_long_press_func_pos = -1;
		pParam->userAction[i].user_short_press_func_pos = -1;
	}
	user_EspnowActionParamSave(pParam);
}

bool ICACHE_FLASH_ATTR
	user_EspnowActionParamCheckKeyVal(UserActionParam* pParam)
{
    int i;
	for(i=0;i<MAX_COMMAND_COUNT;i++){
        if(pParam->userAction[i].key_value != (uint16)key_tag[i]) {
            ACTION_DBG("ESPNOW ACT PARAM ERROR\r\n");
			return false;
        }
	}
    return true;
}

void ICACHE_FLASH_ATTR
user_EspnowActionParamInit()
{
	user_EspnowActionParamLoad(&userActionParam);
	if(userActionParam.magic == ACTION_PARAM_MAGIC  
		&& config_ParamCsumCheck(&userActionParam, sizeof(UserActionParam))
		&& user_EspnowActionParamCheckKeyVal(&userActionParam)){
        ACTION_DBG("ESPNOW ACTION MAP LOAD OK\r\n");
	}else{
		user_EespnowActionParamReset(&userActionParam);
		ACTION_DBG("ESPNOW ACTION MAP RESET OK\r\n");
	}
}

/*----------------------------------------------------
                  short press register function
----------------------------------------------------*/
LOCAL uint8 color_bit=1;
LOCAL uint8 toggle_flg = 0;
void ICACHE_FLASH_ATTR 
UserPushFunction1(void*arg)/*  set color*/
{
    EspNowCmdParam* param=(EspNowCmdParam*)arg; 
    ACTION_DBG("UserPushFunction1\n");
    ACTION_DBG("R:%d G:%d B :%d, BCST: %d\n",param->R,param->G,param->B,param->broadcast_if);	
	
	uint32 duty_limit = DUTY_MAX_LIMIT(1000); 
	#if ESP_MESH_SUPPORT
	if(param->broadcast_if){
		#if ESP_DEBUG_MODE
	    light_SendMeshBroadcastCmd(duty_limit*(param->R)/255,
			                       duty_limit*(param->G)/255,
			                       duty_limit*(param->B)/255,0,0,1000);
		#else
	    light_SendMeshBroadcastCmd(duty_limit*(param->R)/255,
			                       duty_limit*(param->G)/255,
			                       duty_limit*(param->B)/255,0,0,1000);
		#endif
	}
	#endif
	//light_set_aim(duty_limit*(param->R)/255,duty_limit*(param->G)/255,duty_limit*(param->B)/255,0,0,1000,0);
	light_set_color(duty_limit*(param->R)/255,duty_limit*(param->G)/255,duty_limit*(param->B)/255,0,0,1000);
#if 0
    ACTION_DBG("UserPushFunction1\n");
    light_hint_abort();
    light_TimerAdd(NULL);
    ACTION_DBG("SHUT DOWN ADD 20 SECONDS...\r\n");
#endif

}
void ICACHE_FLASH_ATTR 
UserPushFunction2(void*arg)/*  set on off*/
{
    EspNowCmdParam* param=(EspNowCmdParam*)arg; 
	static uint8 toggle_flg = 0;
    ACTION_DBG("UserPushFunction2\n");
    ACTION_DBG("On_OFF %d\n",param->on_off);	
	ACTION_DBG("BCAST: %d\r\n",param->broadcast_if);
	if(param->on_off == 1){
        ACTION_DBG("turn on the light:\r\n");
		
        #if ESP_MESH_SUPPORT
		if(param->broadcast_if){
			light_SendMeshBroadcastCmd(0,0,0,22222,22222,1000);
		}
		#endif
		//light_set_aim(0,0,0,22222,22222,1000,0);
		light_set_color(0,0,0,22222,22222,1000);
	}else if(param->on_off == 0){
		ACTION_DBG("turn on the light:\r\n");
		
        #if ESP_MESH_SUPPORT
		if(param->broadcast_if){
			light_SendMeshBroadcastCmd(0,0,0,0,0,1000);
		}
		#endif
		//light_set_aim(0,0,0,0,0,1000,0);
		light_set_color(0,0,0,0,0,1000);
	}else if(param->on_off == -1){
		ACTION_DBG("toggle the light:\r\n");
		if(toggle_flg == 0){
		    //light_set_aim(0,0,0,22222,22222,1000,0);
			light_set_color(0,0,0,22222,22222,1000);
            #if ESP_MESH_SUPPORT
			if(param->broadcast_if){
				light_SendMeshBroadcastCmd(0,0,0,22222,22222,1000);
			}
			#endif
			toggle_flg = 1;
		}else{
		
            #if ESP_MESH_SUPPORT
			if(param->broadcast_if){
				light_SendMeshBroadcastCmd(0,0,0,0,0,1000);
			}
			#endif
		    //light_set_aim(0,0,0,0,0,1000,0);
			light_set_color(0,0,0,0,0,1000);
			toggle_flg = 0;
		}
	}
#if 0
    int i;
    uint32 duty_set[PWM_CHANNEL] = {0};
    uint32 period=1000;

    ACTION_DBG("UserPushFunction2\n");
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
   //ESPNOW_DBG("set color : %d %d %d %d %d %d\r\n",duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],EspnowMsg.lightCmd.period);
    light_set_aim( duty_set[0],duty_set[1],duty_set[2],duty_set[3],duty_set[4],1000,0);	
    color_bit++;
    if(color_bit>=32) color_bit=1;
#endif
}
void ICACHE_FLASH_ATTR 
UserPushFunction3(void*arg)/*  set timer*/
{
    EspNowCmdParam* param=(EspNowCmdParam*)arg; 
    ACTION_DBG("UserPushFunction3\n");
	ACTION_DBG("Broadcast flg: %d \r\n",param->broadcast_if);
    ACTION_DBG("Time_Ms %d\n",param->timer_ms);	
	light_TimerAdd(NULL,(param->timer_ms)*1000,false,true);
#if 0
    ACTION_DBG("UserPushFunction3\n");
    light_hint_abort();
    light_TimerStop();
    color_bit = 1;
    //toggle_flg= 1;
    if(toggle_flg == 0){
        light_set_aim( 0,0,0,0,0,1000,0); 
        toggle_flg = 1;
}else{
        light_set_aim( 0,0,0,22222,22222,1000,0); 
        toggle_flg = 0;
}
#endif
}
void ICACHE_FLASH_ATTR 
UserPushFunction4(void*arg)
{
    EspNowCmdParam* param=(EspNowCmdParam*)arg; 
    ACTION_DBG("UserPushFunction4\n");
    uint32 r = user_light_get_duty(LIGHT_RED);
    uint32 g = user_light_get_duty(LIGHT_GREEN);
    uint32 b = user_light_get_duty(LIGHT_BLUE);
    uint32 cw = user_light_get_duty(LIGHT_WARM_WHITE);
    uint32 ww = user_light_get_duty(LIGHT_COLD_WHITE);
	int k = 100;
	uint32 duty_target;
	ACTION_DBG("brightness : %d \r\n",param->brightness);
	ACTION_DBG("broadcast : %d \r\n",param->broadcast_if);
	if(param->brightness){
		duty_target = DUTY_MAX_LIMIT(1000) / 20; 
		
		if((r<duty_target) && (g<duty_target) && (b<duty_target) && (cw<duty_target) && (ww<duty_target)){
            k=-1;
		}else{
            k = 200;
		}
	}else{
        k = 50;
	}

    if(k==-1){
        duty_target = DUTY_MAX_LIMIT(1000) / 10;
		r= 0;
		g= 0;
		b= 0;
		cw= duty_target;
		ww= duty_target;
    }else{
		r = r*k/100;
        g = g*k/100;
    	b = b*k/100;
    	cw = cw*k/100;
    	ww = ww*k/100;
    }
	//light_set_aim(r,g,b,cw,ww,1000,0);
	light_set_color(r,g,b,cw,ww,1000);
    #if ESP_MESH_SUPPORT
	if(param->broadcast_if){
		light_SendMeshBroadcastCmd(r,g,b,cw,ww,1000);
	}
    #endif
	
#if 0
    light_hint_abort();
    light_TimerStop();
	
    struct ip_info sta_ip;
    wifi_get_ip_info(STATION_IF,&sta_ip);
#if ESP_MESH_SUPPORT
    if( espconn_mesh_local_addr(&sta_ip.ip)){
        ESPNOW_DBG("THIS IS A MESH SUB NODE..\r\n");
        uint32 mlevel = sta_ip.ip.addr&0xff;
        light_ShowDevLevel(mlevel);
    }else if(sta_ip.ip.addr!= 0){
        ESPNOW_DBG("THIS IS A MESH ROOT..\r\n");
        light_ShowDevLevel(1);
    }else{
        ESPNOW_DBG("THIS IS A MESH ROOT..\r\n");
        light_ShowDevLevel(0);
    }
#else
    if(sta_ip.ip.addr!= 0){
        ESPNOW_DBG("wifi connected..\r\n");
        light_ShowDevLevel(1);
    }  else{
        ESPNOW_DBG("wifi not connected..\r\n");
        light_ShowDevLevel(0);
    }
#endif

#endif
}
void ICACHE_FLASH_ATTR 
UserPushFunction5(void*arg)
{
    ACTION_DBG("UserPushFunction5\n");

}
void ICACHE_FLASH_ATTR 
UserPushFunction6(void*arg)
{
    ACTION_DBG("UserPushFunction6\n");
}
void ICACHE_FLASH_ATTR 
UserPushFunction7(void*arg)
{
    ACTION_DBG("UserPushFunction7\n");
}
void ICACHE_FLASH_ATTR 
UserPushFunction8(void*arg)
{
    ACTION_DBG("UserPushFunction8\n");
}
void ICACHE_FLASH_ATTR 
UserPushFunction9(void*arg)
{
    ACTION_DBG("UserPushFunction9\n");
}
void ICACHE_FLASH_ATTR 
UserPushFunction10(void*arg)
{
    ACTION_DBG("UserPushFunction10\n");
}


/*
find the key of short press coresspond function
*/
int8 ICACHE_FLASH_ATTR
UserGetShortPressFuncByKey(uint8 idx)
{
    //find  shortr press fixed function
    if(-1==userActionParam.userAction[idx].user_short_press_func_pos){
        return -1;
    }
    else{
        return (userActionParam.userAction[idx].user_short_press_func_pos);
    }
}

int8 ICACHE_FLASH_ATTR
UserGetLongPressFuncByKey(uint8 idx)
{
    if(-1==userActionParam.userAction[idx].user_long_press_func_pos){
        return -1;
    }
    else {
        return userActionParam.userAction[idx].user_long_press_func_pos;
    }
}

bool ICACHE_FLASH_ATTR
UserGetShortPressFuncString(uint8 idx,char* str)
{
    int8 pos=0;
	char  pbuf_tmp[100];
    if((pos=UserGetShortPressFuncByKey(idx))>-1){/*can find coreespond fun,next format string by the fun param list*/
		switch(pos){
            case 0:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)TAP_FUNC_STR_0);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM0(idx,SHORT_PRESS));				
				#else
                os_sprintf(str,TAP_FUNC_STR_0,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM0(idx,SHORT_PRESS));
                #endif
				return TRUE;
                break;
            }
            case 1:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)TAP_FUNC_STR_1);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM1(idx,SHORT_PRESS));				
				#else
                os_sprintf(str,TAP_FUNC_STR_1,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM1(idx,SHORT_PRESS));
                #endif
				return TRUE;
                break;
            }
            case 2:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)TAP_FUNC_STR_2);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM2(idx,SHORT_PRESS));				
				#else
                os_sprintf(str,TAP_FUNC_STR_2,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM2(idx,SHORT_PRESS));
                #endif
				return TRUE;
                break;
            }
			case 3:
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)TAP_FUNC_STR_3);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM3(idx,SHORT_PRESS));				
				#else
                os_sprintf(str,TAP_FUNC_STR_3,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM3(idx,SHORT_PRESS));
                #endif
				return TRUE;
				break;
            default:
                ACTION_DBG("the key short press not add key=%c\n",key_tag[idx]);
                return FALSE;
                //break;
        }
    }
    return FALSE;
    
}
bool ICACHE_FLASH_ATTR
UserGetLongPressFuncString(uint8 idx,char* str)
{
    int8 pos=0;
	char pbuf_tmp[100];
    if((pos=UserGetLongPressFuncByKey(idx))>-1){/*can find coreespond fun,next format string by the fun param list*/
        switch(pos){
            case 0:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)PRESS_FUNC_STR_0);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM0(idx,LONG_PRESS));				
				#else
                os_sprintf(str,PRESS_FUNC_STR_0,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM0(idx,LONG_PRESS));
                #endif
				return TRUE;
                break;
            }
            case 1:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)PRESS_FUNC_STR_1);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM1(idx,LONG_PRESS));				
				#else
                os_sprintf(str,PRESS_FUNC_STR_1,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM1(idx,LONG_PRESS));
                #endif
				return TRUE;
                break;
            }
            case 2:{
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)PRESS_FUNC_STR_2);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM2(idx,LONG_PRESS));				
				#else
                os_sprintf(str,PRESS_FUNC_STR_2,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM2(idx,LONG_PRESS));
                #endif
				return TRUE;
                break;
            }
            case 3:
				#if STRING_SV_IN_FLASH
                load_string_from_flash(pbuf_tmp,sizeof(pbuf_tmp),(void*)PRESS_FUNC_STR_3);
                os_sprintf(str,pbuf_tmp,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM3(idx,LONG_PRESS));				
				#else
                os_sprintf(str,PRESS_FUNC_STR_3,key_tag[idx],fun_tag[pos].Tag,key_tag[idx],FUNC_PARAM3(idx,LONG_PRESS));
                #endif
				return TRUE;
                break;
            default:
                ACTION_DBG("the key long press not add key=%c\n",key_tag[idx]);
                return FALSE;
                break;
        }
    }
    return FALSE;
    
}

void ICACHE_FLASH_ATTR
UserResponseFuncMap(char* data_buf,uint16 buf_len)
{
    //char AllStr[500];
    char data[100];
    int8 i=0;
	int ltmp = 0;
    //os_bzero(data_buf,sizeof(data_buf));
    os_bzero(data_buf,buf_len);
    
    os_sprintf(data_buf,"{\"status\":\"200\"");

    for(i=0;i<=MAX_COMMAND_COUNT;i++){
        os_bzero(data,sizeof(data));
        if(UserGetShortPressFuncString(i,data)){
            os_sprintf(data_buf+os_strlen(data_buf),"%s",data);
        }
        os_bzero(data,sizeof(data));
        if(UserGetLongPressFuncString(i, data)){
            os_sprintf(data_buf+os_strlen(data_buf),"%s",data);
        }
    }
    os_sprintf(data_buf+os_strlen(data_buf),"%s","}\r\n");
    ACTION_DBG("data_buf :%s\n",data_buf);
}


#define STRCMP_PRIATR(a,b,c) (os_strcmp(a,c) b 0)
int8 ICACHE_FLASH_ATTR
UserGetCorrespondFunction(const char * string)
{
    int8 j=0;
    for(j=0;j<MAX_COMMAND_COUNT;j++){
        if(STRCMP_PRIATR(string,==,fun_tag[j].Tag)){
            return j;
        }
     }
     ACTION_DBG("UserGetCorrespondFunction ERR!  THE String:%s\n",string);
     return -1;
}

#if 1
//to be improved
bool ICACHE_FLASH_ATTR
UserGetParamInt(const char* pParamStr, int* val, uint8 idx)
{
    char* pHead = (char*)pParamStr;
	char* pStop = NULL;

	while(idx>0){
		pHead = os_strchr(pHead,',');
		if(!pHead) return false;
		idx--;
		pHead++;
	}
	//ACTION_DBG("debug: parse ptr: %s \r\n",pHead);
	*val = strtol(pHead,NULL,10);
    return true;
}
#endif


bool ICACHE_FLASH_ATTR
UserGetCmdParam(uint8 fun_pos,const char* pStr,EspNowCmdParam* param)
{
    /*
    fun1 :Get Pwm Param
    fun2: Get on or off
    fun3: Get time_ms
    */
    if(fun_pos>=MAX_COMMAND_COUNT){//
        ACTION_DBG("The fun_pos err  fun_pos=%d!\n",fun_pos);
    	return FALSE;
    }
    if(!pStr){//the string is empty ==err
        ACTION_DBG("UserGetCmdParam the stirng err!!!\n");
    	 return FALSE;
    }
    if(os_strcmp(pStr,"nil")==0){
         ACTION_DBG("the fun_pos=%d is nil!!!\n",fun_pos);
         return FALSE;
    }

	int bcast_if = 0;
	int brighten_flg = 0;
	ACTION_DBG("pStr: %s \r\n",pStr);
    switch(fun_pos){
       case(0):{//the function 1 ,should give r g b param
           int r=0,g=0,b=0;
           if(!UserGetParamInt(pStr, &bcast_if, 0)) return false;
           if(!UserGetParamInt(pStr, &r, 1)) return false;
           if(!UserGetParamInt(pStr, &g, 2)) return false;
           if(!UserGetParamInt(pStr, &b, 3)) return false;
		   param->R = r;
		   param->G = g;
		   param->B = b;
		   param->broadcast_if = bcast_if;
		   ACTION_DBG("r:%d;g:%d;b:%d;bcst:%d;",param->R,param->G,param->B,param->broadcast_if);
           break;
       }
       case(1):{//the function 2 ,should give  on offf  param
           int on_off=0;			   
           if(!UserGetParamInt(pStr, &on_off, 1)) return false;
           if(!UserGetParamInt(pStr, &bcast_if, 0)) return false;
           ACTION_DBG("parse res: on/off: %d , bcast: %d \r\n",on_off,bcast_if);
           if(on_off!=0&&on_off!=1&&on_off!=-1){
               ACTION_DBG("ON OFF err!!! on_off=%d\n",on_off);
               return FALSE;
           } else{
               param->on_off=on_off;
               param->broadcast_if=bcast_if;
           }
           break;
       }
       case(2):{//the function 3 ,should give  time  param
           int time=0;
           if(!UserGetParamInt(pStr, &time, 1)) return false;
           if(!UserGetParamInt(pStr, &bcast_if, 0)) return false;
           ACTION_DBG("parse res: time: %d , bcast: %d \r\n",time,bcast_if);
           if(time<0||time>3600) return false;
           param->timer_ms=time;
           param->broadcast_if = bcast_if;
		   break;

       }
	   case 3: 
	   	   if(!UserGetParamInt(pStr, &bcast_if, 0)) return false;
		   if(!UserGetParamInt(pStr, &brighten_flg, 1)) return false;
		   ACTION_DBG("parse res: brighten_flg: %d ; broadcast: %d \r\n",brighten_flg,bcast_if);
		   if(brighten_flg>1) return false;
		   param->broadcast_if = bcast_if;
		   param->brightness = brighten_flg;
		   break;
       default:{
           ACTION_DBG("The fun is not add fun=%d!\n",fun_pos);
           return FALSE;
       }
       
    }
    return TRUE;
  
}

bool ICACHE_FLASH_ATTR
UserExecuteEspnowCmd(uint16 key_value)
{
    uint16 i=0;
	int8 cmd_idx;
    for(i=0;i<MAX_COMMAND_COUNT;i++){
        if(userActionParam.userAction[i].key_value==(key_value&0xff)){//find the key 
            ACTION_DBG("---------------------------------\n");
            ACTION_DBG("the key value 0x%x\n",key_value);
            if(key_value>>8){//find long press
                cmd_idx = userActionParam.userAction[i].user_long_press_func_pos;
                if(cmd_idx!=-1){
                    fun_tag[cmd_idx].user_press_func(&(userActionParam.userAction[i].press_arg[1]));
                }else{
                    ACTION_DBG("the key =%04x,but long press function is null\n",key_value);
                    return FALSE;
                }
            }
            else{//Short press
                cmd_idx = userActionParam.userAction[i].user_short_press_func_pos;
                if(cmd_idx!=-1){
                    fun_tag[cmd_idx].user_press_func(&(userActionParam.userAction[i].press_arg[0]));
                }else{
                    ACTION_DBG("the key =%04x,but short press function is null\n",key_value);
                    return FALSE;
                }
            }
            ACTION_DBG("---------------------------------\n");
            return TRUE;
        }
    }
    ACTION_DBG("Can not find cmd %d\n",key_value );
    return FALSE;
}
#endif
