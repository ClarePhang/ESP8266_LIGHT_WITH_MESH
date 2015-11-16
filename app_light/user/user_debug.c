#include "user_debug.h"
#include "ringbuf.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "osapi.h"

#if ESP_MESH_SUPPORT
#include "mesh.h"
#endif

typedef struct{
    uint8     csum;
    uint8     pad;
    uint16     Size;
    uint32     StartAddr;
    uint32     InPos;
    uint32     OutPos;
    uint32     DebugVersion;
    //RINGBUF*     debugBuf;
    //uint16     Space;  //remanent space of the buffer
}flashDebugBuf;

flashDebugBuf FlashDebugBufParam;

struct rst_info rtc_info_dbg;
#define DEBUG_UPLOAD_BUF_LEN 500

#define UPLOAD_DEBUG_LOG  "{\"path\": \"/v1/device/debugs/\", \"method\": \"POST\", \
\"body\": {\"debugs\": [{\"type\": 1,\"errno\": 100,\"message\":\"[%d,%d,0x%08x,0x%08x,0x%08x,%d,%d,%d]\"}]}, \"meta\": {\"Authorization\": \"token %s\"}}\n"

bool ICACHE_FLASH_ATTR
debug_FlashParamCsumCheck()
{
    uint8 csum_cal=0;
    int i ;
    char* tmp = (char*)(&FlashDebugBufParam);
    for(i=1;i<sizeof(FlashDebugBufParam);i++){
        csum_cal+= *(tmp+i);

    }
    os_printf("flash csum cal: %d ; ori: %d \r\n",csum_cal,FlashDebugBufParam.csum);
    if(csum_cal==FlashDebugBufParam.csum)
        return true;
    else
        return false;
}

void ICACHE_FLASH_ATTR
    debug_FlashParamCsumSet()
{
    uint8 csum_cal=0;
    int i;
    char* tmp = (char*)(&FlashDebugBufParam);
    for(i=1;i<sizeof(FlashDebugBufParam);i++){
        csum_cal+= *(tmp+i);
    }
    FlashDebugBufParam.csum = csum_cal;

}

void ICACHE_FLASH_ATTR
    debug_FlashParamSv()
{
    debug_FlashParamCsumSet();
    spi_flash_erase_sector(Flash_DEBUG_INFO_ADDR);
    spi_flash_write(Flash_DEBUG_INFO_ADDR*0x1000,(uint32*)&FlashDebugBufParam,sizeof(FlashDebugBufParam));  
}

bool ICACHE_FLASH_ATTR
    debug_FlashAddrCheck()
{ 
    if(FlashDebugBufParam.InPos >= FlashDebugBufParam.OutPos  && 
        FlashDebugBufParam.InPos< FlashDebugBufParam.StartAddr+FlashDebugBufParam.Size &&
        FlashDebugBufParam.InPos>=FlashDebugBufParam.StartAddr){
        return true;
    }else{
        return false;
    }
}

void ICACHE_FLASH_ATTR
debug_FlashBufReset()
{
    FlashDebugBufParam.StartAddr = FLASH_DEBUG_SV_ADDR*0x1000;
    FlashDebugBufParam.Size = FLASH_DEBUG_SV_SIZE;
    FlashDebugBufParam.InPos = FLASH_DEBUG_SV_ADDR*0x1000;
    FlashDebugBufParam.OutPos = FLASH_DEBUG_SV_ADDR*0x1000;
    debug_FlashParamSv();
}

void ICACHE_FLASH_ATTR
debug_FlashBufInit()
{
    spi_flash_read(Flash_DEBUG_INFO_ADDR*0x1000,(uint32*)&FlashDebugBufParam,sizeof(FlashDebugBufParam));
    if(debug_FlashParamCsumCheck() && debug_FlashAddrCheck()){
        
    }else{
        debug_FlashBufReset();
    }
}

int ICACHE_FLASH_ATTR
debug_GetDebugVersion()
{
    return FlashDebugBufParam.DebugVersion;
}

void ICACHE_FLASH_ATTR
debug_SetDebugVersion(int ver)
{
    if(debug_FlashParamCsumCheck() && debug_FlashAddrCheck()){
        //FlashDebugBufParam.DebugVersion = ver;
    }else{
        debug_FlashBufInit();
    }
    FlashDebugBufParam.DebugVersion = ver;
    debug_FlashParamSv();
}

void ICACHE_FLASH_ATTR
    debug_SvExceptionInfo(struct rst_info* pInfo)
{
    os_memset(&rtc_info_dbg,0,sizeof(struct rst_info));
    os_memcpy(&rtc_info_dbg,pInfo,sizeof(struct rst_info));
}

void ICACHE_FLASH_ATTR
    debug_DropExceptionInfo()
{
    os_memset(&rtc_info_dbg,0,sizeof(struct rst_info));
}

void ICACHE_FLASH_ATTR
debug_PrintToFlash(uint8* data, uint16 len)
{
    if(len%4 != 0){
        os_printf("4Bytes ...\r\n");
        return;
    }
    
    if(debug_FlashAddrCheck()){
        if(FlashDebugBufParam.InPos % 0x1000 ==0)
            spi_flash_erase_sector(FlashDebugBufParam.InPos / 0x1000);
    }else{
        debug_FlashBufInit();
    }
    if(FlashDebugBufParam.InPos+len <  FlashDebugBufParam.StartAddr+FlashDebugBufParam.Size ){
        spi_flash_write(FlashDebugBufParam.InPos,(uint32 *)data,len);
        FlashDebugBufParam.InPos+=len;
        debug_FlashParamSv();
    }
}

