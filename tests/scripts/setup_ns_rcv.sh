#!/bin/bash
set -x

VETH1="veth1"
BW=1000
BURST=100
LIMIT=0
let "LIMIT=$BW*37,5"
LATENCY=50

tc qdisc del dev "$VETH1" root
tc qdisc add dev "$VETH1" root handle 1: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH1" parent 1: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT

VETH3="veth3"
BW=2000
BURST=100
LIMIT=0
let "LIMIT=$BW*37,5"
LATENCY=150

tc qdisc del dev "$VETH3" root
tc qdisc add dev "$VETH3" root handle 2: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH3" parent 2: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT

VETH5="veth5"
BW=3000
BURST=100
LIMIT=0
let "LIMIT=$BW*37,5"
LATENCY=300

tc qdisc del dev "$VETH5" root
tc qdisc add dev "$VETH5" root handle 3: netem delay "$LATENCY"ms 
tc qdisc add dev "$VETH5" parent 3: handle 2: tbf rate "$BW"kbit burst "$BURST"kbit limit $LIMIT


