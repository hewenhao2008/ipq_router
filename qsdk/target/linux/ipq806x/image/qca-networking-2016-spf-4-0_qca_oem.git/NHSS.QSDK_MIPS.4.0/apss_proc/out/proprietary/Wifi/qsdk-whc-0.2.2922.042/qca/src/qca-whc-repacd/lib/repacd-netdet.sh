#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.


# NOTE: At boot, even if we add 'list interface "wan"' to /etc/config/lldpd,
# lldpd doesn't listen on eth0.  We have to restart lldpd and then it starts
# listening.

NETDET_DEBUG_OUTOUT=0

NETDET_MODE_DB=/etc/ap.mode

NETDET_RESULT_ROOTAP=0
NETDET_RESULT_RE=1
NETDET_RESULT_INDETERMINATE=2

NETDET_IS_WIFISON_DEVICE_VISIBLE_TRUE=0
NETDET_IS_WIFISON_DEVICE_VISIBLE_FALSE=1

NETDET_LLDP_WIFISON_CUSTOM_TLV_OUI=33,44,55
NETDET_LLDP_WIFISON_CUSTOM_TLV_SUBTYPE=44
NETDET_LLDP_WIFISON_CUSTOM_TLV_DATA=45,45,45,45,45

# Emit a message at debug level.
# input: $1 - the message to log
__netdet_debug() {
    local stderr=''
    if [ "$NETDET_DEBUG_OUTOUT" -gt 0 ]; then
        stderr='-s'
    fi

    logger $stderr -t repacd.netdet -p user.debug "$1"
}

# Check to see if lldpd is listening on a particular port
__netdet_is_lldpd_listening() {
    local port=$1
    lldpcli show configuration | grep "Interface pattern: " | grep $port > /dev/null
}

# Are there any Wi-Fi SON neighbors reachable via a particular port?
__netdet_has_wifison_neighbors() {
    local port=$1
    lldpcli show neighbors ports $port details | grep "TLV:.*OUI: ${NETDET_LLDP_WIFISON_CUSTOM_TLV_OUI}, SubType: ${NETDET_LLDP_WIFISON_CUSTOM_TLV_SUBTYPE},.* ${NETDET_LLDP_WIFISON_CUSTOM_TLV_DATA}$" > /dev/null
}

# Does a particular port have an IPv4 address?
__netdet_has_address() {
    local port=$1
    ifconfig $port | grep "inet addr:[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}" > /dev/null
}

# Is dhcpc running on a particular port?
__netdet_is_running_dhcp_client() {
    local port=$1
    ps -w | grep dhcpc | grep $port > /dev/null
}

# Does a particular port have a DHCP-assigned IPv4 address?
__netdet_has_dhcp_address() {
    local port=$1
    __netdet_is_running_dhcp_client $port && __netdet_has_address $port
}

# Can you see WiFiSON devices on a particular port?
__netdet_is_wifison_device_visible() {
    local port=$1
    if ! __netdet_is_lldpd_listening $port; then
        __netdet_debug "warning: lldpd wasn't listening on $port so restarting"
        /etc/init.d/lldpd restart
        netdet_set_custom_wifison_lldp_tlv
    fi
    local timeout=30 # secs
    local currtime=$(date +%s)
    local timeouttime=$(($currtime + $timeout))
    while [ $currtime -le $timeouttime ]; do
        if __netdet_has_wifison_neighbors $port; then
            return $NETDET_IS_WIFISON_DEVICE_VISIBLE_TRUE
        fi
        sleep 1
        currtime=$(date +%s)
    done
    return $NETDET_IS_WIFISON_DEVICE_VISIBLE_FALSE
}

# Return a list of all the network interfaces(except loopback)
__netdet_get_interfaces() {
    ifconfig | grep "Link encap" | awk '{ print $1 }' | grep -v "^lo$"
}

# Returns the determined mode (cap, re, or unknown)
netdet_get_mode() {
    local mode="unknown"
    if [ -f $NETDET_MODE_DB ]; then
        mode="$(cat $NETDET_MODE_DB)"
    fi
    echo $mode
}

# Saves the given mode to the database
netdet_set_mode_db() {
    local mode="$1"
    echo "$mode" > $NETDET_MODE_DB
}

# Add our custom WiFi SON TLV to lldpd
netdet_set_custom_wifison_lldp_tlv() {
    # remove any existing custom TLVs
    lldpcli unconfigure lldp custom-tlv > /dev/null
    lldpcli configure lldp custom-tlv oui $NETDET_LLDP_WIFISON_CUSTOM_TLV_OUI subtype $NETDET_LLDP_WIFISON_CUSTOM_TLV_SUBTYPE oui-info $NETDET_LLDP_WIFISON_CUSTOM_TLV_DATA > /dev/null
}

# Do WAN/LAN detection
netdet_wan_lan_detect() {
    __netdet_debug "running wan/lan detection"
    local is_uplink_no_wifison="false"
    local is_uplink_with_wifison="false"
    for int in $(__netdet_get_interfaces); do
        if __netdet_has_dhcp_address $int; then
            __netdet_is_wifison_device_visible $int
            case $? in
            $NETDET_IS_WIFISON_DEVICE_VISIBLE_TRUE)
                is_uplink_with_wifison="true"
                __netdet_debug "$int: LAN (with visible Wi-Fi SON device)"
                ;;
            $NETDET_IS_WIFISON_DEVICE_VISIBLE_FALSE)
                is_uplink_no_wifison="true"
                __netdet_debug "$int: WAN"
                ;;
            *)
                __netdet_debug "got unknown return from __netdet_is_wifison_device_visible"
                ;;
            esac
        else
            __netdet_debug "$int: LAN"
        fi
    done
    if [ "$is_uplink_no_wifison" == "true" ]; then
        __netdet_debug "looks like a root AP"
        return $NETDET_RESULT_ROOTAP
    elif [ "$is_uplink_with_wifison" == "true" ]; then
        __netdet_debug "looks like a range extender"
        return $NETDET_RESULT_RE
    else
        __netdet_debug "indeterminate"
        return $NETDET_RESULT_INDETERMINATE
    fi
}

repacd_netdet_init() {
    netdet_set_custom_wifison_lldp_tlv
}
