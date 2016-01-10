#include "user_config.h"

#if ESP_SIMPLE_PAIR_SUPPORT
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_webserver.h"
#include "user_simplepair.h"
#include "user_light_hint.h"
#include "mem.h"
#include "user_light_mesh.h"

uint8 pair_dst_addr[ESP_MESH_ADDR_LEN];
PairedButtonParam PairedDev;
#define ISDIGIT(c) (('0'<=c&&c<='9')?1:0)

#if STRING_SV_IN_FLASH
const static char REQUEST_PERMISSION[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"device_mac\":\"%02X%02X%02X%02X%02X%02X\",\"button_mac\":\"%02X%02X%02X%02X%02X%02X\",\"path\":\"%s\"}\n";
const static char REPORT_RESULT[] ICACHE_RODATA_ATTR STORE_ATTR = "{\"device_mac\":\"%02X%02X%02X%02X%02X%02X\",\"button_mac\":\"%02X%02X%02X%02X%02X%02X\",\"result\":%d,\"path\":\"%s\"}";
#else
#define REQUEST_PERMISSION  "{\"device_mac\":\"%02X%02X%02X%02X%02X%02X\",\"button_mac\":\"%02X%02X%02X%02X%02X%02X\",\"path\":\"%s\"}\n"
#define REPORT_RESULT "{\"device_mac\":\"%02X%02X%02X%02X%02X%02X\",\"button_mac\":\"%02X%02X%02X%02X%02X%02X\",\"result\":%d,\"path\":\"%s\"}"
#endif



void ICACHE_FLASH_ATTR sp_PrintBuf(uint8* data,uint8 len);
//deinit peer info for espnow
void ICACHE_FLASH_ATTR sp_PairedDevRmAll(PairedButtonParam* buttonParam)
{
    int i;
    for(i=0;i<buttonParam->PairedNum;i++){
        esp_now_del_peer(buttonParam->PairedList[i].mac_t);
    }
}
//load all paired dev info
void ICACHE_FLASH_ATTR sp_PairedDevLoadAll(PairedButtonParam* buttonParam)
{
    int i;
    for(i=0;i<buttonParam->PairedNum;i++){
        esp_now_add_peer(buttonParam->PairedList[i].mac_t,ESP_NOW_ROLE_CONTROLLER,0,buttonParam->PairedList[i].key_t,ESPNOW_KEY_LEN);
    } 
}

//save paired param
void ICACHE_FLASH_ATTR sp_PairedParamSave(PairedButtonParam* buttonParam)
{
    buttonParam->magic = SP_PARAM_MAGIC;
    config_ParamCsumSet(buttonParam,&(buttonParam->csum),sizeof(PairedButtonParam));
    config_ParamSaveWithProtect(LIGHT_PAIRED_DEV_PARAM_ADDR,(uint8*)buttonParam ,sizeof(PairedButtonParam));
    config_ParamCsumCheck(buttonParam,sizeof(PairedButtonParam));
}

//reset the paired param
void ICACHE_FLASH_ATTR sp_PairedDevParamReset(PairedButtonParam* buttonParam,uint8 max_num)
{
    os_memset(buttonParam,0,sizeof(PairedButtonParam));
    buttonParam->MaxPairedDevNum = (max_num>MAX_BUTTON_NUM)?MAX_BUTTON_NUM:max_num;
    buttonParam->PairedNum=0;
    sp_PairedParamSave(buttonParam);
}

//init paired device
void ICACHE_FLASH_ATTR
sp_MacInit()
{
    SP_DBG("SP MAC INIT...\r\n");
	os_printf("sizeof PairedButtonParam PairedDev: %d \r\n",sizeof(PairedDev));


    config_ParamLoadWithProtect(LIGHT_PAIRED_DEV_PARAM_ADDR,0,(uint8*)&PairedDev,sizeof(PairedDev));
    if(config_ParamCsumCheck((uint8*)&PairedDev, sizeof(PairedDev)) && PairedDev.magic == SP_PARAM_MAGIC){
        SP_DBG("Paired dev check ok...\r\n");
    }else{
        SP_DBG("Paired dev not valid...\r\n");
        sp_PairedDevParamReset(&PairedDev,MAX_BUTTON_NUM);
    }
    SP_DBG("===============\r\n");
}


int ICACHE_FLASH_ATTR 
sp_FindPairedDev(PairedButtonParam* buttonParam,uint8* button_mac)
{
    uint8 i=0;
    for(i=0;i<buttonParam->MaxPairedDevNum;i++){
        if(0 == os_memcmp(&buttonParam->PairedList[i].mac_t,button_mac,6)){
            return i; 
        }
    } 
    return -1;
}

LOCAL void ICACHE_FLASH_ATTR 
sp_PopPairedDev(PairedButtonParam* buttonParam,uint8 idx)
{
    uint8 i = idx;
	for(i;i<buttonParam->PairedNum-1;i++){
		os_memcpy(&buttonParam->PairedList[i],&buttonParam->PairedList[i+1],sizeof(PairedSingleDev));
	}
    os_memset(&buttonParam->PairedList[buttonParam->PairedNum-1],0,sizeof(PairedSingleDev));
    buttonParam->PairedNum-=1;
}


//add and save paired device. if full,delete the oldest one.
bool ICACHE_FLASH_ATTR sp_AddPairedDev(PairedButtonParam* buttonParam,uint8* button_mac,uint8* key,uint8 channel)
{    
    int i=sp_FindPairedDev(buttonParam,button_mac);
	//already exist,remove the old one, add the new one in the end
    if(i>-1 && i< buttonParam->PairedNum){
        sp_PopPairedDev(buttonParam,i);
    } 
	//add new one at the end
	uint8 idx;
	if(buttonParam->PairedNum < buttonParam->MaxPairedDevNum){
        idx = buttonParam->PairedNum;
		buttonParam->PairedNum++;
	}else{
	    #if ESP_DEBUG_MODE
    	sp_PopPairedDev(buttonParam,1);
		#else
        sp_PopPairedDev(buttonParam,0);
		#endif
		idx = buttonParam->MaxPairedDevNum-1;
		buttonParam->PairedNum = buttonParam->MaxPairedDevNum;
	}
    os_memcpy(&buttonParam->PairedList[idx].mac_t,button_mac,DEV_MAC_LEN);
    os_memcpy(&buttonParam->PairedList[idx].key_t,key,ESPNOW_KEY_LEN);
    buttonParam->PairedList[idx].channel_t = channel;
    sp_PairedParamSave(buttonParam);
    return true;
}

//search and delete the button with the given MAC
bool ICACHE_FLASH_ATTR sp_DelPairedDev(PairedButtonParam* buttonParam,uint8* button_mac)
{
    if((buttonParam==NULL)||((buttonParam->PairedNum)==0)){
        return  false;
    }
    uint8 i=0;
    for(i=0;i<buttonParam->PairedNum;i++){
        if(0 == os_memcmp(&buttonParam->PairedList[i],button_mac,DEV_MAC_LEN)){
            sp_PopPairedDev(buttonParam,i);
            sp_PairedParamSave(buttonParam);
            return true;    
        }
    }
    return false;    
}

uint8 ICACHE_FLASH_ATTR sp_GetPairedNum(PairedButtonParam* buttonParam)
{
    return (buttonParam->PairedNum);
}

PairedButtonParam* ICACHE_FLASH_ATTR
sp_GetPairedParam()
{
    return &PairedDev;
}


#if ESP_SIMPLE_PAIR_SUPPORT
bool ICACHE_FLASH_ATTR sp_PairedDevMac2Str(PairedButtonParam* buttonParam,uint8* SaveStrBuffer,uint16 buf_len)
{ 
    #define MACSTR_PRIVATE "%02X%02X%02X%02X%02X%02X"
    uint8 i=0;
    uint8 cnt=sp_GetPairedNum(buttonParam);
    
    SP_DBG("PairedDevNum = %d\n",cnt);
    if(cnt==0x00){
        return true;
    }
    
    uint16 len_cnt = 0;
    for(i=0;i<cnt;i++){
        if(len_cnt>buf_len-12){
            SP_DBG("length error in sp_PairedDevMac2Str\r\n");
            return false;
        }
        os_sprintf(SaveStrBuffer+len_cnt,MACSTR_PRIVATE,MAC2STR(buttonParam->PairedList[i].mac_t));
        len_cnt+=12;
    }
    return true;
    #undef MACSTR_PRIVATE
}

//show paired dev
void ICACHE_FLASH_ATTR sp_DispPairedDev(PairedButtonParam* pairingInfo)
{
    uint8 i=0;
    for(i=0;i<pairingInfo->MaxPairedDevNum;i++){
        SP_DBG("Cnt=%d,PairedNum=%d------------------------\n",i,pairingInfo->PairedNum);
        SP_DBG("MAC:");
        sp_PrintBuf(pairingInfo->PairedList[i].mac_t,6);
        SP_DBG("KEY:");
        sp_PrintBuf(pairingInfo->PairedList[i].key_t,16);
    }
}

void ICACHE_FLASH_ATTR sp_DispDeleteMacList(uint8*Tag,uint8(* List)[DEV_MAC_LEN])
{
    uint8 i=0,j=0;
    SP_DBG("%s",Tag);
    for(i=0;i<MAX_BUTTON_NUM;i++){
        SP_DBG("cnt=%d MAC:",i);
        for(j=0;j<DEV_MAC_LEN;j++){
            SP_DBG("%02x ",List[i][j]);
        }
        SP_DBG("\n");
    }
    SP_DBG("\n");  
}

void ICACHE_FLASH_ATTR
	sp_PrintBuf(uint8 * ch,uint8 len)
{
    uint8 i=0;
    for(i=0;i<len;i++) SP_DBG("%x ",ch[i]);
    SP_DBG("\n");
}

//parse button number
int8 ICACHE_FLASH_ATTR
sp_GetDeletButtonCnt(const char* buff)
{
    char* tag_pos=NULL;
    uint8 cnt;
    if(NULL==buff) return -1;
    tag_pos=(char*)os_strstr(buff,DELET_BUTTON_LEN_TAG);
    if(NULL==tag_pos){
        SP_DBG("buff canot find delet button len tag\n");
        return  -1;
    }
    tag_pos+=10;
    if(','==*(tag_pos+1)){
        cnt=*tag_pos-'0';
        return cnt;
    }else if(ISDIGIT(*(tag_pos+1))){//if the len >10 ,this is  unit
        cnt=(*tag_pos-'0')*10+*(tag_pos+1)-'0';
        if(cnt>MAX_BUTTON_NUM){
            SP_DBG("will delet button cnt=%d  err!!!\n",cnt);
            return -1;
        }
        else return cnt;
    }
    else{
        SP_DBG("delet button mac format err 0x%x\n",*(tag_pos+1));
        return -1;
    }
}

//parse button mac
int8 ICACHE_FLASH_ATTR
sp_GetDeletButtonsMAC(const char* Databuff,uint8 (*SaveMacBuffer)[DEV_MAC_LEN],uint8 Cnt)
{
    char* tag_pos=NULL;
    uint8 cnt=Cnt;
    if(cnt==0||cnt>MAX_BUTTON_NUM){
        SP_DBG("sp_GetDeletButtonsMAC err\n");
        return -1;
    }
    if(NULL==Databuff||NULL==SaveMacBuffer) return -1;
    tag_pos=(char*)os_strstr(Databuff,DELET_BUTTON_MAC_TAG);
    if(NULL==tag_pos){
        SP_DBG("buff canot find delet button mac tag\n");
        return  -1;
    }
    int i,j;
    char tmp[3];
    os_bzero(tmp,sizeof(tmp));
    char* ptmp = tag_pos+7;
    for(i=0;i<cnt;i++){
        for(j=0;j<DEV_MAC_LEN;j++){ 
            os_memcpy(tmp,ptmp,2);
            uint8 val = strtol(tmp,NULL,16);
            SaveMacBuffer[i][j]=val;
            ptmp+=2;
        }
    }
    return 0;
}

//refuse to pair 
void ICACHE_FLASH_ATTR
sp_LightPairRefuse()
{
    simple_pair_set_peer_ref(buttonPairingInfo.button_mac, buttonPairingInfo.tempkey, NULL);
    simple_pair_ap_refuse_negotiate();
    simple_pair_deinit();//check
}

//gen random key for paired dev
void ICACHE_FLASH_ATTR
sp_LightPairGenKey(uint8* key_buf)// length must be 16 bytes
{
    int i;
    uint32 r_v;
    for(i=0;i<4;i++){
        r_v = os_random();
        os_memcpy(key_buf+4*i,&r_v,4);
    }
}


//smart phone permit pairing , try to pair with the slave device
void ICACHE_FLASH_ATTR
sp_LightPairStart()
{
    sp_LightPairGenKey(buttonPairingInfo.espnowKey);
    simple_pair_set_peer_ref(buttonPairingInfo.button_mac, buttonPairingInfo.tempkey, buttonPairingInfo.espnowKey);
    simple_pair_ap_start_negotiate();
    SP_DBG("send mac\n");
    sp_PrintBuf(buttonPairingInfo.button_mac,6);
    SP_DBG("channel =%d\n",wifi_get_channel());
    SP_DBG("tmpkey\n");
    sp_PrintBuf(buttonPairingInfo.tempkey,16);
    SP_DBG("tmpkey lenght=%d\n",sizeof(buttonPairingInfo.tempkey));
    SP_DBG("espnow key: \r\n");
    sp_PrintBuf(buttonPairingInfo.espnowKey,16);
}



//report to smart phone that someone is requesting for pairing, need permission
void ICACHE_FLASH_ATTR
sp_LightPairRequestPermission()
{
    char data_body[200];
    os_bzero(data_body,sizeof(data_body));
    uint8 mac_sta[6] = {0};
    wifi_get_macaddr(STATION_IF, mac_sta);
	#if STRING_SV_IN_FLASH
    char request_permission[200];
	load_string_from_flash(request_permission,sizeof(request_permission),(void*)REQUEST_PERMISSION);
    os_sprintf(data_body,request_permission,MAC2STR(mac_sta),MAC2STR(buttonPairingInfo.button_mac),PAIR_FOUND_REQUEST);	
	#else
    os_sprintf(data_body,REQUEST_PERMISSION,MAC2STR(mac_sta),MAC2STR(buttonPairingInfo.button_mac),PAIR_FOUND_REQUEST);
    #endif
	
	#if ESP_MESH_SUPPORT
        char* dev_mac = (char*)mesh_GetMdevMac();
	    #if MESH_HEADER_MODE
    		struct mesh_header_format* mesh_header = NULL;// (struct mesh_header_format*)espconn->reverse;;
    		SP_DBG("src: "MACSTR"\r\n",MAC2STR(mesh_src_addr));
    		SP_DBG("dst: "MACSTR"\r\n",MAC2STR(pair_dst_addr));
    		if(!mesh_json_add_elem(data_body, sizeof(data_body), dev_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN)) return;			
		#else
    		if(!mesh_json_add_elem(data_body, sizeof(data_body), pair_sip, ESP_MESH_JSON_IP_ELEM_LEN)) return;
    		if(!mesh_json_add_elem(data_body, sizeof(data_body), pair_sport, ESP_MESH_JSON_PORT_ELEM_LEN)) return;
		#endif
        response_send_str((struct espconn*)user_GetUserPConn(),true,data_body,os_strlen(data_body),NULL,0,0,0,mesh_src_addr,pair_dst_addr);//swap src and dst
    #else
        response_send_str((struct espconn*)user_GetWebPConn(),true,data_body,os_strlen(data_body),NULL,0,0,0,NULL,NULL);
    #endif
	    if(mesh_header){
            os_free(mesh_header);
			mesh_header = NULL;
	    }
}





//pairing success, report to the smart phone
void ICACHE_FLASH_ATTR
sp_LightPairReportResult(bool res)
{
    SP_DBG("sp_LightPairReportResult %d \r\n",res);
    char data_body[300];
    os_bzero(data_body,sizeof(data_body));
    uint8 mac_sta[6] = {0};
    wifi_get_macaddr(STATION_IF, mac_sta);
    os_sprintf(data_body,REPORT_RESULT,MAC2STR(mac_sta),MAC2STR(buttonPairingInfo.button_mac),res,PAIR_RESULT);
    #if ESP_MESH_SUPPORT
    	char* dev_mac = (char*)mesh_GetMdevMac();
    	if (!mesh_json_add_elem(data_body, sizeof(data_body), dev_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN)) return;
		#if MESH_HEADER_MODE
    		struct mesh_header_format* mesh_header = NULL;// (struct mesh_header_format*)espconn->reverse;;
    		SP_DBG("src: "MACSTR"\r\n",MAC2STR(mesh_src_addr));
    		SP_DBG("dst: "MACSTR"\r\n",MAC2STR(pair_dst_addr));
		#else
    		if (!mesh_json_add_elem(data_body, sizeof(data_body), pair_sip, ESP_MESH_JSON_IP_ELEM_LEN)) return;
    		if (!mesh_json_add_elem(data_body, sizeof(data_body), pair_sport, ESP_MESH_JSON_PORT_ELEM_LEN)) return;

		#endif
        response_send_str((struct espconn*)user_GetUserPConn(),true,data_body,os_strlen(data_body),NULL,0,0,0,mesh_src_addr,pair_dst_addr);//swap src and dst
    #else
        response_send_str((struct espconn*)user_GetWebPConn(),true,data_body,os_strlen(data_body),NULL,0,0,0,NULL,NULL);
    #endif

	
	if(mesh_header){
		os_free(mesh_header);
		mesh_header = NULL;
	}
}


//reply keep-alive packet with smart phone
void ICACHE_FLASH_ATTR
sp_LightPairReplyKeepAlive(struct espconn* pconn)
{
    char data_resp[100];
    os_bzero(data_resp,sizeof(data_resp));
    os_sprintf(data_resp, "{\"status\":200,\"path\":\"%s\"}",PAIR_KEEP_ALIVE);
    SP_DBG("response send str...\r\n");

    #if ESP_MESH_SUPPORT
	    #if MESH_HEADER_MODE
        response_send_str(pconn, true, data_resp,os_strlen(data_resp),NULL,0,1,0,mesh_src_addr,pair_dst_addr);
		#else
        response_send_str(pconn, true, data_resp,os_strlen(data_resp),NULL,0,1,0,NULL,NULL);
		#endif
    #else
        response_send_str(pconn, true, data_resp,os_strlen(data_resp),NULL,0,1,0,NULL,NULL);
    #endif
}

//report to smart phone paired device info
void ICACHE_FLASH_ATTR
sp_LightReplyPairedDev()
{
    uint8* MacListBuf = (uint8*)os_zalloc(PairedDev.PairedNum*DEV_MAC_LEN*2+1);
    uint8 cnt=sp_GetPairedNum(&PairedDev);
    sp_PairedDevMac2Str(&PairedDev,MacListBuf,sizeof(MacListBuf)-1);
    #if ESP_MESH_SUPPORT
	    #if MESH_HEADER_MODE
        response_send_str((struct espconn*)user_GetUserPConn(), true, MacListBuf,os_strlen(MacListBuf),NULL,0,1,1,mesh_src_addr,pair_dst_addr);
		#else
        response_send_str((struct espconn*)user_GetUserPConn(), true, MacListBuf,os_strlen(MacListBuf),NULL,0,1,1,NULL,NULL);
		#endif
    #else
        response_send_str((struct espconn*)user_GetWebPConn(), true, MacListBuf,os_strlen(MacListBuf),NULL,0,1,1,NULL,NULL);
    #endif
    if(MacListBuf){
        os_free(MacListBuf);
        MacListBuf = NULL;
    }
}

//simple pair status handler
void ICACHE_FLASH_ATTR
sp_LightPairStateCb(uint8 *mac,uint8 state)
{
    SP_DBG("state=%d\n",state);
    SP_DBG("receive mac\n");
    sp_PrintBuf(mac,6);
    if(state==SP_ST_AP_RECV_NEG){   //receive pairing request, ask phone for permission   
        SP_DBG("SHOULD WE CHECK MAC HERE...\r\n");
        os_memcpy(buttonPairingInfo.button_mac,mac,DEV_MAC_LEN);
        buttonPairingInfo.simple_pair_state=LIGHT_RECV_BUTTON_REQUSET;
    }else if(state==SP_ST_AP_FINISH){
        buttonPairingInfo.simple_pair_state=LIGHT_SIMPLE_PAIR_SUCCED;
        SP_DBG("pair succeed\n");
    }else{
        SP_DBG("---------------\r\n");
        SP_DBG("simple pair state: %d \r\n",state);
        SP_DBG("---------------\r\n");
		buttonPairingInfo.simple_pair_state=LIGHT_SIMPLE_PAIR_FAIL;
    }
    sp_LightPairState(); 
}

//get into pairing mode, wait for pairing request from slave device
void ICACHE_FLASH_ATTR
sp_LightPairAccept(void)
{
    simple_pair_deinit();
    simple_pair_init();
    register_simple_pair_status_cb(sp_LightPairStateCb);
    simple_pair_ap_enter_scan_mode();
}

LOCAL os_timer_t lightTout_t;
LOCAL enum SimplePairStatus light_state = SP_LIGHT_IDLE;
//pair time out function
void ICACHE_FLASH_ATTR
sp_LightPairTout(void)
{
    if(LIGHT_SIMPLE_PAIR_SUCCED!=buttonPairingInfo.simple_pair_state){
        SP_DBG("sp_LightPairTout the simple pair fail  timeout\n");
        buttonPairingInfo.simple_pair_state=SP_LIGHT_ERROR_HANDLE;
        sp_LightPairState();
    }else{
        SP_DBG("sp_LightPairTout the simple pair succed\n");
    }
}

//start pair timeout count down
void ICACHE_FLASH_ATTR
sp_LightPairTimerStart()
{
    os_timer_disarm(&lightTout_t);
    os_timer_setfn(&lightTout_t, (os_timer_func_t *)sp_LightPairTout, NULL);
    os_timer_arm(&lightTout_t, 60*1000, 0);
}

//stop pair check timer
void ICACHE_FLASH_ATTR
sp_LightPairTimerStop()
{
    os_timer_disarm(&lightTout_t);
}

//process simple pair status
void ICACHE_FLASH_ATTR
sp_LightPairState(void)
{
    switch(light_state){
        case (SP_LIGHT_IDLE):
            SP_DBG("status:SP_LIGHT_IDLE\n");
            //pair start command, correct
            if(USER_PBULIC_BUTTON_INFO==buttonPairingInfo.simple_pair_state){
                SP_DBG("statue:Get button Info,next wait button request\n");
                sp_LightPairTimerStart();
                sp_LightPairAccept();
                light_state=SP_LIGHT_WAIT_BUTTON_REQUEST;
                light_shadeStart(HINT_WHITE,500,3,1,NULL);
            }
            //exception
            else{
                light_state=SP_LIGHT_ERROR_HANDLE;
                light_hint_stop(HINT_RED);
                SP_DBG("buttonPairingInfo.simple_pair_state=%d\n",buttonPairingInfo.simple_pair_state);
                sp_LightPairState();
            }
            break;
        case (SP_LIGHT_WAIT_BUTTON_REQUEST):
            SP_DBG("status:SP_LIGHT_WAIT_BUTTON_REQUEST\n");
            light_shadeStart(HINT_GREEN,500,3,0,NULL);
            //receive button pair request, send req to phone
            if(LIGHT_RECV_BUTTON_REQUSET==buttonPairingInfo.simple_pair_state){
                SP_DBG("statue:Get button request,next wait user permit or refuse\n"); 
                sp_LightPairRequestPermission();
                light_state=SP_LIGHT_WAIT_USER_INDICATE_PERMIT;
            }
            //receive pair start command again before timeout , right now, restart state machine
            else if(USER_PBULIC_BUTTON_INFO==buttonPairingInfo.simple_pair_state){
                SP_DBG("statue:Get button Info,restart state machine,wait button request\n");
                sp_LightPairTimerStart();
                sp_LightPairAccept();
                light_state=SP_LIGHT_WAIT_BUTTON_REQUEST;
            }
            //error if other states
            else{
                //simple_pair_deinit();
                light_state=SP_LIGHT_ERROR_HANDLE;
                light_hint_stop(HINT_RED);
                SP_DBG("err in SP_LIGHT_WAIT_BUTTON_REQUEST ;simple_pair_state=%d\n",buttonPairingInfo.simple_pair_state);
                sp_LightPairState();
            }
            break;
        case (SP_LIGHT_WAIT_USER_INDICATE_PERMIT):
            light_shadeStart(HINT_BLUE,500,3,0,NULL);
            SP_DBG("status:SP_LIGHT_WAIT_USER_INDICATE_PERMIT\n");
            //phone user permit pairing
            if(USER_PERMIT_SIMPLE_PAIR==buttonPairingInfo.simple_pair_state){
                SP_DBG("statue:User permit simple pair,next wait simple result\n"); 
                //sp_PairedDevRmAll(&PairedDev);//
                esp_now_del_peer(buttonPairingInfo.button_mac);
				SP_DBG("del peer:"MACSTR"\r\n",MAC2STR(buttonPairingInfo.button_mac));
                sp_LightPairStart();
                light_state=SP_LIGHT_WAIT_SIMPLE_PAIR_RESULT;
            }
            //phone user refuse pairing, END
            else if(USER_REFUSE_SIMPLE_PAIR==buttonPairingInfo.simple_pair_state){
                SP_DBG("statue:User refuse simple pair,state clear\n");
                //check state
                sp_LightPairRefuse();
                light_state = SP_LIGHT_END;
                light_shadeStart(HINT_RED,500,2,0,NULL);
                sp_LightPairState();
            }
            //exceptions
            else{
                //simple_pair_deinit();
                light_state=SP_LIGHT_ERROR_HANDLE;
                light_hint_stop(HINT_RED);
                SP_DBG("buttonPairingInfo.simple_pair_state=%d\n",buttonPairingInfo.simple_pair_state);
                sp_LightPairState();
            }
            break;
        case (SP_LIGHT_WAIT_SIMPLE_PAIR_RESULT):
            SP_DBG("status:SP_LIGHT_WAIT_SIMPLE_PAIR_RESULT\n");
            //pairing finished , END
            if(LIGHT_SIMPLE_PAIR_SUCCED==buttonPairingInfo.simple_pair_state){
                SP_DBG("status:sp_LightPairSucced\n");  
                light_shadeStart(HINT_WHITE,500,2,0,NULL);
                //1.get key
                //buttonPairingInfo.espnowKey
                //2.add peer and save
                int res = esp_now_add_peer((uint8*)(buttonPairingInfo.button_mac), (uint8)ESP_NOW_ROLE_CONTROLLER,(uint8)wifi_get_channel(), (uint8*)(buttonPairingInfo.espnowKey), (uint8)ESPNOW_KEY_LEN);
                SP_DBG("INIT RES: %d ; MAC:"MACSTR"\r\n",res,MAC2STR(((uint8*)(buttonPairingInfo.button_mac))));
                //3.save
                sp_AddPairedDev(&PairedDev,buttonPairingInfo.button_mac,buttonPairingInfo.espnowKey,wifi_get_channel());
                sp_DispPairedDev(&PairedDev);
                sp_LightPairReportResult(true);
                light_state = SP_LIGHT_END;
                sp_LightPairState();
            }
            //pairing failed , END
            else if(LIGHT_SIMPLE_PAIR_FAIL==buttonPairingInfo.simple_pair_state){
                SP_DBG("status:sp_LightPairFail\n");
                sp_LightPairReportResult(false);
                light_state = SP_LIGHT_END;
                light_hint_stop(HINT_RED);
                sp_LightPairState();
            }
            //exception
            else{
                simple_pair_deinit();
                light_hint_stop(HINT_RED);
                light_state=SP_LIGHT_ERROR_HANDLE;
                SP_DBG("buttonPairingInfo.simple_pair_state=%d\n",buttonPairingInfo.simple_pair_state);
                sp_LightPairState();
            }
            break; 
        case (SP_LIGHT_ERROR_HANDLE):
            SP_DBG("status:simpaire in Err!!!\r\nBACK TO IDLE\r\n"); 
            light_hint_stop(HINT_RED);
			
        case (SP_LIGHT_END):
            //reset state, deinit simplepair, stop tout timer
            sp_LightPairTimerStop();
            simple_pair_deinit();
            light_state=SP_LIGHT_IDLE;
            buttonPairingInfo.simple_pair_state=LIGHT_PAIR_IDLE;
            ////sp_PairedDevLoadAll(&PairedDev);
            break;
        default:
            SP_DBG("status:unsafe param: %d\n",light_state);
            break;
    }
}
#endif
#endif

