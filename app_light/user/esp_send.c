#include "esp_send.h"
#include "ip_addr.h"
#include "espconn.h"
#include "ringbuf.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#if ESP_MESH_SUPPORT
#include "mesh.h"
#include "user_light_mesh.h"
#endif
#if ESP_DEBUG_MODE
#include "user_debug.h"
#endif


RINGBUF ringBuf;
#define ESP_SEND_QUEUE_LEN (1024*4)
#define ESP_SEND_BUF_LEN (1460)
uint8 espSendBuf[ESP_SEND_QUEUE_LEN];
#define SEND_RETRY_NUM 50
uint16 send_retry_cnt = 0;
os_event_t esp_SendProcTaskQueue[ESP_SEND_TASK_QUEUE_SIZE];
os_timer_t esp_send_t;



ESP_SEND_STATUS sendStatus = ESP_SERVER_ACTIVE;

void ICACHE_FLASH_ATTR
	espSendSetStatus(ESP_SEND_STATUS status)
{
	sendStatus = status;
}

ESP_SEND_STATUS ICACHE_FLASH_ATTR
	espSendGetStatus()
{
    return sendStatus;
}



/******************************************************************************
 * FunctionName : espSendEnq
 * Description  : push the sending packet to a platform tx queue.
                  post an event to system task to send TCP data.
                  keep the data until the packet is sent out
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR
    espSendEnq(uint8* data, uint16 length, struct espconn* pconn, EspPlatformMsgType MsgType ,EspSendTgt Target ,RINGBUF* r)
{
	SEND_DBG("++++++++++++\r\n%s\r\n+++++++++++++++\r\n",__func__);
	if(data==NULL){
		SEND_DBG("NULL DATA IN espSendEnq...return...\r\n");
		return -2;
	}
	
	if( RINGBUF_Check(r,length+sizeof(EspSendFrame))){
		EspSendFrame sf;
		sf.dataLen = length;
		sf.dType = MsgType;
		sf.pConn = pconn;
		sf.dTgt = Target;

		//SEND_DBG("raw data: \r\n");
		//SEND_DBG("%s\r\n",data);
		//SEND_DBG("len: %d\r\n",os_strlen(data));

		//SEND_DBG("ringbuf: %p\r\n",r->p_o);
		//SEND_DBG("ringbuf w %p \r\n",r->p_w);
		//SEND_DBG("size: %d \r\n",sizeof(EspSendFrame));
		uint8* pdata = (uint8*)&sf;
		RINGBUF_Push(r,pdata ,sizeof(EspSendFrame));

		//SEND_DBG("ringbuf: %p\r\n",r->p_o);
		//SEND_DBG("ringbuf w %p \r\n",r->p_w);
		//SEND_DBG("size: %d \r\n",length);
		pdata = data;
		RINGBUF_Push(r,pdata ,length);
		//SEND_DBG("end of espSendEnq...\r\n");
		return 0;
	}else{
        SEND_DBG("error...\r\n");
		SEND_DBG("r->size - r->fill_cnt:data_len ; %d:%d \r\n",r->size - r->fill_cnt,length+sizeof(EspSendFrame));
        uart0_tx_buffer(data,length);
		return -1;
	}
}


/******************************************************************************
 * FunctionName : espSendDataRetry
 * Description  : if the result of espconn_send is not 0, that means failed to send data out, try sending again
*******************************************************************************/
void ICACHE_FLASH_ATTR
	espSendDataRetry(void* para)
{
	if(espSendQueueIsEmpty(espSendGetRingbuf())){
		return;
	}else{
		system_os_post(ESP_SEND_TASK_PRIO, 0, (os_param_t)espSendGetRingbuf());
	}
}

