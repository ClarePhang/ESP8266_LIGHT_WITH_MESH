#ifndef _USER_LIGHT_HINT_H
#define _USER_LIGHT_HINT_H
#include "c_types.h"
extern struct light_saved_param light_param_target;
typedef enum{
	HINT_WHITE,
	HINT_RED,
	HINT_GREEN,
	HINT_BLUE,
	HINT_PURE_RED,
	HINT_PURE_GREEN,
	HINT_PURE_BLUE,
	HINT_TARGET,
	HINT_YELLOW,
	HINT_PURPLE,
}HINT_COLOR;

void light_set_color(uint32 r,uint32 g,uint32 b,uint32 cw,uint32 ww,uint32 period);

#endif
