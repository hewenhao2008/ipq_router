#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

. /usr/share/libubox/jshn.sh

LP_DIRECTION_UPSTREAM="upstream"
LP_DIRECTION_DOWNSTREAM="downstream"
LP_DIRECTION_UNKNOWN="unknown"
LP_ROLE_CAP="CAP"
LP_ROLE_RE="RE"
LP_VLAN_OFFSET=10

LP_DIRECTION_DB="/etc/ports.dir"

#TODO - these values need to be determined by board
#Port number of the port in the eth0 vlan by default
LP_ETH0_PORT=5
#Port number of the cpu port including the "t" for tagging if appropriate
LP_CPU_PORT="0t"

#TODO - these should probably be configurable from outside ths script
#Timeout for dhcp requests from CA
LP_DHCP_TO_CA=15
#Timeout for dhcp requests from RE (needs to be longer than from CA)
LP_DHCP_TO_RE=30

#Returns the isolated inteface name for a given port
lp_interfacebyport() {
    local port=$1
    if [ $port = $LP_ETH0_PORT ]; then
        local baseinterface="eth0"
    else
        local baseinterface="eth1"
    fi
    local interface="${baseinterface}.$(( $port + $LP_VLAN_OFFSET ))"
    echo $interface
}

#Returns the role of the device, either "CAP" or "RE"
lp_devicerole() {
    #A CAP should have an entry for network.wan.ifname, an RE should not
    uci get network.wan.ifname >/dev/null
    if [ $? = 0 ]; then
        echo $LP_ROLE_CAP
    else
        echo $LP_ROLE_RE
    fi
}

#Returns the determined direction (upstream, downstram, or unknown) for the
#given port
lp_getportdirection() {
    local port=$1
    local direction=$LP_DIRECTION_UNKNOWN
    if [ -f $LP_DIRECTION_DB ]; then
        json_load "$(cat $LP_DIRECTION_DB)"
        json_select "port" >/dev/null
        json_get_var direction $port
    fi

    echo $direction
}

#set the current direction (upstream, downstream, or unknown) in a
#persistent db
lp_setportdb() {
    local port=$1
    local direction=$2
    local current
    local new
    local nports=$(lp_numports)
    local dir

    if [ -f $LP_DIRECTION_DB ]; then
        current=$(cat $LP_DIRECTION_DB)
    else
        current="{}"
    fi
    json_init
    json_add_array "port"
    new=$(json_dump)
    for p in $(seq 1 $nports); do
        if [ $p = $port ]; then
            dir=$direction
        else
            json_load "$current"
            dir=$LP_DIRECTION_UNKNOWN
            json_select "port" >/dev/null
            json_get_var dir $p
        fi
        json_load "$new"
        json_select "port"
        json_add_string $p $dir
        new=$(json_dump)
    done
    json_close_array
    new=$(json_dump)
    echo $new > $LP_DIRECTION_DB
}

#Returns the current number of ports which are marked upstream
lp_numupstream() {
    local nup=0
    local dir
    local nports=$(lp_numports)

    if [ -f $LP_DIRECTION_DB ]; then
        json_load "$(cat $LP_DIRECTION_DB)"
        json_select "port" >/dev/null
        for p in $(seq 1 $nports); do
            dir=$LP_DRIECTION_UNKNOWN
            json_get_var dir $p
            if [ $dir = $LP_DIRECTION_UPSTREAM ]; then
                nup=$(( nup + 1 ))
            fi
        done
    fi
    echo $nup
}


#Returns the number of ethernet ports on switch0
lp_numports() {
    local lines=$(swconfig dev switch0 show | grep ports | wc -l)
    local words=$(swconfig dev switch0 show | grep ports | wc -w)

    echo $(( (( $words - $lines )) - $lines ))
}

