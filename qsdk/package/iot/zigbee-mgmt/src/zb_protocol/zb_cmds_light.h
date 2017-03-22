#ifndef ZB_CMDS_LIGHT_H
#define ZB_CMDS_LIGHT_H

#include "common_def.h"
#include "zb_serial_protocol.h"

int zb_onoff_light_new(struct serial_session_node *pNode, int on_off);

int zb_light_move_to_color(struct serial_session_node *pNode, zb_uint16 colorX, zb_uint16 colorY, zb_uint16  TransitionTime);

#endif

