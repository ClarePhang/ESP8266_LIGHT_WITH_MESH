#include "user_config.h"
#include "osapi.h"
#include "c_types.h"



bool ICACHE_FLASH_ATTR
config_ParamCsumCheck(void* pData, int length)
{
	uint8 csum_cal=0;
	int i ;
	char* tmp = (char*)pData;
	
	for(i=0;i<length;i++){
		csum_cal+= *(tmp+i);
	}
	
	if(csum_cal==0xff)
		return true;
	else
		return false;
}

void ICACHE_FLASH_ATTR
	config_ParamCsumSet(void* pData,uint8* csum,int length)
{
	uint8 csum_cal=0;
	int i;
	char* tmp = (char*)pData;
	
	for(i=0;i<length;i++){
		csum_cal+= *(tmp+i);
	}
	*csum = 0xff - (csum_cal-(*csum));
}


void ICACHE_FLASH_ATTR
config_ParamLoad(uint32 addr,uint8* pParam,int length)
{
    spi_flash_read(addr,(uint32*)pParam,length);
#if 0
	char* tmp = (char*)pParam;
	int i;
	os_printf("--------after load----------\r\n");
	for(i=0;i<length;i++){
		os_printf("%02x ",*(tmp+i));
		if((i+1)%16==0) os_printf("\r\n");
	}
	os_printf("\r\n---------------------------\r\n");
#endif
}

void ICACHE_FLASH_ATTR
config_ParamLoadWithProtect(uint16 start_sec,uint16 offset,uint8* pParam,int length)
{
	system_param_load(start_sec,offset,pParam,length);
}

void ICACHE_FLASH_ATTR
	config_ParamSaveWithProtect(uint16 start_sec,uint8* pParam,int length)
{
	system_param_save_with_protect(start_sec,pParam,length);
}





