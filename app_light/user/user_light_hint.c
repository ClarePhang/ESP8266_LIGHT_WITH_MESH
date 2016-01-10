#include "user_light_hint.h"
#include "osapi.h"
#include "os_type.h"
#include "user_light.h"
#include "user_light_adj.h"

#if LIGHT_DEVICE
#if ESP_MESH_SUPPORT
	#include "mesh.h"
#endif

os_timer_t light_hint_t,light_recover_t;
struct light_saved_param light_param_target;
static bool light_error_flag = false;
static bool light_shade_idle = true;
struct light_saved_param light_param_error; 
	

LOCAL void ICACHE_FLASH_ATTR
light_blink(uint32 color)
{
    static bool blink_flg = true;
    if(blink_flg){
        switch(color){
            case HINT_WHITE:
                user_light_set_duty(0,LIGHT_RED);
                user_light_set_duty(0,LIGHT_GREEN);
                user_light_set_duty(0,LIGHT_BLUE);
                user_light_set_duty(5000,LIGHT_COLD_WHITE);
                user_light_set_duty(5000,LIGHT_WARM_WHITE);
                break;
            case HINT_RED:
                user_light_set_duty(10000,LIGHT_RED);
                user_light_set_duty(0,LIGHT_GREEN);
                user_light_set_duty(0,LIGHT_BLUE);
                user_light_set_duty(2000,LIGHT_COLD_WHITE);
                user_light_set_duty(2000,LIGHT_WARM_WHITE);
                break;
            case HINT_GREEN:
                user_light_set_duty(0,LIGHT_RED);
                user_light_set_duty(10000,LIGHT_GREEN);
                user_light_set_duty(0,LIGHT_BLUE);
                user_light_set_duty(2000,LIGHT_COLD_WHITE);
                user_light_set_duty(2000,LIGHT_WARM_WHITE);
                break;
            case HINT_BLUE:
                user_light_set_duty(0,LIGHT_RED);
                user_light_set_duty(0,LIGHT_GREEN);
                user_light_set_duty(10000,LIGHT_BLUE);
                user_light_set_duty(2000,LIGHT_COLD_WHITE);
                user_light_set_duty(2000,LIGHT_WARM_WHITE);
                break;
            default :
                break;
        }
        blink_flg = false;
    }else{
        user_light_set_duty(0,LIGHT_RED);
        user_light_set_duty(0,LIGHT_GREEN);
        user_light_set_duty(0,LIGHT_BLUE);
        user_light_set_duty(0,LIGHT_COLD_WHITE);
        user_light_set_duty(0,LIGHT_WARM_WHITE);
        blink_flg = true;
    }
    pwm_start();
}

void ICACHE_FLASH_ATTR
light_blinkStart(uint32 COLOR)
{
    os_timer_disarm(&light_hint_t);
    os_timer_setfn(&light_hint_t,light_blink,COLOR);
    os_timer_arm(&light_hint_t,1000,1);
}

LOCAL uint8 restore_flg = 0;  //0: not change pre-param, set back to pre-param
                              //1: restore cur-param to pre param, set back to pre-param
LOCAL uint8 shade_cnt = 0;


void ICACHE_FLASH_ATTR 
light_show_hint_color(uint32 color)
{
    switch(color){
    	case HINT_GREEN:
    		light_set_aim(0,20000,0,2000,2000,1000,0);
    		break;
    	case HINT_RED:
    		light_set_aim(20000,0,0,2000,2000,1000,0);
    		break;
    	case HINT_BLUE:
    		light_set_aim(0,0,20000,2000,2000,1000,0);
    		break;
    	case HINT_WHITE:
    		light_set_aim(0,0,0,20000,20000,1000,0);
    		break;
    	case HINT_PURE_RED:
    		light_set_aim(22222,0,0,0,0,1000,0);
    		break;
    	case HINT_PURE_GREEN:
    		light_set_aim(0,22222,0,0,0,1000,0);
    		break;
    	case HINT_PURE_BLUE:
    		light_set_aim(0,0,22222,0,0,1000,0);
    		break;
		case HINT_TARGET:
            light_set_aim(light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
            light_param_target.pwm_duty[3],light_param_target.pwm_duty[4],light_param_target.pwm_period,0);
			break;
		case HINT_YELLOW:
			light_set_aim(22222,22222,0,0,0,1000,0);
			break;
		case HINT_PURPLE:
			light_set_aim(0,22222,22222,0,0,1000,0);
			break;
    }
}

