#!/bin/sh

#change ip format into int number from string
ipToInt()
{
	if [ $# -ne 1 ]; then
		return
	fi

	expr $1 : '^[0-9]\{1\}[0-9]\{0,2\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$' >/dev/null 2>&1 || return
	
	local tmpIFS=$IFS
	IFS=.
	set $1
	if [ $1 -gt 255 ] || [ $1 -lt 0 ] || [ $2 -gt 255 ] || [ $2 -lt 0 ] || [ $3 -gt 255 ] || [ $3 -lt 0 ] || [ $4 -gt 255 ] || [ $4 -lt 0 ]; then
		return
	fi
	IFS=$tmpIFS

	local num=$(($1*256*256*256+$2*256*256+$3*256+$4))
	
	if [ "$num" -eq "0" ]; then
		return
	fi

	echo $num
}

TIMEOUT=2.0

ERROR_INTERNEL_ERROR=-2
ERROR_INPUT_PARA_ERROR=-1
WAN_IFACE_NOT_LINK=0
WAN_TYPE_DHCP=1
WAN_TYPE_STATIC=2
WAN_TYPE_PPPOE=3

PROTO_DHCP="dhcp"
PROTO_STATIC="static"
PROTO_PPPOE="none"

. /usr/share/libubox/jshn.sh
. /usr/lib/libmsglog.sh
. /lib/netifd/common_api.sh

BINPATH=/usr/sbin/link_status_detect_core

WANPROTO=$(uci get network.wan.proto)
json_init
if [ "$WANPROTO" == "none" ];then
	WANIFACE=internet
else
	WANIFACE=wan
fi

#WANSTATUS=$(ubus -S call network.interface.$WANIFACE status)

#json_load "$WANSTATUS"
#json_get_var wanDev device
wanDev=$(uci get network.wan.ifname)

DEVSTATUS=$(ubus -S call network.device status)
json_load "$DEVSTATUS"
json_select "$wanDev"

json_get_var up up

#PPPoE手动连接模式下，点击断开后，up字段会变为false，且没有link和macaddr字段
SET_UP_FLAG=0
if [ "$up" != "1" ]; then
	SET_UP_FLAG=1
	ubus call network.interface.$WANIFACE up

	LOOPTIME=0
	until [ "$up" == "1" -o $LOOPTIME -ge 3 ]; do
		sleep 1s
		DEVSTATUS=$(ubus call network.device status)
		json_load "$DEVSTATUS"
		json_select "$wanDev"

		json_get_var up up
		LOOPTIME=$(expr $LOOPTIME + 1)
	done
fi

json_get_var link link
json_get_var macaddr macaddr

#网线未连接，返回WAN_IFACE_NOT_LINK
if [ "$link" != "1" -o "$up" != "1" ]; then
	if [ "$SET_UP_FLAG" != "0" ]; then
		ubus call network.interface.$WANIFACE down
	fi

	msglog $LOG_PRIO_NOTICE "OTHER" "WanTypeDetect:wan interface is not link"
	echo "$WAN_IFACE_NOT_LINK"
	return "$WAN_IFACE_NOT_LINK"
fi

#获取原配置参数的 wanIP, priDns, sndDns。开始dns探测 
local wanIP=   
local gPriDns=
local gSndDns=
local priDns=
local sndDns=                 
             
get_wan_ip_addr wanIP			
if [ "$wanIP" == "0.0.0.0" ]; then
	echo "$WAN_TYPE_PPPOE"
	return "$WAN_TYPE_PPPOE"	
fi
 
get_wan_dns gPriDns gSndDns
if [ -z "$gPriDns" ]; then
	echo "$WAN_TYPE_PPPOE"
	return "$WAN_TYPE_PPPOE"	
else
	priDns=$(ipToInt $gPriDns) 	
fi

[ -n "$gSndDns" ] && sndDns=$(ipToInt $gSndDns) 

LINK_STATUS_INVALID=0
LINK_STATUS_VALID=1      

$BINPATH $TIMEOUT $priDns $sndDns
RETVAR=$?

# 映射探测结果到具体的连接方式
if [ "$RETVAR" == "$LINK_STATUS_VALID" ]; then
	case $WANPROTO in
		$PROTO_DHCP)
			RETVAR=$WAN_TYPE_DHCP
		;;
		$PROTO_STATIC)
			RETVAR=$WAN_TYPE_STATIC
		;;
		$PROTO_PPPOE)		
			RETVAR=$WAN_TYPE_PPPOE
		;;
		*)
			RETVAR=$WAN_TYPE_PPPOE
		;;
	esac
elif [ "$RETVAR" == "$LINK_STATUS_INVALID" ]; then
	RETVAR=$WAN_TYPE_PPPOE
fi

if [ "$SET_UP_FLAG" != "0" ]; then
	ubus call network.interface.$WANIFACE down
fi

case $RETVAR in
	$ERROR_INTERNEL_ERROR)
		msglog $LOG_PRIO_ERROR "OTHER" "WanTypeDetect:internal error"
	;;
	$ERROR_INPUT_PARA_ERROR)
		msglog $LOG_PRIO_ERROR "OTHER" "WanTypeDetect:input para error"
	;;
	$WAN_IFACE_NOT_LINK)
		msglog $LOG_PRIO_NOTICE "OTHER" "WanTypeDetect:wan interface is not link"
	;;	
	$WAN_TYPE_DHCP)
		msglog $LOG_PRIO_NOTICE "OTHER" "WanTypeDetect:wan type is DHCP"
	;;
	$WAN_TYPE_STATIC)
		msglog $LOG_PRIO_NOTICE "OTHER" "WanTypeDetect:wan type is Static IP"
	;;
	$WAN_TYPE_PPPOE)
		msglog $LOG_PRIO_NOTICE "OTHER" "WanTypeDetect:wan type is PPPoE"
	;;
esac

echo "$RETVAR"
return "$RETVAR"