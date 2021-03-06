#!/bin/bash
set -x
#BW/8*1000 = RATE_IN_BYTES-> RATE/10 * 3 -> 30% of the bytes. => 1000/8 * 3/10 = 125 * 3 / 10 = 37,5
VETH0="veth0"
BW=1000
LIMIT=0
let "LIMIT=$BW*37,5"
LATENCY=50
BURST=100

tc qdisc del dev "$VETH0" root
tc qdisc add dev "$VETH0" root handle 1: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH0" parent 1: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT

VETH2="veth2"
BW=2000
let "LIMIT=$BW*37,5"
LATENCY=150
BURST=100

tc qdisc del dev "$VETH2" root
tc qdisc add dev "$VETH2" root handle 2: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH2" parent 2: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT

VETH4="veth4"
BW=3000
let "LIMIT=$BW*37,5"
LATENCY=300
BURST=100

tc qdisc del dev "$VETH4" root
tc qdisc add dev "$VETH4" root handle 3: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH4" parent 3: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT


