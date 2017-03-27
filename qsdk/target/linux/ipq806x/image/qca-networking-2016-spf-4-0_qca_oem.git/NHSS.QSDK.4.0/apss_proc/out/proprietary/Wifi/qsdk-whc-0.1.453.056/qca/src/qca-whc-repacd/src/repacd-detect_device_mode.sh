#!/bin/sh
# Copyright (c) 2016 Qualcomm Atheros, Inc.
#
# All Rights Reserved.
# Qualcomm Atheros Confidential and Proprietary.

. /lib/functions/repacd-netdet.sh

netdet_wan_lan_detect
case $? in
    $NETDET_RESULT_ROOTAP)
        __netdet_debug "we have a WAN link so configure as CAP"
        netdet_set_mode_db "cap"
        ;;
    $NETDET_RESULT_RE)
        __netdet_debug "we see a WiFi SON device upstream so configure as range extender"
        netdet_set_mode_db "re"
        ;;
    $NETDET_RESULT_INDETERMINATE)
        __netdet_debug "We can't tell so don't do anything"
        netdet_set_mode_db "unknown"
        ;;
    *)
        echo "error: unknown return code: $?" >&2
        exit 4
        ;;
esac