void ICACHE_FLASH_ATTR
	espSendFlush(RINGBUF* r)
{
    SEND_DBG("USER SEND QUEUE FLUSH.\r\n");
    while(!espSendQueueIsEmpty(r)){
		system_soft_wdt_feed();
		espSendQueueUpdate(r);
    }

}
/******************************************************************************
 * FunctionName : espSendTask
 * Description  : handler for platform data send
*******************************************************************************/
void ICACHE_FLASH_ATTR
	espSendTask(os_event_t *e)
{
	SEND_DBG("************\r\n%s\r\n*************\r\n",__func__);
	if(espSendQueueIsEmpty(espSendGetRingbuf())){
		SEND_DBG("SENDING QUEUE EMPTY ALREADY,RETURN...\r\n");
		return;
	}
	RINGBUF* r = (RINGBUF*)(e->par);
	os_timer_disarm(&esp_send_t);
	uint8 dataSend[ESP_SEND_BUF_LEN];
	os_memset(dataSend,0,sizeof(dataSend));
	EspSendFrame sf;
	uint8* pdata = (uint8*)&sf;
    RINGBUF_PullRaw(r,pdata,sizeof(EspSendFrame),0);

	SEND_DBG("get data length: %d \r\n",sf.dataLen);
	SEND_DBG("get pconn: 0x%08x \r\n",sf.pConn);
	SEND_DBG("get data type: %d \r\n",sf.dType);
	SEND_DBG("get data target: %d \r\n",sf.dTgt);

	if(sf.dataLen>ESP_SEND_BUF_LEN){
		SEND_DBG("WARNING: DATA TOO LONG ,DROP(<=%d)...\r\n",ESP_SEND_BUF_LEN);
		return;
	}

	if(sf.pConn==NULL|| sf.pConn->proto.tcp == NULL){
		espSendQueueUpdate(espSendGetRingbuf());//	
		espSendAck(espSendGetRingbuf());
		SEND_DBG("send espconn NULL \r\n");
		return;
	}else{
		
		RINGBUF_PullRaw(r,dataSend,sf.dataLen,sizeof(EspSendFrame));
		
        #if ESP_DEBUG_MODE
	    if(NULL == (char*)os_strstr(dataSend,"download_rom_base64")){
        //if(1){
            #if ESP_MESH_SUPPORT
			    #if MESH_HEADER_MODE
			    if(espconn_mesh_get_status() == MESH_DISABLE 
					#if ESP_DEBUG_MODE
					&& DEBUG_FLG == true
					#endif
					){
                    
			    }else{
			        #if 0
                    SEND_DBG("MESH HEADER:\r\n");
    			    int k=0;
    				for(k=0;k<8;k++) {
                        SEND_DBG("%02x %02x %02x %02x \r\n",dataSend[k*4+0],dataSend[k*4+1],dataSend[k*4+2],dataSend[k*4+3]);
    				}
					#endif
    				uint8* usr_data = NULL;	
                    struct mesh_header_format* mesh_header = NULL;
                    uint16 usr_data_len = 0;
                    
                    mesh_header = (struct mesh_header_format*)dataSend;
                	if(!espconn_mesh_get_usr_data(mesh_header,&usr_data,&usr_data_len)){ 
    					SEND_DBG("-----WARNING-------\r\n");
                		SEND_DBG("debug get send data fail\n");
    					SEND_DBG("-------------------\r\n");
                		return; 
                	}else{
                	    SEND_DBG("user send queue data: length: %d \r\n",usr_data_len);
                        uart0_tx_buffer(usr_data, usr_data_len);
						SEND_DBG("------------------\r\n");
                	}
			    }
				#endif
			#endif
            SEND_DBG("------------------\r\n");
			SEND_DBG("ESPCONN: %p; STATUS: %d \r\n",sf.pConn,sf.pConn->state);
            SEND_DBG("------------------\r\n");
        }else{
            SEND_DBG("data send: %d \r\n",sf.dataLen);
        }
        #endif


    	sint8 res;

#if ESP_MESH_SUPPORT
	#if ESP_DEBUG_MODE
	    if(espconn_mesh_get_status() == MESH_DISABLE 
			#if ESP_DEBUG_MODE
			&& DEBUG_FLG == true
			#endif
			){
			res = espconn_sent(sf.pConn, dataSend, sf.dataLen);
	    }else{
			if(sf.dTgt == TO_SERVER && espSendGetStatus() == ESP_LOCAL_ACTIVE){
				SEND_DBG("LOCAL MODE, drop server data...\r\n");
                res = 0;				
			}else{
			    res = espconn_mesh_sent(sf.pConn, dataSend, sf.dataLen);
			}
	    }
	#else
	    if(sf.dTgt== TO_SERVER && espSendGetStatus() == ESP_LOCAL_ACTIVE){     	
            SEND_DBG("LOCAL MODE, drop server data...\r\n");
			res = 0;
	    }else{
            res = espconn_mesh_sent(sf.pConn, dataSend, sf.dataLen);
        	//SEND_DBG("call espconn mesh send\r\n");
        }
	#endif


#else
#ifdef CLIENT_SSL_ENABLE
		res = espconn_secure_sent(sf.pConn, dataSend, sf.dataLen);
#else
		res = espconn_sent(sf.pConn, dataSend, sf.dataLen);
#endif
#endif
    	//SEND_DBG("pconn: 0x%08x \r\n",sf.pConn);
		//SEND_DBG("pconn state: %d \r\n",sf.pConn->state);
		//SEND_DBG("pconn linkcnt: %d\r\n",sf.pConn->link_cnt);
    	//SEND_DBG("datalen: %d \r\n******************\r\n",sf.dataLen);
    	//SEND_DBG("%s \r\n",dataSend);
    	//SEND_DBG("*******************\r\n");

    	if( res==0 || res == -4){
    		if(res == 0) SEND_DBG("espconn send ok, wait for callback...\r\n");
			else if(res == -4) SEND_DBG("espconn send connection error, drop ...\r\n");
			send_retry_cnt = 0;
    		espSendQueueUpdate(espSendGetRingbuf());//	
    		espSendAck(espSendGetRingbuf());
			//if( !espSendQueueIsEmpty(espSendGetRingbuf())){
			//	system_os_post(ESP_SEND_TASK_PRIO, 0, (os_param_t)espSendGetRingbuf());
			//}
    	}else{
    		SEND_DBG("espconn send res: %d \r\n",res);
    		SEND_DBG("espconn send fail, send again...retry cnt: %d\r\n",send_retry_cnt);
			SEND_DBG("espconn state: %d ; heap: %d ; ringbuf fill_cnt: %d\r\n",sf.pConn->state,system_get_free_heap_size(),espSendGetRingbuf()->fill_cnt);
			if (send_retry_cnt>=SEND_RETRY_NUM   //timeout
				|| sf.pConn->state == ESPCONN_CLOSE //TCP connection error
				|| wifi_station_get_connect_status() != STATION_GOT_IP  //WiFi connection error
				)
			{
			
				send_retry_cnt = 0;
				SEND_DBG("ERR.DROP.\r\n");
				espSendQueueUpdate(espSendGetRingbuf());//	
				espSendAck(espSendGetRingbuf());
			}else{
			    if(sf.dType == ESP_DATA_FORCE){
					//no retry limit for upgrade
			    }else if(res == -1 && espSendGetStatus() == ESP_LOCAL_ACTIVE){    //fail to send , check status first
                    SEND_DBG("LOCAL MODE: send error, try flush");
					espconn_mesh_release_congest();
					send_retry_cnt++;
			    }
#if ESP_MESH_SUPPORT
				else if((res == -1) && (send_retry_cnt>5) && mesh_root_if()){
			        SEND_DBG("debug: release server packets for root...\r\n");
                    espconn_mesh_release_congest();
					send_retry_cnt = 0;
				}
#endif
				else{
			        send_retry_cnt++;
			    }
    		    os_timer_disarm(&esp_send_t);
				if(send_retry_cnt>50){
                    os_timer_arm(&esp_send_t,1000,0);
				}else{
    		        os_timer_arm(&esp_send_t,500,0);
				}
				
			}
    	}
	}
	
}

