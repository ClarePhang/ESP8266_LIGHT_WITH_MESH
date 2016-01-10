#include "user_config.h"
uint8 DEBUG_FLG = false;

#if ESP_DEBUG_MODE
#include "user_debug.h"
#include "ringbuf.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"
#include "user_debug.h"

#if ESP_MESH_SUPPORT
#include "mesh.h"
#endif

flashDebugBuf FlashDebugBufParam;

struct rst_info rtc_info_dbg;
#define DEBUG_UPLOAD_BUF_LEN 500

#define UPLOAD_DEBUG_LOG  "{\"path\": \"/v1/device/debugs/\", \"method\": \"POST\", \
\"body\": {\"debugs\": [{\"type\": 1,\"errno\": 100,\"message\":\"[%d,%d,0x%08x,0x%08x,0x%08x,%d,%d,%d]\"}]}, \"meta\": {\"Authorization\": \"token %s\"}}\n"

//cal checksum and check
bool ICACHE_FLASH_ATTR
debug_FlashParamCsumCheck(flashDebugBuf* flashDbgBuf)
{
    uint8 csum_cal=0;
    int i ;
    char* tmp = (char*)(flashDbgBuf);
    for(i=1;i<sizeof(flashDebugBuf);i++){
        csum_cal+= *(tmp+i);
    }
    ESP_DBG("flash csum cal: %d ; ori: %d \r\n",csum_cal,flashDbgBuf->csum);
    if(csum_cal==flashDbgBuf->csum)
        return true;
    else
        return false;
}

//cal checksum and check
void ICACHE_FLASH_ATTR
    debug_FlashParamCsumSet(flashDebugBuf* flashDbgBuf)
{
    uint8 csum_cal=0;
    int i;
    char* tmp = (char*)(flashDbgBuf);
    for(i=1;i<sizeof(flashDebugBuf);i++){
        csum_cal+= *(tmp+i);
    }
    flashDbgBuf->csum = csum_cal;
}

//save flash debug param
void ICACHE_FLASH_ATTR
    debug_FlashParamSv(flashDebugBuf* flashDbgBuf,uint32 sec_addr)
{
	ESP_DBG("debug_FlashParamSv\r\n");

    debug_FlashParamCsumSet(flashDbgBuf);
	
    //spi_flash_erase_sector(Flash_DEBUG_INFO_ADDR);
    //spi_flash_write(Flash_DEBUG_INFO_ADDR*0x1000,(uint32*)&FlashDebugBufParam,sizeof(FlashDebugBufParam));  
	config_ParamSaveWithProtect(sec_addr,(uint32*)flashDbgBuf,sizeof(flashDebugBuf));
}

//check flash debug info address
bool ICACHE_FLASH_ATTR
    debug_FlashAddrCheck(flashDebugBuf* flashDbgBuf)
{ 
#if 0
    if(FlashDebugBufParam.InPos >= FlashDebugBufParam.OutPos  && 
        FlashDebugBufParam.InPos< FlashDebugBufParam.StartAddr+FlashDebugBufParam.Size &&
        FlashDebugBufParam.InPos>=FlashDebugBufParam.StartAddr){
        ESP_DBG("debug flash addr check ok...\r\n");
        return true;
    }else{
        ESP_DBG("debug flash addr check error...\r\n");
        return false;
    }
#endif

    if((flashDbgBuf->InPos>=flashDbgBuf->StartAddr)&&(flashDbgBuf->InPos<=flashDbgBuf->StartAddr+flashDbgBuf->Size)
		&&(flashDbgBuf->OutPos>=flashDbgBuf->StartAddr)&&(flashDbgBuf->OutPos<=flashDbgBuf->StartAddr+flashDbgBuf->Size)){
		ESP_DBG("debug flash addr check ok...\r\n");
		return true;
	}else{
		ESP_DBG("debug flash addr check error...\r\n");
		return false;
	}	
}


flashDebugBuf FlashDumpBufParam;
//flash dump buf reset
void ICACHE_FLASH_ATTR
	debug_DumpBufReset()
{
    FlashDumpBufParam.StartAddr = FLASH_DEBUG_DUMP_ADDR*0x1000;
    FlashDumpBufParam.Size = FLASH_DEBUG_SV_SIZE;
    FlashDumpBufParam.InPos = FLASH_DEBUG_DUMP_ADDR*0x1000;
    FlashDumpBufParam.OutPos = FLASH_DEBUG_DUMP_ADDR*0x1000;
	FlashDumpBufParam.DebugVersion = MESH_TEST_VERSION;    
}

//write string to flash
void ICACHE_FLASH_ATTR
debug_DumpToFlash(uint8* data, uint16 len)
{
    if(len%4 != 0){
        ESP_DBG("4Bytes ...\r\n");
        return;
    }
    
    if(FlashDumpBufParam.InPos % 0x1000 ==0)
        spi_flash_erase_sector(FlashDumpBufParam.InPos / 0x1000);

    if(FlashDumpBufParam.InPos+len <  FlashDumpBufParam.StartAddr+FlashDumpBufParam.Size ){
        spi_flash_write(FlashDumpBufParam.InPos,(uint32 *)data,len);
		ESP_DBG("addr: 0x%08x :  %d \r\n",FlashDumpBufParam.InPos,len);
        FlashDumpBufParam.InPos+=len;
    }
}

//reset debug buf param
void ICACHE_FLASH_ATTR
debug_FlashBufReset(flashDebugBuf* flashDbgBuf,uint32 idx_sec)
{
    flashDbgBuf->StartAddr = FLASH_DEBUG_SV_ADDR*0x1000;
    flashDbgBuf->Size = FLASH_DEBUG_SV_SIZE;
    flashDbgBuf->InPos = FLASH_DEBUG_SV_ADDR*0x1000;
    flashDbgBuf->OutPos = FLASH_DEBUG_SV_ADDR*0x1000;
	flashDbgBuf->DebugVersion = MESH_TEST_VERSION;
	flashDbgBuf->CurrentSec = FLASH_DEBUG_SV_ADDR;
    debug_FlashParamSv(flashDbgBuf,idx_sec);
}

//FLASH buf init
void ICACHE_FLASH_ATTR
debug_FlashBufInit(flashDebugBuf* flashDbgBuf,uint32 idx_sec)
{
    //spi_flash_read(idx_sec*0x1000,(uint32*)flashDbgBuf,sizeof(flashDebugBuf));
	config_ParamLoadWithProtect(idx_sec,0,(uint8*)flashDbgBuf,sizeof(flashDebugBuf));

	if(debug_FlashParamCsumCheck(flashDbgBuf) && debug_FlashAddrCheck(flashDbgBuf)){
        ESP_DBG("flash debug check sum ok..\r\n");
    }else{
        ESP_DBG("flash debug info reset...\r\n");
        debug_FlashBufReset(flashDbgBuf,idx_sec);
    }
}

//return debug version
int ICACHE_FLASH_ATTR
debug_GetDebugVersion()
{
    return FlashDebugBufParam.DebugVersion;
}

