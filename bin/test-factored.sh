#!/bin/bash

SEARCH_OPTS1="--max-time 840 --max-mem 7168 -s lazy -H ma-dtg"
SEARCH_OPTS2="--max-time 840 --max-mem 7168 -s lazy -H ma-ff"
SEARCH_OPTS1="--max-time 840 --max-mem 7168 -s lazy -H potential"

CURDIR=$(dirname $0)
SEARCH="$CURDIR/search"
VAL="$CURDIR/../third-party/VAL/validate"
OUTDIR=/home/danfis/tmp/test-factor
DOMAIN=depot
PROBLEM=pfile1
PROTO=~/dev/plan-data/proto/codmap-2015-factor/$DOMAIN/$PROBLEM/
VAL_DOMAIN=~/dev/plan-data/bench/codmap-2015/$DOMAIN/$PROBLEM/unfactor/domain.pddl
VAL_PROBLEM=~/dev/plan-data/bench/codmap-2015/$DOMAIN/$PROBLEM/unfactor/problem.pddl
MAX_CYCLES=1

AGENT_URLS=""
idx=0
for proto in $(ls $PROTO/*.proto); do
    port=$((11100 + $idx))
    AGENT_URLS="$AGENT_URLS --tcp 127.0.0.1:$port"
    idx=$(($idx + 1))
done

rm -rf $OUTDIR
mkdir $OUTDIR

cycle=0
while [ $cycle != $MAX_CYCLES ]; do
    mkdir $OUTDIR/$cycle
    OUT=$OUTDIR/$cycle/out
    ERR=$OUTDIR/$cycle/err
    PLAN=$OUTDIR/$cycle/plan

    pids=""
    idx=0
    for proto in $(ls $PROTO/*.proto); do
        agent_id="--tcp-id $idx"
        opts="-p $proto --ma-factor -o $PLAN.$idx $SEARCH_OPTS1 $AGENT_URLS $agent_id"
        $SEARCH $opts >$OUT.$idx 2>$ERR.$idx &
        pid=$!
        pids="$pids $pid"
        idx=$(($idx + 1))
    done

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

    cycle=$(($cycle + 1))
done