/******************************************************************************
 * FunctionName : espSendGetRingbuf
 * Description  : return the pointer of the RINGBUF
*******************************************************************************/
RINGBUF* ICACHE_FLASH_ATTR espSendGetRingbuf()
{
	return &ringBuf;
}


/******************************************************************************
 * FunctionName : espSendQueueInit
 * Description  : Init for the RINGBUFFER and set a system task to process platform send task.
*******************************************************************************/
void ICACHE_FLASH_ATTR
	espSendQueueInit()
{
	os_memset(espSendBuf,0,sizeof(espSendBuf));
	SEND_DBG("sizeof(espSendBuf): %d\r\n",sizeof(espSendBuf));
	RINGBUF_Init(espSendGetRingbuf(),espSendBuf,sizeof(espSendBuf) );
	system_os_task(espSendTask, ESP_SEND_TASK_PRIO, esp_SendProcTaskQueue, ESP_SEND_TASK_QUEUE_SIZE);
	os_timer_disarm(&esp_send_t);
	os_timer_setfn(&esp_send_t,espSendDataRetry,espSendGetRingbuf());
}

/******************************************************************************
 * FunctionName : espSendQueueIsEmpty
 * Description  : check whether the BINGBUF is empty
*******************************************************************************/
bool ICACHE_FLASH_ATTR
	espSendQueueIsEmpty(RINGBUF* r)
{
	if(r->p_r == r->p_w) return true;
	else return false;
}

/******************************************************************************
 * FunctionName : espSendQueueUpdate
 * Description  : Free the 1st packet in tx queue
*******************************************************************************/
sint8 ICACHE_FLASH_ATTR
	espSendQueueUpdate(RINGBUF* r)
{
	EspSendFrame sf;
	
	RINGBUF_Pull(r,(uint8*)&sf,sizeof(EspSendFrame));
	SEND_DBG("get data length: %d \r\n",sf.dataLen);
	SEND_DBG("get pconn: 0x%08x \r\n",sf.pConn);
	SEND_DBG("get data type: %d \r\n",sf.dType);
	SEND_DBG("esp send queue drop : %d bytes \r\n",sf.dataLen);
	RINGBUF_Drop(r,sf.dataLen);
	SEND_DBG("esp send buf remain: %d ; %d ; %d \r\n",r->fill_cnt,(r->size)-(r->fill_cnt),r->size);
}