#Determines whether the given interface is upstream or downstream
lp_determinedirection() {
    local port=$1
    local role=$2
    local iface=$(lp_interfacebyport $port)
    local timeout

    if [ $role = $LP_ROLE_CAP ]; then
        timeout=$LP_DHCP_TO_CA
    else
        timeout=$LP_DHCP_TO_RE
    fi

    if udhcpc -i $iface -t $timeout -T1 -n > /dev/null 2>&1; then
        echo $LP_DIRECTION_UPSTREAM
    else
        echo $LP_DIRECTION_DOWNSTREAM
    fi
}

#Isolate port in its own vlan and create virtual interface
lp_isolateport() {
    local port=$1
    local isovlan=$(( $port + $LP_VLAN_OFFSET ))
    local currvlan=$(swconfig dev switch0 port $port get pvid)
    local currports=$(swconfig dev switch0 vlan $currvlan show | grep ports | cut -d: -f2)
    local newports=""

    #remove port from its current vlan
    for p in $currports; do
        if [ $p != $port ]; then
            newports="$newports $p"
        fi
    done
    swconfig dev switch0 vlan $currvlan set ports "$LP_CPU_PORT $newports"

    #add port to isovlan
    currports=$(swconfig dev switch0 vlan $isovlan show | grep ports | cut -d: -f2)
    swconfig dev switch0 vlan $isovlan set ports "$LP_CPU_PORT $currports $port"
    swconfig dev switch0 set apply

    if [ $port = $LP_ETH0_PORT ]; then
        vconfig add eth0 $isovlan
    else
        vconfig add eth1 $isovlan
    fi
}

lp_deisolateport() {
    local port=$1
    local isovlan=$(( $port + $LP_VLAN_OFFSET ))
    local currvlan=$(swconfig dev switch0 port $port get pvid)

    if [ $isovlan = $currvlan ]; then
        #This check might not be necessary, but it prevents us from
        #manipulating a port which is not isolated

        local isoiface=$(lp_interfacebyport $port)
        local unisovlan

        #Remove port form the isolated vlan
        swconfig dev switch0 vlan $isovlan set ports ""
        #Remove virtual interface
        vconfig rem $isoiface
        #put port back in the appropriate eth0 or eth1 vlan
        if [ $port = $LP_ETH0_PORT ]; then
            unisovlan=2
        else
            unisovlan=1
        fi

        local currports=$(swconfig dev switch0 vlan $unisovlan show | grep ports | cut -d: -f2)
        swconfig dev switch0 vlan $unisovlan set ports "$currports $port"
        swconfig dev switch0 set apply
    fi
}

#Make the hotplug call to signal the direction has been determined for
#the given port
lp_signaldirectionhotplug() {
    local port=$1
    local direction=$2
    (export PORT=$port; export EVENT="direction"; export STATE=$direction; hotplug-call switch)
}

#Called by hotplug whenever a port transtions to link-up status
lp_onlinkup() {
    local role=$(lp_devicerole)
    local port=$1
    local upstrmports=$(lp_numupstream)
    local direction

    if [ $role = $LP_ROLE_CAP ] && [ $upstrmports -gt 0 ]; then
        #On the CA (at least for phase 1, if we already have an upstream
        #connection we assume all other connections are downstream
        direction=$LP_DIRECTION_DOWNSTREAM
    else
        direction=$(lp_determinedirection $port $role)
    fi
    lp_signaldirectionhotplug $port $direction
}

#Called by hotplug whenever a port transtions to link-down status
lp_onlinkdown() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_UNKNOWN
    #isolate port back into its own iface
    lp_isolateport $port
}

#Called by hotplug whenever a port direction has been determined upstream
lp_onupstream() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_UPSTREAM
    lp_deisolateport $port
}

#Called by hotplug whenever a port direction has been determined downstream
lp_ondownstream() {
    local port=$1
    lp_setportdb $port $LP_DIRECTION_DOWNSTREAM
    lp_deisolateport $port
}

repacd_lp_init() {
    local nports=$(lp_numports)
    for p in $(seq 1 $nports); do
        lp_isolateport $p
        lp_setportdb $p $LP_DIRECTION_UNKNOWN
    done
}