LOCAL void ICACHE_FLASH_ATTR
light_shade(int color)
{
    static bool color_flg = true;
    #if 1
    static uint32 cnt=0;
    if(shade_cnt != 0){	
        if(cnt >= (2*shade_cnt)){
            cnt = 0;
            color_flg = true;
            os_timer_disarm(&light_hint_t);
            LIGHT_INFO("RECOVER LIGHT PARAM; light_shade: \r\n");
			#if ESP_DEBUG_MODE
			if(light_error_flag){
                light_set_aim(light_param_error.pwm_duty[0],light_param_error.pwm_duty[1],light_param_error.pwm_duty[2],
                              light_param_error.pwm_duty[3],light_param_error.pwm_duty[4],light_param_error.pwm_period,0);
			}else{
                light_set_aim(light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
                              light_param_target.pwm_duty[3],light_param_target.pwm_duty[4],light_param_target.pwm_period,0);
			}
			#else
			light_set_aim(light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
						  light_param_target.pwm_duty[3],light_param_target.pwm_duty[4],light_param_target.pwm_period,0);
			#endif
            return;
        }else if(cnt==0){
            color_flg = true;
        }
        cnt++;
    }
    #endif
    
    if(color_flg){
		light_show_hint_color(color);      
        color_flg = false;
    }
    else{
        light_set_aim(0,0,0,1000,1000,1000,0);
        color_flg = true;
    }
}

void ICACHE_FLASH_ATTR
light_MeshStoreCurParam(struct light_saved_param* plight_param)
{
    int i;
    os_memset(plight_param,0,sizeof(struct light_saved_param));
    plight_param->pwm_period = user_light_get_period();
    for(i=0;i<PWM_CHANNEL;i++){
        plight_param->pwm_duty[i] = user_light_get_duty(i);
    }
    LIGHT_INFO("CURRENT LIGHT PARAM:light_MeshStoreCurParam\r\n");
    LIGHT_INFO("r: %d ; g: %d ; b: %d ;ww: %d ; cw: %d \r\n",plight_param->pwm_duty[0],
                                                            plight_param->pwm_duty[1],
                                                            plight_param->pwm_duty[2],
                                                            plight_param->pwm_duty[3],
                                                            plight_param->pwm_duty[4]);
}

void ICACHE_FLASH_ATTR
light_MeshStoreSetParam(struct light_saved_param* plight_param,uint32 period,uint32* light_duty)
{
    int i;
    os_memset(plight_param,0,sizeof(struct light_saved_param));
    plight_param->pwm_period = period;
    for(i=0;i<PWM_CHANNEL;i++){
        plight_param->pwm_duty[i] = light_duty[i];
    }
    LIGHT_INFO("CURRENT LIGHT PARAM:light_MeshStoreSetParam\r\n");
    LIGHT_INFO("r: %d ; g: %d ; b: %d ;ww: %d ; cw: %d \r\n",plight_param->pwm_duty[0],
                                                            plight_param->pwm_duty[1],
                                                            plight_param->pwm_duty[2],
                                                            plight_param->pwm_duty[3],
                                                            plight_param->pwm_duty[4]);
}

void ICACHE_FLASH_ATTR
light_shadeStart(uint32 color,uint32 t,uint32 shadeCnt,uint8 restore_if,uint32* color_param)
{
    shade_cnt = shadeCnt;
    restore_flg = restore_if;
    
	
    if(restore_flg==1){  //RESTORE THE GIVEN COLOR AFTER SHADING
        if(color_param){
            os_memcpy(light_param_target.pwm_duty,color_param,sizeof(uint32)*PWM_CHANNEL);
        }else{
            //light_MeshStoreCurParam(&light_param_target);
        }
    }else if(restore_flg ==2 || restore_flg==3 ){  //keep the shade color after finish shading
        uint32 v_r=0,v_g=0,v_b=0,v_cw=3000,v_ww=3000;
        switch(color){
            case HINT_GREEN:
                v_g = 20000;
                break;
            case HINT_RED:
                v_r = 20000;
                break;
            case HINT_BLUE:
                v_b = 20000;
                break;
            case HINT_WHITE:
                v_cw = 20000;
                v_ww = 20000;
                break;
			case HINT_PURE_RED:
				v_r = 22222;
				v_cw=0;
				v_ww=0;
		        break;
			case HINT_PURE_GREEN:
				v_g = 22222;
				v_cw=0;
				v_ww=0;
				break;
			case HINT_PURE_BLUE:
				v_b = 22222;
				v_cw=0;
				v_ww=0;
				break;
			case HINT_YELLOW:
				v_r = 22222;
				v_g = 22222;
				break;
			case HINT_PURPLE:
				v_g = 22222;
				v_b = 22222;
				break;
            default:
                break;
        }   
        uint32 dtmp[] = {v_r,v_g,v_b,v_cw,v_ww};
		if(restore_flg == 2){
            light_MeshStoreSetParam(&light_param_target,1000,dtmp);
	    }else if(restore_flg==3){
            light_MeshStoreSetParam(&light_param_error,1000,dtmp);
			light_error_flag = true;
	    }
    }
    os_timer_disarm(&light_hint_t);
    os_timer_setfn(&light_hint_t,light_shade,color);
    os_timer_arm(&light_hint_t,t,1);
}


