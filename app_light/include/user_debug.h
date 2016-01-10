#ifndef _USER_DEBUG_H
#define _USER_DEBUG_H
#include "c_types.h"

typedef struct{
    uint8     csum;
    uint8     pad;
    uint16     Size;
    uint32     StartAddr;
    uint32     InPos;
    uint32     OutPos;
    uint32     DebugVersion;
	uint32     CurrentSec;
}flashDebugBuf;

enum espnow_dbg_data_type{
    M_FREQ_CAL = 0, //int16
	WIFI_STATUS,  //uint8
	FREE_HEAP_SIZE, // uint16
	CHILD_NUM,      //uint8
	SUB_DEV_NUM,    //uint16
	MESH_STATUS,    //int8
	MESH_VERSION,   //string
	MESH_ROUTER,    //STRUCT STATION_CONF
	MESH_LAYER,     //uint8
	MESH_ASSOC,     //UINT8
	MESH_CHANNEL,   //UINT8
};
extern void espnow_get_debug_data(uint8_t type,uint8_t *buf);


extern flashDebugBuf FlashDebugBufParam;
extern uint8 DEBUG_FLG;
#endif
