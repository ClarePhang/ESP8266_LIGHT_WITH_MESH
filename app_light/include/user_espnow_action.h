#ifndef __USER_ESPNOW_ACTION_H__
#define __USER_ESPNOW_ACTION_H__
#define MAX_COMMAND_COUNT 8
#include"c_types.h"
#include "user_config.h"
#define GET_CMD_LEN_TAG "CMD_LEN:"
#define USER_SET_CMD_ACTION     "/device/espnow/action_set"  //"/device/light/espnow/action_set"  
#define USET_SET_LIGHT_COLOUR  "/device/espnow/set_colour"   //"/device/light/espnow/set_colour"
#define ACTION_PARAM_MAGIC  0x5cc555cc

typedef void(*user_espnow_action)(void*arg);
static const uint8 key_tag[MAX_COMMAND_COUNT] = {'1' , '2' , '3' , '4' , 'U' , 'D' , 'L' , 'R'};
//static const uint8 key_val[MAX_COMMAND_COUNT] = {0x0e, 0x0d, 0x07, 0x0b, 0x
enum PRESS_TYPE
{
 SHORT_PRESS=0,
 LONG_PRESS,
};

typedef struct
{
    char * Tag;
    user_espnow_action user_press_func;
    //user_espnow_action user_long_press_func;
}FunctionTag;

typedef struct 
{
    uint32 R;
    uint32 G;
    uint32 B;
    uint32 Cw;
    uint32 Ww;
    uint32 Period;
}PwmParam;

typedef struct 
{
    uint32 R;
    uint32 G;
    uint32 B;
	uint32 CW;
	uint32 WW;
    sint8 on_off;
	uint8 brightness;
    uint32 timer_ms;
	uint8 broadcast_if;
}EspNowCmdParam;

typedef struct 
{
    uint16 key_value;
   // user_espnow_action user_short_press_action;
    //user_espnow_action user_long_press_action;
    int8 user_short_press_func_pos;
    int8 user_long_press_func_pos;
    //void*  short_press_arg;
    //void* long_press_arg;
	EspNowCmdParam press_arg[2];
	//EspNowCmdParam long_press_arg;
}UserEspnowAction;

typedef struct{
    uint8 csum;
	uint32 magic;
	UserEspnowAction userAction[MAX_COMMAND_COUNT];
}UserActionParam;


bool UserExecuteEspnowCmd(uint16 cmd);
int8 UserGetCorrespondFunction(const char * string);
bool UserGetCmdParam(uint8 fun_pos,const char* string,EspNowCmdParam* param);
void ResponseFuncMap();
//extern UserEspnowAction UserAction[MAX_COMMAND_COUNT];
extern FunctionTag fun_tag[MAX_COMMAND_COUNT];
extern PwmParam Pwm_Param;
extern EspNowCmdParam esp_now_cmd_param[MAX_COMMAND_COUNT][2];
extern UserActionParam userActionParam;
void UserResponseFuncMap(char* data_buf,uint16 buf_len);
#endif