//set debug version
void ICACHE_FLASH_ATTR
debug_SetDebugVersion(int ver)
{
    if(debug_FlashParamCsumCheck(&FlashDebugBufParam) && debug_FlashAddrCheck(&FlashDebugBufParam)){
        //FlashDebugBufParam.DebugVersion = ver;
    }else{
        debug_FlashBufInit(&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
    }
    FlashDebugBufParam.DebugVersion = ver;
    //debug_FlashParamSv();
    debug_FlashParamSv(&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
}

//save exception info and upload to server later
void ICACHE_FLASH_ATTR
    debug_SvExceptionInfo(struct rst_info* pInfo)
{
    os_memset(&rtc_info_dbg,0,sizeof(struct rst_info));
    os_memcpy(&rtc_info_dbg,pInfo,sizeof(struct rst_info));
}

//clear debug info after uploaded to server
void ICACHE_FLASH_ATTR
    debug_DropExceptionInfo()
{
    os_memset(&rtc_info_dbg,0,sizeof(struct rst_info));
}

//write debug info to flash
void ICACHE_FLASH_ATTR
debug_PrintToFlash(uint8* data, uint16 len,flashDebugBuf* flashDbgBuf,uint32 idx_sector)
{
    ESP_DBG("debug_PrintToFlash\r\n");
    if(len%4 != 0){
        ESP_DBG("4Bytes ...\r\n");
        return;
    }
    
    if(debug_FlashAddrCheck(flashDbgBuf)){
        //if( (flashDbgBuf->InPos == flashDbgBuf->StartAddr &&flashDbgBuf->InPos == flashDbgBuf->OutPos) || 
			if((flashDbgBuf->InPos+len)/0x1000 == flashDbgBuf->CurrentSec+1){
			#if 1
			flashDbgBuf->CurrentSec = (flashDbgBuf->InPos+len>=flashDbgBuf->StartAddr+flashDbgBuf->Size)?
				                              flashDbgBuf->StartAddr/0x1000:(flashDbgBuf->InPos+len)/0x1000 ;
            //ESP_DBG("out: %08x; flashDbgBuf->CurrentSec*0x1000: %08x \r\n",flashDbgBuf->OutPos,flashDbgBuf->CurrentSec*0x1000);
			if((flashDbgBuf->OutPos<flashDbgBuf->CurrentSec*0x1000+0x1000) && (flashDbgBuf->OutPos>=flashDbgBuf->CurrentSec*0x1000)){
                flashDbgBuf->OutPos = flashDbgBuf->CurrentSec*0x1000 + 0x1000;
				if(flashDbgBuf->OutPos>=flashDbgBuf->StartAddr+flashDbgBuf->Size) flashDbgBuf->OutPos-=flashDbgBuf->Size;
			}
			//ESP_DBG("after set: out: %08x \r\n",flashDbgBuf->OutPos);
			#else
			

			if(flashDbgBuf->InPos+len>flashDbgBuf->StartAddr+flashDbgBuf->Size){
                flashDbgBuf->CurrentSec = flashDbgBuf->StartAddr/0x1000;
                ESP_DBG("overflow,loop back\r\n");
				if(flashDbgBuf->OutPos<flashDbgBuf->CurrentSec*0x1000+0x1000){
                    flashDbgBuf->OutPos = flashDbgBuf->CurrentSec*0x1000 + 0x1000;
    			}
			}else{
                flashDbgBuf->CurrentSec = (flashDbgBuf->InPos+len)/0x1000;
			}
			#endif
			spi_flash_erase_sector(flashDbgBuf->CurrentSec);		
        }
    }else{
        //debug_FlashBufInit(flashDbgBuf,idx_sector);
        ESP_DBG("error1\r\n");
    }
	#if 0
    ESP_DBG("================================\r\n");
	ESP_DBG("test data: LEN: %d\r\n",len);
	ESP_DBG("%s \r\n",data);
	ESP_DBG("================================\r\n");
	#endif
	
    if(flashDbgBuf->InPos+len <  flashDbgBuf->StartAddr+flashDbgBuf->Size ){
		ESP_DBG("in: %08x ,len: %d \r\n",flashDbgBuf->InPos,len);
        spi_flash_write(flashDbgBuf->InPos,(uint32 *)data,len);
        flashDbgBuf->InPos+=len;
        //debug_FlashParamSv();
		//debug_FlashParamSv(flashDbgBuf,Flash_DEBUG_INFO_ADDR);
		debug_FlashParamSv(flashDbgBuf,idx_sector);
		ESP_DBG("1\r\n");
    }else{
    
        ESP_DBG("in: %08x ,len: %d \r\n",flashDbgBuf->InPos,len);
		
        int len_tmp = flashDbgBuf->StartAddr+flashDbgBuf->Size - flashDbgBuf->InPos;
	    int len_remain = len-len_tmp;
		spi_flash_write(flashDbgBuf->InPos,(uint32 *)data,len_tmp);
		spi_flash_write(flashDbgBuf->StartAddr,(uint32 *)(data+len_tmp),len_remain);
		flashDbgBuf->InPos = flashDbgBuf->StartAddr+len_remain;
		//debug_FlashParamSv(flashDbgBuf,Flash_DEBUG_INFO_ADDR);
		debug_FlashParamSv(flashDbgBuf,idx_sector);
		ESP_DBG("2\r\n");
    }
}

//write mesh reconn info to flash
void ICACHE_FLASH_ATTR
debug_FlashSvMeshReconInfo(char* time_str,sint8 err)
{

    uint8 reconInfo[200];
	int len;
	os_memset(reconInfo,0,sizeof(reconInfo));
	
    struct ip_info ipconfig;
    wifi_get_ip_info(STATION_IF, &ipconfig);



	len = os_sprintf(reconInfo,"MESH RECONN AT %s,ERR:%d,IP:"IPSTR";\r\n",time_str,err,IP2STR(&(ipconfig.ip)));
	
    //int len = os_strlen(reconInfo);
	char* ptmp = reconInfo + len;
	
    uint8 pad_len = 0;
    if(len%4 != 0){
        pad_len = 4 - (len%4);
        os_memcpy(ptmp,"   ",pad_len);
    }
    len += pad_len;

	
	ESP_DBG("%s\r\n",reconInfo);
    debug_PrintToFlash(reconInfo,len,&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);


}



//write reset info to flash
void ICACHE_FLASH_ATTR
debug_FlashSvExceptInfo(struct rst_info* pInfo)
{
    debug_FlashBufInit(&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
    uint8 InfoBuf[200];
    os_memset(InfoBuf,0,200);
    uint8* ptmp = InfoBuf;
    
    ptmp += os_sprintf(ptmp,"reset reason:%x\n", pInfo->reason);
    
    if (pInfo->reason == REASON_WDT_RST ||
        pInfo->reason == REASON_EXCEPTION_RST ||
        pInfo->reason == REASON_SOFT_WDT_RST) {
        if (pInfo->reason == REASON_EXCEPTION_RST) {
            ptmp += os_sprintf(ptmp,"Fatal exception (%d):\n", pInfo->exccause);
        }
        ptmp+=os_sprintf(ptmp,"debug_version:%d\r\n",FlashDebugBufParam.DebugVersion);
        ptmp+=os_sprintf(ptmp,"epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\r\n",
                pInfo->epc1, pInfo->epc2, pInfo->epc3, pInfo->excvaddr, pInfo->depc);
    }

    int len = os_strlen(InfoBuf);
    uint8 pad_len = 0;
    if(len%4 != 0){
        pad_len = 4 - (len%4);
        os_memcpy(ptmp,"   ",pad_len);
    }
    len += pad_len;
	
    ESP_DBG("============\r\nSV DBUG INFO:\r\n%s\r\n==============\r\n",InfoBuf);
    debug_PrintToFlash(InfoBuf,len,&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
}


#if 0
//not used
void ICACHE_FLASH_ATTR
debug_FlashUpdateExpetInfo(uint16 len)
{
    if(debug_FlashAddrCheck(&FlashDebugBufParam) && (FlashDebugBufParam.OutPos+len <= FlashDebugBufParam.InPos)){
        FlashDebugBufParam.InPos-=len;
        //debug_FlashParamSv();
		debug_FlashParamSv(&FlashDebugBufParam,Flash_DEBUG_INFO_ADDR);
    }else{
        //return false;
    }
}
#endif


void ICACHE_FLASH_ATTR
    debug_DispFlashExceptInfo(flashDebugBuf* flashDbgBuf)
{
    ESP_DBG("flash debug : version: %d \r\n",flashDbgBuf->DebugVersion);
    ESP_DBG("flash debug : InPos: %08x\r\n",flashDbgBuf->InPos);
    ESP_DBG("flash debug : OutPos: %08x\r\n",flashDbgBuf->OutPos);
    uint8* debug_str = NULL;
    
    int size = (flashDbgBuf->InPos-flashDbgBuf->OutPos);
        
    if(size>0){
        debug_str = (uint8*)os_zalloc(size+1);
        spi_flash_read(flashDbgBuf->OutPos,(uint32*)debug_str,size);
        ESP_DBG("-----------------------\r\n");
        ESP_DBG("FLASH DEBUG INFO: \r\n");
        ESP_DBG("%s\r\n",debug_str);
        ESP_DBG("-----------------------\r\n");
    }
	if(debug_str){
        os_free(debug_str);
		debug_str = NULL;
	}
	
}

int ICACHE_FLASH_ATTR
    debug_GetFlashDebugInfoLen(flashDebugBuf* flashDbgBuf)
{
    //return (FlashDebugBufParam.InPos-FlashDebugBufParam.OutPos);
    if(flashDbgBuf->InPos>=flashDbgBuf->OutPos)
		return flashDbgBuf->InPos-flashDbgBuf->OutPos;
	else
		return flashDbgBuf->Size - (flashDbgBuf->OutPos-flashDbgBuf->InPos);
}

//read exception info
void ICACHE_FLASH_ATTR
    debug_GetFlashExceptInfoForce(uint8* buf, uint16 length,uint16 offset)
{
	spi_flash_read(FLASH_DEBUG_SV_ADDR*0x1000+offset,(uint32*)buf,length);
}

//read flash data
void ICACHE_FLASH_ATTR
    debug_GetFlashDataForce(uint8* buf, uint32 addr,uint16 length)
{
	spi_flash_read(addr,(uint32*)buf,length);
}

//get flash debug info
void ICACHE_FLASH_ATTR
    debug_GetFlashExceptInfo(uint8* buf, uint16 max_len,uint16 offset)
{
    ESP_DBG("flash debug : InPos: %08x\r\n",FlashDebugBufParam.InPos);
    ESP_DBG("flash debug : OutPos: %08x\r\n",FlashDebugBufParam.OutPos);    
    int size = (FlashDebugBufParam.InPos-FlashDebugBufParam.OutPos-offset);
    if(size>max_len) size = max_len;

    offset = offset - offset%4;
    if(size>0){
        //debug_str = (uint8*)os_zalloc(size+1);
        spi_flash_read(FlashDebugBufParam.OutPos+offset,(uint32*)buf,size);
        ESP_DBG("-----------------------\r\n");
        ESP_DBG("FLASH DEBUG INFO: \r\n");
        ESP_DBG("%s\r\n",buf);
        ESP_DBG("-----------------------\r\n");
    }
}

//upload debug info to esp-server
void ICACHE_FLASH_ATTR
    debug_UploadExceptionInfo(void* arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    
    ESP_DBG("reset reason: %x\n", rtc_info_dbg.reason);
    uint8 debug_upload_buf[DEBUG_UPLOAD_BUF_LEN];
    uint8* pInfo = debug_upload_buf;
    os_memset(debug_upload_buf,0,DEBUG_UPLOAD_BUF_LEN);

    uint8 devkey[41];
    os_memset(devkey,0,sizeof(devkey));
    user_esp_platform_get_devkey(devkey);
    
    if (rtc_info_dbg.reason == REASON_WDT_RST ||
        rtc_info_dbg.reason == REASON_EXCEPTION_RST ||
        rtc_info_dbg.reason == REASON_SOFT_WDT_RST) {
        os_sprintf(pInfo,UPLOAD_DEBUG_LOG,rtc_info_dbg.reason,rtc_info_dbg.exccause,rtc_info_dbg.epc1,
                                                  rtc_info_dbg.epc2,rtc_info_dbg.epc3,rtc_info_dbg.excvaddr,
                                                  rtc_info_dbg.depc,FlashDebugBufParam.DebugVersion ,devkey);
    }else{
        return;
    }

    #if ESP_MESH_SUPPORT
    mesh_json_add_elem(pInfo, sizeof(pInfo), (char*)mesh_GetMdevMac(), ESP_MESH_JSON_DEV_MAC_ELEM_LEN);
    #endif

	uint8 *dst = NULL,*src = NULL;
	#if ESP_MESH_SUPPORT
    	uint8 dst_t[6],src_t[6];
		if(pespconn && pespconn->proto.tcp){
            os_memcpy(dst_t,pespconn->proto.tcp->remote_ip,4);
			os_memcpy(dst_t+4,&pespconn->proto.tcp->remote_port,2);
		}
    	wifi_get_macaddr(STATION_IF,src_t);	
		dst = dst_t;
		src = src_t;
	#endif
	
	ESP_DBG("debug Info: %s \r\n",pInfo);
    if(0 == user_JsonDataSend(pespconn, pInfo, os_strlen(pInfo),0,src,dst)){
        debug_DropExceptionInfo();
		ESP_DBG("upload success...\r\n");
    }else{
		ESP_DBG("upload fail...\r\n");
    }
}


#include "upgrade.h"
os_timer_t reboot_t;
//OTA reboot function
void ICACHE_FLASH_ATTR
	debug_OtaReboot()
{
    #if ESP_MESH_SUPPORT
		ESP_DBG("deinit for OTA\r\n");
		espconn_mesh_disable(NULL);
		wifi_station_disconnect();
		wifi_set_opmode(NULL_MODE);
    #endif

	user_esp_clr_upgrade_finish_flag();//clear flag and reboot
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	ESP_DBG("call system upgrade reboot...\r\n");
    system_upgrade_reboot();
}

//delay , then call OTA reboot,3s*level
void ICACHE_FLASH_ATTR
	debug_OtaRebootStart(uint32 t_ms)
{
	os_timer_disarm(&reboot_t);
	os_timer_setfn(&reboot_t,debug_OtaReboot,NULL);

	
	struct ip_info sta_ip;
	wifi_get_ip_info(STATION_IF,&sta_ip);
	uint16 mesh_level = 0;
    #if ESP_MESH_SUPPORT						
		if( espconn_mesh_local_addr(&sta_ip.ip)){
			uint32 mlevel = sta_ip.ip.addr&0xff;
			mesh_level = mlevel;
			ESP_DBG("THIS IS A MESH SUB NODE..level: %d\r\n",mesh_level);
		}else if(sta_ip.ip.addr!= 0){
			ESP_DBG("THIS IS A MESH ROOT..\r\n");
			mesh_level = 1;
		}else{
            //why?
            mesh_level = 1;
		}
    #endif

	os_timer_arm(&reboot_t,t_ms+3000*(mesh_level-1),0);
	ESP_DBG("upgrade reboot in %d ms\r\n",t_ms+3000*mesh_level);
}


//=========================================================
//add DEBUG PRINT TO FLASH FOR SHK
//--------------------------------
flashDebugBuf FlashPrintBufParam;

void ICACHE_FLASH_ATTR
	debug_FlashPrintParamReset()
{
    FlashPrintBufParam.StartAddr = FLASH_PRINT_INFO_ADDR_0*0x1000;
    FlashPrintBufParam.Size = 0x2000;
    FlashPrintBufParam.InPos = FLASH_PRINT_INFO_ADDR_0*0x1000;
    FlashPrintBufParam.OutPos = FLASH_PRINT_INFO_ADDR_0*0x1000;
	FlashPrintBufParam.DebugVersion = MESH_TEST_VERSION;
	FlashPrintBufParam.CurrentSec = FLASH_PRINT_INFO_ADDR_0;
	
	spi_flash_erase_sector(FlashPrintBufParam.CurrentSec);
	spi_flash_erase_sector(FlashPrintBufParam.CurrentSec+1);
	
    debug_FlashParamSv(&FlashPrintBufParam,FLASH_PRINT_INDEX_ADDR);
}


void ICACHE_FLASH_ATTR
	debug_FlashPrintParamInit()
{
	//debug_FlashBufInit(&FlashPrintBufParam,FLASH_PRINT_INDEX_ADDR);
	
    //spi_flash_read(idx_sec*0x1000,(uint32*)flashDbgBuf,sizeof(flashDebugBuf));
	config_ParamLoadWithProtect(FLASH_PRINT_INDEX_ADDR,0,(uint8*)&FlashPrintBufParam,sizeof(flashDebugBuf));

	if(debug_FlashParamCsumCheck(&FlashPrintBufParam) && debug_FlashAddrCheck(&FlashPrintBufParam)){
        ESP_DBG("flash debug check sum ok..\r\n");
    }else{
        ESP_DBG("flash debug info reset...\r\n");
        //debug_FlashBufReset(&FlashPrintBufParam,FLASH_PRINT_INDEX_ADDR);
		debug_FlashPrintParamReset();
    }
}


void ICACHE_FLASH_ATTR
	debug_FlashPrint(uint8* pdata,uint32 len)
{
    ESP_DBG("debug_FlashPrint\r\n");
	debug_PrintToFlash(pdata, len,&FlashPrintBufParam,FLASH_PRINT_INDEX_ADDR);
}


void ICACHE_FLASH_ATTR
	mem_leak_debug_hook(void* buf,uint16_t len)
{
    ESP_DBG("mem_leak_debug_hook\r\n");

    static bool hook_called = false;
    if(hook_called == false){
        debug_DumpBufReset();
		hook_called = true;
    }

	#if 0
    extern uint8 time_stamp_str[32];
	char time_buf[128];
	os_memset(time_buf,' ',sizeof(time_buf));
    if(os_strchr(time_stamp_str,':')){
		//debug_FlashPrint((uint8*)time_stamp_str,32);
		os_sprintf(time_buf,"last time recorded: %s \r\n",time_stamp_str);
    }else{
        os_sprintf(time_buf,"time not recorded\r\n");
    }
	debug_FlashPrint((uint8*)time_buf,sizeof(time_buf));
	#endif

	
    int tmp = 4-len%4;
	int length = len;
    if(tmp != 0){
		int i;
		for(i=0;i<tmp;i++)
            *(((uint8*)buf)+len+i)= ' ';

		length = len+tmp;
    }
	ESP_DBG("len: %d ; length: %d \r\n",len,length);
	debug_FlashPrint((uint8*)buf,length);
	ESP_DBG("------------AFTER-------------\r\n");
	
    ESP_DBG("flash debug : CURSEC: %02x\r\n",FlashPrintBufParam.CurrentSec);
    ESP_DBG("flash debug : size: %d\r\n",FlashPrintBufParam.Size);
    ESP_DBG("flash debug : START: %08x\r\n",FlashPrintBufParam.StartAddr);
    ESP_DBG("flash debug : InPos: %08x\r\n",FlashPrintBufParam.InPos);
    ESP_DBG("flash debug : OutPos: %08x\r\n",FlashPrintBufParam.OutPos);
	ESP_DBG("flash debug : length : %d \r\n",debug_GetFlashDebugInfoLen(&FlashPrintBufParam));
}





//=========================================================


//print exception log to Flash
void ICACHE_FLASH_ATTR
stack_dump(struct rst_info *info, uint32 sp)
{
    uint32 i;
	uint8 log_buf[200];
    const char* pad = "    ";
	int len;
    if(info->reason == REASON_WDT_RST || info->reason == REASON_SOFT_WDT_RST
    || info->reason == REASON_EXCEPTION_RST) {

	ets_memset(log_buf,0,sizeof(log_buf));
    ets_sprintf(log_buf,"stack sp is %08x\n",sp);
    len = os_strlen(log_buf);
	if(len%4!=0) ets_memcpy(log_buf+len,pad, 4 - (len%4));
	ESP_DBG("Stack sp is %08x\n",sp);
    debug_DumpToFlash(log_buf,os_strlen(log_buf));

	ets_memset(log_buf,0,sizeof(log_buf));
    ets_sprintf(log_buf,"*** stack dump start ***\n");
    len = os_strlen(log_buf);
	if(len%4!=0) os_memcpy(log_buf+len,pad, 4 - (len%4));
    debug_DumpToFlash(log_buf,os_strlen(log_buf));
	ESP_DBG("*** Stack dump start ***\n");

	ets_memset(log_buf,0,sizeof(log_buf));
    ets_sprintf(log_buf,"PC = 0x%08x\n",info->epc1);
    len = os_strlen(log_buf);
	if(len%4!=0) os_memcpy(log_buf+len,pad, 4 - (len%4));
    debug_DumpToFlash(log_buf,os_strlen(log_buf));
	ESP_DBG("DBG PC = 0x%08x\n",info->epc1);

    for (i = sp; i < 0x40000000; i+=16) {
        uint32 *val = (uint32 *)i;
		ets_memset(log_buf,0,sizeof(log_buf));
        ets_sprintf(log_buf,"%08x: %08x %08x %08x %08x\n", i, *val, *(val + 1), *(val + 2), *(val + 3));
		len = os_strlen(log_buf);
		if(len%4!=0) os_memcpy(log_buf+len,pad, 4 - (len%4));
		debug_DumpToFlash(log_buf,os_strlen(log_buf));
		ESP_DBG("%08x: %08x %08x %08x %08x\n", i, *val, *(val + 1), *(val + 2), *(val + 3));
    }
	ets_memset(log_buf,0,sizeof(log_buf));
    ets_sprintf(log_buf,"*** stack dump end ***\n");
    len = os_strlen(log_buf);
	if(len%4!=0) os_memcpy(log_buf+len,pad, 4 - (len%4));
	debug_DumpToFlash(log_buf,os_strlen(log_buf));
	ESP_DBG("*** Stack dump end ***\n");
    }
}

//dump exception log into Flash
void ICACHE_FLASH_ATTR
system_restart_hook(struct rst_info *info)
{
    register uint32 sp asm("a1");
    uint32 offset = 0;
    #if 0
    if (info->reason == REASON_SOFT_WDT_RST) {
        offset = 0x1c0;
    }
    else if (info->reason == REASON_EXCEPTION_RST) {
        offset = 0x1b0;
    }
	#endif
	debug_DumpBufReset();	
    uint8 InfoBuf[200];
    os_memset(InfoBuf,0,200);
    uint8* ptmp = InfoBuf;
    os_sprintf(ptmp,"reset reason:%x\n", info->reason);
    ptmp+=os_strlen(ptmp);
    if (info->reason == REASON_WDT_RST ||
        info->reason == REASON_EXCEPTION_RST ||
        info->reason == REASON_SOFT_WDT_RST) {
        if (info->reason == REASON_EXCEPTION_RST) {
            os_sprintf(ptmp,"Fatal exception (%d):\n", info->exccause);
            ptmp += os_strlen(ptmp);
        }
        ptmp+=os_strlen(ptmp);
        os_sprintf(ptmp,"epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\r\n",
                info->epc1, info->epc2, info->epc3, info->excvaddr, info->depc);
        ptmp+=os_strlen(ptmp);
    }

    int len = os_strlen(InfoBuf);
    uint8 pad_len = 0;
    if(len%4 != 0){
        pad_len = 4 - (len%4);
        os_memcpy(ptmp,"   ",pad_len);
    }
    len += pad_len;
    ESP_DBG("============\r\nSV DBUG INFO:\r\n%s\r\n==============\r\n",InfoBuf);
    debug_DumpToFlash(InfoBuf,len);
    stack_dump(info, sp + offset);
}


//-------------ESPNOW DEBUG------------------
uint8 debug_target_mac[6];
bool espnow_debug_enable = false;

void ICACHE_FLASH_ATTR
debug_SetEspnowTargetMac(char* mac_addr)
{
     os_memcpy(debug_target_mac,mac_addr,6);
     return;
}

uint8* ICACHE_FLASH_ATTR
debug_GetEspnowTargetMac()
{
    return debug_target_mac;
}

bool ICACHE_FLASH_ATTR
debug_GetEspnowDebugEn()
{
    return espnow_debug_enable;
}

void ICACHE_FLASH_ATTR
debug_SetEspnowDebugEn(bool en)
{
    espnow_debug_enable = en;
}

void ICACHE_FLASH_ATTR
espnow_debug_send(uint8* data,uint16 len)
{
    if(espnow_debug_enable){
        ESP_DBG("espnow send to "MACSTR"\r\n",MAC2STR(debug_target_mac));
		ESP_DBG("data: %s \r\n",data);
		esp_now_send(debug_target_mac,data,len);
    }
}



//-----------END OF ESPNOW DEBUG--------------

#endif

