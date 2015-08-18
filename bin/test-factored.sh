#!/bin/bash

SEARCH_OPTS1="--max-time 840 --max-mem 7168 -s lazy -H ma-dtg"
SEARCH_OPTS2="--max-time 840 --max-mem 7168 -s lazy -H ma-ff"
SEARCH_OPTS1="--max-time 840 --max-mem 7168 -s lazy -H potential"

CURDIR=$(dirname $0)
SEARCH="$CURDIR/search"
VAL="$CURDIR/../third-party/VAL/validate"
DOMAIN=depot
PROBLEM=pfile1
#DOMAIN=depot
#PROBLEM=pfile9
PROTO=~/dev/plan-data/proto/codmap-2015-factor/$DOMAIN/$PROBLEM/
VAL_DOMAIN=~/dev/plan-data/bench/codmap-2015/$DOMAIN/$PROBLEM/unfactor/domain.pddl
VAL_PROBLEM=~/dev/plan-data/bench/codmap-2015/$DOMAIN/$PROBLEM/unfactor/problem.pddl
MAX_CYCLES=1000

AGENT_URLS=""
idx=0
for proto in $(ls $PROTO/*.proto); do
    port=$((18100 + $idx))
    AGENT_URLS="$AGENT_URLS --tcp 127.0.0.1:$port"
    idx=$(($idx + 1))
done

rm -rf /tmp/test-factored
mkdir /tmp/test-factored

cycle=0
while [ $cycle != $MAX_CYCLES ]; do
    D=/tmp/test-factored/$cycle
    OUT=$D/out
    ERR=$D/err
    PLAN=$D/plan

    mkdir $D
    pids=""
    idx=0
    for proto in $(ls $PROTO/*.proto); do
        agent_id="--tcp-id $idx"
        opts="-p $proto --ma-factor -o $PLAN.$idx $SEARCH_OPTS1 $AGENT_URLS $agent_id"
        #valgrind --leak-check=full --show-reachable=yes $SEARCH $opts >$OUT.$idx 2>$ERR.$idx &
        $SEARCH $opts >$OUT.$idx 2>$ERR.$idx &
        pid=$!
        pids="$pids $pid"
        idx=$(($idx + 1))
#echo $SEARCH $opts
    done
#exit

    for pid in $pids; do
        if ! wait $pid; then
            STOP=1
        fi
    done

    if [ "$STOP" = "1" ]; then
        break
    fi

    res=$(cat $PLAN.* | cut -f1 -d':' | sort -k1n | uniq -c | sed 's/^ *//' \
            | cut -f1 -d' ' | sort | uniq | wc -l)
    echo $cycle $res
    if [ "$res" != "1" ]; then
        break
    fi

    cat $PLAN.[0-9] >$PLAN.9999
    if ! $VAL $VAL_DOMAIN $VAL_PROBLEM $PLAN.9999; then
        break
    fi

    if grep Error $ERR.*; then
        grep -n Error $ERR.*
        break
    fi

    cycle=$(($cycle + 1))
done