void ICACHE_FLASH_ATTR
light_hint_stop(uint32 color)
{
    os_timer_disarm(&light_hint_t);
	light_show_hint_color(color);
}

void ICACHE_FLASH_ATTR
light_hint_abort()
{
    os_timer_disarm(&light_hint_t);
}

void ICACHE_FLASH_ATTR
	light_ColorRecover()
{

	#if ESP_DEBUG_MODE
    if(light_error_flag){
        light_error_flag = false;
		light_set_aim(light_param_error.pwm_duty[0],light_param_error.pwm_duty[1],light_param_error.pwm_duty[2],
					  light_param_error.pwm_duty[3],light_param_error.pwm_duty[4],light_param_error.pwm_period,0);
		LIGHT_INFO("RECOVER LIGHT PARAM ;light_ColorRecover: \r\n");
		LIGHT_INFO("r: %d ; g: %d ; b: %d ;ww: %d ; cw: %d \r\n",light_param_error.pwm_duty[0],light_param_error.pwm_duty[1],light_param_error.pwm_duty[2],
																light_param_error.pwm_duty[3],light_param_error.pwm_duty[4]);
		return;
    }
	#endif
	
    light_set_aim(light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
                  light_param_target.pwm_duty[3],light_param_target.pwm_duty[4],light_param_target.pwm_period,0);
    LIGHT_INFO("RECOVER LIGHT PARAM ;light_ColorRecover: \r\n");
    LIGHT_INFO("r: %d ; g: %d ; b: %d ;ww: %d ; cw: %d \r\n",light_param_target.pwm_duty[0],light_param_target.pwm_duty[1],light_param_target.pwm_duty[2],
                                                            light_param_target.pwm_duty[3],light_param_target.pwm_duty[4]);
}

void ICACHE_FLASH_ATTR
light_ShowDevLevel_t(void* mlevel_t)
{
    uint32 mlevel = (uint32)mlevel_t;
    light_hint_abort();

    
	uint32 color = HINT_WHITE;
    switch(mlevel){
        case 1:
			color = HINT_WHITE;
            break;
        case 2:
			color = HINT_RED;
            break;
        case 3:
			color = HINT_GREEN;
            break;
        case 4:
			color = HINT_BLUE;
            break;
		case 5: //level 5: PURPLE
			color = HINT_PURPLE;
			break;
		case 6: //level 6: YELLOW
			color = HINT_YELLOW;
			break;
        default:
            color = HINT_WHITE;
            break;
		
    }
	
	light_shadeStart(color,500,mlevel,1,NULL);
	#if ESP_DEBUG_MODE
        os_timer_disarm(&light_recover_t);
        os_timer_setfn(&light_recover_t,light_ColorRecover,NULL);
    	os_timer_arm(&light_recover_t,15000,0);
	#endif
}


os_timer_t show_level_delay_t;
void ICACHE_FLASH_ATTR
light_ShowDevLevel(uint32 mlevel,uint32 delay_ms)
{
    os_timer_disarm(&show_level_delay_t);
	os_timer_setfn(&show_level_delay_t,light_ShowDevLevel_t,mlevel);
	os_timer_arm(&show_level_delay_t,delay_ms,0);

}

void ICACHE_FLASH_ATTR
light_Espnow_ShowSyncSuc()
{
	light_shadeStart(HINT_WHITE,500,2,1,NULL);
}


void ICACHE_FLASH_ATTR
	light_set_color(uint32 r,uint32 g,uint32 b,uint32 cw,uint32 ww,uint32 period)
{
	os_timer_disarm(&light_hint_t);
	uint32 d_color[] = {r,g,b,cw,ww};
	light_MeshStoreSetParam(&light_param_target,period,d_color);
    light_set_aim(r,g,b,cw,ww,period,0);
}
#endif