/******************************************************************************
 * FunctionName : espSendAck
 * Description  : In mesh network, there are some protocol to check whether it is capable to send the packets out.
                  After we have send the first packet in the sending queue, post an event to send other packets.
*******************************************************************************/
void ICACHE_FLASH_ATTR
	espSendAck(RINGBUF* r)
{
	SEND_DBG("test send finish ack\r\n");	
	if(espSendQueueIsEmpty(r))
		return;
	else 
		os_timer_arm(&esp_send_t,500,0);
		//system_os_post(ESP_SEND_TASK_PRIO, 0, (os_param_t)r);
}


#if 0
//------------------------------------------------------------------------------
//user_mesh_send
//#include "jsontree.h"
#include "user_json.h"

typedef struct{
    char sip_str[9];
    char sport_str[5];
}MeshInfo;

MeshInfo meshParseInfo;


LOCAL int ICACHE_FLASH_ATTR
mesh_parse_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
    int type;
    while ((type = jsonparse_next(parser)) != 0) {
        if (type == JSON_TYPE_PAIR_NAME) {
            if (jsonparse_strcmp_value(parser, "sip") == 0) {
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, meshParseInfo.sip_str, sizeof(meshParseInfo.sip_str));
				os_printf("sip: %s\r\n",meshParseInfo.sip_str);
            }else if(jsonparse_strcmp_value(parser, "sport") == 0){
                jsonparse_next(parser);
                jsonparse_next(parser);
                jsonparse_copy_value(parser, meshParseInfo.sport_str, sizeof(meshParseInfo.sport_str));
				os_printf("sport: %s\r\n",meshParseInfo.sport_str);
            }
        }
    }
    return 0;
}


LOCAL struct jsontree_callback mesh_parse_callback =
    JSONTREE_CALLBACK(NULL, mesh_parse_set);



JSONTREE_OBJECT(mesh_parse_tree,
                JSONTREE_PAIR("sip", &mesh_parse_callback),
                JSONTREE_PAIR("sport", &mesh_parse_callback),
                 //JSONTREE_PAIR("mdev_mac", &mesh_parse_callback),
                 //JSONTREE_PAIR("glen", &mesh_parse_callback),
                 //JSONTREE_PAIR("group", &mesh_parse_callback)
                );
JSONTREE_OBJECT(mesh_json_info,
               JSONTREE_PAIR("mesh_json_info", &mesh_parse_tree));


void ICACHE_FLASH_ATTR user_mesh_send(struct espconn* pConn,char* data, uint16 data_len,uint8 max_buf_len)
{
    uint8 data_buf[1300];
	uint8 tag_buf[20];
	os_bzero(data_buf,sizeof(data_buf));
	os_memcpy(data_buf,data,data_len);
	//if (!mesh_json_add_elem(pbuf, sizeof(pbuf), sip, ESP_MESH_JSON_IP_ELEM_LEN)) return;
	//if (!mesh_json_add_elem(pbuf, sizeof(pbuf), sport, ESP_MESH_JSON_PORT_ELEM_LEN)) return;
	//if (!mesh_json_add_elem(pbuf, sizeof(pbuf), dev_mac, ESP_MESH_JSON_DEV_MAC_ELEM_LEN)) return;
     
	if(os_strlen(meshParseInfo.sip_str)){
	    os_bzero(tag_buf,sizeof(tag_buf));
		os_sprintf(tag_buf,ESP_MESH_SIP_STRING": \"%s\"",meshParseInfo.sip_str);
		if (!mesh_json_add_elem(data_buf, sizeof(data_buf), tag_buf, os_strlen(tag_buf))) return;
	}
	if(os_strlen(meshParseInfo.sport_str)){
	    os_bzero(tag_buf,sizeof(tag_buf));
		os_sprintf(tag_buf,ESP_MESH_SPORT_STRING": \"%s\"",meshParseInfo.sport_str);
		if (!mesh_json_add_elem(data_buf, sizeof(data_buf), tag_buf, os_strlen(tag_buf))) return;
	}
	char* pMmac = (char*)mesh_GetMdevMac();
	mesh_json_add_elem(data_buf, sizeof(data_buf), pMmac, os_strlen(pMmac));


}

#endif