void ICACHE_FLASH_ATTR
debug_FlashSvExceptInfo(struct rst_info* pInfo)
{
    debug_FlashBufInit();
    uint8 InfoBuf[200];
    os_memset(InfoBuf,0,200);
    uint8* ptmp = InfoBuf;
    
    os_sprintf(ptmp,"reset reason:%x\n", pInfo->reason);
    ptmp+=os_strlen(ptmp);
    
    if (pInfo->reason == REASON_WDT_RST ||
        pInfo->reason == REASON_EXCEPTION_RST ||
        pInfo->reason == REASON_SOFT_WDT_RST) {
        if (pInfo->reason == REASON_EXCEPTION_RST) {
            os_sprintf(ptmp,"Fatal exception (%d):\n", pInfo->exccause);
            ptmp += os_strlen(ptmp);
        }
        os_sprintf(ptmp,"debug_version:%d\r\n",FlashDebugBufParam.DebugVersion);
        ptmp+=os_strlen(ptmp);
        os_sprintf(ptmp,"epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x\r\n",
                pInfo->epc1, pInfo->epc2, pInfo->epc3, pInfo->excvaddr, pInfo->depc);
        ptmp+=os_strlen(ptmp);
    }

    int len = os_strlen(InfoBuf);
    uint8 pad_len = 0;
    if(len%4 != 0){
        pad_len = 4 - (len%4);
        os_memcpy(ptmp,"   ",pad_len);
    }
    len += pad_len;
        
    debug_PrintToFlash(InfoBuf,len);
}


//not used
void ICACHE_FLASH_ATTR
debug_FlashUpdateExpetInfo(uint16 len)
{
    if(debug_FlashAddrCheck() && (FlashDebugBufParam.OutPos+len <= FlashDebugBufParam.InPos)){
        FlashDebugBufParam.InPos-=len;
        debug_FlashParamSv();
    }else{
        //return false;
    }
}

void ICACHE_FLASH_ATTR
    debug_DispFlashExceptInfo()
{
    os_printf("flash debug : version: %d \r\n",FlashDebugBufParam.DebugVersion);
    os_printf("flash debug : InPos: %08x\r\n",FlashDebugBufParam.InPos);
    os_printf("flash debug : OutPos: %08x\r\n",FlashDebugBufParam.OutPos);
    uint8* debug_str = NULL;
    
    int size = (FlashDebugBufParam.InPos-FlashDebugBufParam.OutPos);
        
    if(size>0){
        debug_str = (uint8*)os_zalloc(size+1);
        spi_flash_read(FlashDebugBufParam.OutPos,(uint32*)debug_str,size);
        os_printf("-----------------------\r\n");
        os_printf("FLASH DEBUG INFO: \r\n");
        os_printf("%s\r\n",debug_str);
        os_printf("-----------------------\r\n");
    }
	if(debug_str){
        os_free(debug_str);
		debug_str = NULL;
	}
	
}

int ICACHE_FLASH_ATTR
    debug_GetFlashDebugInfoLen()
{
    return (FlashDebugBufParam.InPos-FlashDebugBufParam.OutPos);

}

void ICACHE_FLASH_ATTR
    debug_GetFlashExceptInfo(uint8* buf, uint16 max_len,uint16 offset)
{
    os_printf("flash debug : InPos: %08x\r\n",FlashDebugBufParam.InPos);
    os_printf("flash debug : OutPos: %08x\r\n",FlashDebugBufParam.OutPos);    
    int size = (FlashDebugBufParam.InPos-FlashDebugBufParam.OutPos-offset);
    if(size>max_len) size = max_len;

    offset = offset - offset%4;
    if(size>0){
        //debug_str = (uint8*)os_zalloc(size+1);
        spi_flash_read(FlashDebugBufParam.OutPos+offset,(uint32*)buf,size);
        os_printf("-----------------------\r\n");
        os_printf("FLASH DEBUG INFO: \r\n");
        os_printf("%s\r\n",buf);
        os_printf("-----------------------\r\n");
    }
}

void ICACHE_FLASH_ATTR
    debug_UploadExceptionInfo(void* arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    
    os_printf("reset reason: %x\n", rtc_info_dbg.reason);
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
	os_printf("debug Info: %s \r\n",pInfo);
    if(0 == espconn_esp_sent(pespconn, pInfo, os_strlen(pInfo))){
        debug_DropExceptionInfo();
		os_printf("upload success...\r\n");
    }else{
		os_printf("upload fail...\r\n");
    }
}


#include "upgrade.h"
os_timer_t reboot_t;

void ICACHE_FLASH_ATTR
	debug_OtaReboot()
{
    #if ESP_MESH_SUPPORT
		os_printf("deinit for OTA\r\n");
		espconn_mesh_disable(NULL);
		wifi_station_disconnect();
		wifi_set_opmode(NULL_MODE);
    #endif

	user_esp_clr_upgrade_finish_flag();//clear flag and reboot
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	os_printf("call system upgrade reboot...\r\n");
    system_upgrade_reboot();
}


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
			os_printf("THIS IS A MESH SUB NODE..level: %d\r\n",mesh_level);
		}else if(sta_ip.ip.addr!= 0){
			os_printf("THIS IS A MESH ROOT..\r\n");
			mesh_level = 1;
		}else{
            //why?
            mesh_level = 1;
		}
    #endif

	os_timer_arm(&reboot_t,t_ms+3000*(mesh_level-1),0);
	os_printf("upgrade reboot in %d ms\r\n",t_ms+3000*mesh_level);
}

