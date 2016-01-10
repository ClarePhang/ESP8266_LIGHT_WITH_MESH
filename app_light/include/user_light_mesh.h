#ifndef USER_LIGHT_MESH_H
#define USER_LIGHT_MESH_H
#include "user_config.h"
#if ESP_MESH_SUPPORT

#include "os_type.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "osapi.h"
#include "ets_sys.h"
#include "mem.h"
#include "user_config.h"
#include "user_light_adj.h"
#include "user_light.h"
#include "user_interface.h"
#include "mesh.h"
#include "esp_send.h"

typedef void (*mesh_FailCallback)(void *para);
typedef void (*mesh_SuccessCallback)(void *para);
typedef void (*mesh_InitTimeoutCallback)(void *para);
extern uint8 mesh_src_addr[ESP_MESH_ADDR_LEN];

typedef struct  {
    mesh_FailCallback mesh_fail_cb;
    mesh_SuccessCallback mesh_suc_cb;
    mesh_InitTimeoutCallback mesh_init_tout_cb;
	espconn_mesh_usr_callback mesh_node_join_cb;
    uint32 start_time;
    uint32 init_retry;
} LIGHT_MESH_PROC;

void user_MeshStart();
void user_MeshSetInfo();
void mesh_StopReconnCheck();
char* mesh_GetMdevMac();
void mesh_EnableCb(sint8 status);

void light_SendMeshUnicastCmd(uint32 r,uint32 g,uint32 b,uint32 cw,uint32 ww,uint32 period,char* dst_mac);


#endif
#endif
