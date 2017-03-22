#include "zb_cmds_light.h"
#include "zb_cmds.h"

int zb_onoff_light_new(struct serial_session_node *pNode, int on_off)
{
	struct cmd_cont *pcmd;	
	zb_uint8 offset;

	//pcmd = &gCmds_to_coord[CT_CMD_ON_OFF];
	pcmd = coord_get_global_cmd(CT_CMD_ON_OFF);
	
	pcmd->cmd.cmd_code = on_off;

	pcmd->payload_len = 0;

	pNode->seq = get_new_seq_no(0);	

	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);

	return 0;
}

#pragma pack(1)
struct move_to_color_payload
{
	zb_uint16  colorX;
	zb_uint16  colorY;
	zb_uint16  TransitionTime;
};
#pragma pack()

//colorX colorY in range [0,65279]
int zb_light_move_to_color(struct serial_session_node *pNode, zb_uint16 colorX, zb_uint16 colorY, zb_uint16  TransitionTime)
{	
	struct cmd_cont *pcmd;	
	zb_uint8 offset;
	struct move_to_color_payload *payload;

	if( (colorX > 65279) || (colorY > 65279) )
	{
		ZB_ERROR("colorX or colorY out of range.\n");
		return -1;
	}
	
	pcmd = coord_get_global_cmd(CT_LIGHT_MOVE_TO_COLOR);	
	payload = (struct move_to_color_payload *)pcmd->payload;

	payload->colorX = colorX;
	payload->colorY = colorY;
	payload->TransitionTime = TransitionTime;
	
	pcmd->payload_len = sizeof(struct move_to_color_payload);

	pNode->seq = get_new_seq_no(0);	
	
	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);

	return 0;	
}

#pragma pack(1)
struct move_to_color_temperature_payload
{
	zb_uint16  temperature;	
	zb_uint16  TransitionTime;
};
#pragma pack()

int zb_light_move_to_color_temperature(struct serial_session_node *pNode, zb_uint16 temperature, zb_uint16  TransitionTime)
{	
	struct cmd_cont *pcmd;	
	zb_uint8 offset;
	struct move_to_color_temperature_payload *payload;
	
	pcmd = coord_get_global_cmd(CT_LIGHT_MOVE_TO_COLOR_TEMPERATURE);	
	payload = (struct move_to_color_temperature_payload *)pcmd->payload;

	payload->temperature = temperature;	
	payload->TransitionTime = TransitionTime;	
	pcmd->payload_len = sizeof(struct move_to_color_temperature_payload);

	pNode->seq = get_new_seq_no(0);	
	
	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);

	return 0;	
}

#pragma pack(1)
struct move_to_level_payload
{
	zb_uint8  level;	
	zb_uint16  TransitionTime;
};
#pragma pack()

int zb_light_move_to_level(struct serial_session_node *pNode, zb_uint8 level, zb_uint16  TransitionTime)
{	
	struct cmd_cont *pcmd;	
	zb_uint8 offset;
	struct move_to_level_payload *payload;
	
	pcmd = coord_get_global_cmd(CT_LIGHT_MOVE_TO_LEVEL);	
	payload = (struct move_to_level_payload *)pcmd->payload;

	payload->level = level;	
	payload->TransitionTime = TransitionTime;	
	pcmd->payload_len = sizeof(struct move_to_level_payload);

	pNode->seq = get_new_seq_no(0);	
	
	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);

	return 0;	
}


