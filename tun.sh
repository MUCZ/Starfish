#!/bin/bash

# funcs
show_usage() {
    echo "Usage $0 <start | stop > [tunnum ...]"
    exit 1
}

check_sudo(){
    # if the user didn't call this script with sudo, re-execute
    if [ -z "$SUDO_USER" ]; then 
        exec sudo $0 "$MODE" "$@"
    fi
}

start_tun(){
    local TUNNUM="$1" TUNDEV="starfish_tun"
    ip tuntap add mode tun user "${SUDO_USER}" name "${TUNDEV}"   # create tun interface for this user
    ip addr add "${TUN_IP_PREFIX}.${TUNNUM}.1/24" dev "${TUNDEV}" # assign ip to tun device 
    ip link set dev "${TUNDEV}" up                                # enable created tun device
    ip route change "${TUN_IP_PREFIX}.${TUNNUM}.0/24" dev "${TUNDEV}" rto_min 10ms # use the minimum TCP-Retransmission-Timeout

    # Apply NAT (masquerading) only to traffic from CS144's network devices
    iptables -t nat -A PREROUTING -s ${TUN_IP_PREFIX}.${TUNNUM}.0/24 -j CONNMARK --set-mark ${TUNNUM} # add mark on our tcp packages
    iptables -t nat -A POSTROUTING -j MASQUERADE -m connmark --mark ${TUNNUM}                         # apply NAT on packages marked above
    echo 1 > /proc/sys/net/ipv4/ip_forward                                                            # allow ip forwarding
}

stop_tun () {
    local TUNDEV="starfish_tun"
    iptables -t nat -D PREROUTING -s ${TUN_IP_PREFIX}.${1}.0/24 -j CONNMARK --set-mark ${1}
    iptables -t nat -D POSTROUTING -j MASQUERADE -m connmark --mark ${1}
    ip tuntap del mode tun name "$TUNDEV"
}

start_all(){
    while [ ! -z "$1" ]; do 
        local INTF="$1" # tun interface 
        shift 
        start_tun "$INTF"
    done
}

stop_all(){
    while [ ! -z "$1" ]; do 
        local INTF="$1"
        shift 
        stop_tun "$INTF"
    done
}

# check args
if [ -z "$1" ] || ([ "$1" != "start" ] && [ "$1" != "stop" ]); then 
    show_usage
fi
MODE=$1
shift

#set default ip_address if not given
if [ "$#" = "0" ]; then 
    set -- 144
fi

# sudo if necessary
check_sudo "$@"

# get tun ip config
. "$(dirname "$0")"/etc/tunconfig

# call start / stop funcs 
eval "${MODE}_all" "$@"