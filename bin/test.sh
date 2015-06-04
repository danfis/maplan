#!/bin/bash

PLAN_DATA_DIR=/home/danfis/dev/plan-data
SEARCH_BIN=$(dirname "$0")/search
TMP_STDERR=/tmp/maplan-test-search.err
TMP_STDOUT=/tmp/maplan-test-search.out
ERROR=no

RUN_SOLUTION_FOUND=
RUN_PLAN_COST=
function run() {
    input=$1
    shift
    echo "$SEARCH_BIN" -p "$input" "$@"
    "$SEARCH_BIN" -p "$input" "$@" >"$TMP_STDOUT" 2>"$TMP_STDERR"

    RUN_SOLUTION_FOUND=no
    if grep 'Solution found.' "$TMP_STDOUT" >/dev/null 2>&1; then
        RUN_SOLUTION_FOUND=yes
    fi

    RUN_PLAN_COST=$(grep 'Plan Cost:' "$TMP_STDOUT" | cut -f3 -d' ')
    rm -f "$TMP_STDOUT", "$TMP_STDERR"
}

function test-optimal(){
    input=${PLAN_DATA_DIR}/$1
    shift
    optimal_cost=$1
    shift

    run "$input" "$@"

    echo "    Solution found: $RUN_SOLUTION_FOUND"
    if [ "$RUN_SOLUTION_FOUND" != "yes" ]; then
        echo "    ERROR: Solution not found!"
        ERROR=yes
    fi

    echo "    Plan cost: $RUN_PLAN_COST"
    if [ "$optimal_cost" != "$RUN_PLAN_COST" ]; then
        echo "    ERROR: Invalid solution cost (should be $optimal_cost)!"
        ERROR=yes
    fi
    echo
}

function test-satisficing(){
    input=${PLAN_DATA_DIR}/$1
    shift

    run "$input" "$@"

    echo "    Solution found: $RUN_SOLUTION_FOUND"
    if [ "$RUN_SOLUTION_FOUND" != "yes" ]; then
        echo "    ERROR: Solution not found!"
        ERROR=yes
    fi
    echo
}

if [ ! -x "$SEARCH_BIN" ]; then
    echo "Error: search binary not available."
    exit -1
fi

test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut
test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H lm-cut
test-optimal benchmarks/rovers/p01.proto 10 -s astar -H lm-cut
test-optimal benchmarks/rovers/p02.proto 8 -s astar -H lm-cut
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut
test-optimal benchmarks/rovers/p04.proto 8 -s astar -H lm-cut
test-optimal benchmarks/satellites/p01-pfile1.proto 9 -s astar -H lm-cut
test-optimal benchmarks/satellites/p02-pfile2.proto 13 -s astar -H lm-cut
test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H lm-cut
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut

test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut --ma-unfactor
test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H ma-lm-cut --ma-unfactor
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut --ma-unfactor
test-optimal benchmarks/rovers/p04.proto 8 -s astar -H ma-lm-cut --ma-unfactor
test-optimal benchmarks/rovers/p05.proto 22 -s astar -H lm-cut --ma-unfactor
test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H ma-lm-cut --ma-unfactor
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut --ma-unfactor
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut --ma-unfactor

test-optimal codmap-2015/factored/depot/pfile1 10 -s astar -H lm-cut --ma-factor-dir
test-optimal codmap-2015/factored/depot/pfile2 15 -s astar -H ma-lm-cut --ma-factor-dir
test-optimal codmap-2015/factored/satellites/p05-pfile5 15 -s astar -H ma-lm-cut --ma-factor-dir


test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut-inc-local
test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H lm-cut-inc-local
test-optimal benchmarks/rovers/p01.proto 10 -s astar -H lm-cut-inc-local
test-optimal benchmarks/rovers/p02.proto 8 -s astar -H lm-cut-inc-local
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut-inc-local
test-optimal benchmarks/rovers/p04.proto 8 -s astar -H lm-cut-inc-local
test-optimal benchmarks/satellites/p01-pfile1.proto 9 -s astar -H lm-cut-inc-local
test-optimal benchmarks/satellites/p02-pfile2.proto 13 -s astar -H lm-cut-inc-local
test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H lm-cut-inc-local
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut-inc-local
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut-inc-local

test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut-inc-local --ma-unfactor
#test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H ma-lm-cut-inc-local --ma-unfactor
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut-inc-local --ma-unfactor
#test-optimal benchmarks/rovers/p04.proto 8 -s astar -H ma-lm-cut-inc-local --ma-unfactor
test-optimal benchmarks/rovers/p05.proto 22 -s astar -H lm-cut-inc-local --ma-unfactor
#test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H ma-lm-cut-inc-local --ma-unfactor
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut-inc-local --ma-unfactor
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut-inc-local --ma-unfactor

test-optimal codmap-2015/factored/depot/pfile1 10 -s astar -H lm-cut-inc-local --ma-factor-dir
#test-optimal codmap-2015/factored/depot/pfile2 15 -s astar -H ma-lm-cut-inc-local --ma-factor-dir
#test-optimal codmap-2015/factored/satellites/p05-pfile5 15 -s astar -H ma-lm-cut-inc-local --ma-factor-dir


test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/rovers/p01.proto 10 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/rovers/p02.proto 8 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/rovers/p04.proto 8 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/satellites/p01-pfile1.proto 9 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/satellites/p02-pfile2.proto 13 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut-inc-cache
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut-inc-cache

test-optimal benchmarks/depot/pfile1.proto 10 -s astar -H lm-cut-inc-cache --ma-unfactor
#test-optimal benchmarks/depot/pfile2.proto 15 -s astar -H ma-lm-cut-inc-cache --ma-unfactor
test-optimal benchmarks/rovers/p03.proto 11 -s astar -H lm-cut-inc-cache --ma-unfactor
#test-optimal benchmarks/rovers/p04.proto 8 -s astar -H ma-lm-cut-inc-cache --ma-unfactor
test-optimal benchmarks/rovers/p05.proto 22 -s astar -H lm-cut-inc-cache --ma-unfactor
#test-optimal benchmarks/satellites/p03-pfile3.proto 11 -s astar -H ma-lm-cut-inc-cache --ma-unfactor
test-optimal benchmarks/satellites/p04-pfile4.proto 17 -s astar -H lm-cut-inc-cache --ma-unfactor
test-optimal benchmarks/mablocks/problem-3-5-3.proto 17 -s astar -H lm-cut-inc-cache --ma-unfactor

test-optimal codmap-2015/factored/depot/pfile1 10 -s astar -H lm-cut-inc-cache --ma-factor-dir
#test-optimal codmap-2015/factored/depot/pfile2 15 -s astar -H ma-lm-cut-inc-cache --ma-factor-dir
#test-optimal codmap-2015/factored/satellites/p05-pfile5 15 -s astar -H ma-lm-cut-inc-cache --ma-factor-dir


test-satisficing benchmarks/depot/pfile1.proto -s lazy -H ff
test-satisficing benchmarks/depot/pfile2.proto -s lazy -H ff
test-satisficing benchmarks/rovers/p01.proto -s lazy -H ff
test-satisficing benchmarks/rovers/p02.proto -s lazy -H dtg
test-satisficing benchmarks/rovers/p03.proto -s lazy -H max
test-satisficing benchmarks/rovers/p04.proto -s lazy -H add
test-satisficing benchmarks/satellites/p01-pfile1.proto -s ehc -H ff
test-satisficing benchmarks/satellites/p02-pfile2.proto -s lazy -H dtg
test-satisficing benchmarks/satellites/p03-pfile3.proto -s ehc -H dtg
test-satisficing benchmarks/satellites/p04-pfile4.proto -s lazy -H ff
test-satisficing benchmarks/mablocks/problem-3-5-3.proto -s lazy -H ff

test-satisficing benchmarks/depot/pfile1.proto -s lazy -H ff:glob --ma-unfactor
test-satisficing benchmarks/depot/pfile2.proto -s lazy -H ma-ff --ma-unfactor
test-satisficing benchmarks/rovers/p03.proto -s lazy -H dtg --ma-unfactor
test-satisficing benchmarks/rovers/p04.proto -s lazy -H ff --ma-unfactor
test-satisficing benchmarks/rovers/p05.proto -s lazy -H ma-dtg --ma-unfactor
test-satisficing benchmarks/satellites/p03-pfile3.proto -s lazy -H ma-dtg --ma-unfactor
test-satisficing benchmarks/satellites/p04-pfile4.proto -s lazy -H ma-ff --ma-unfactor
test-satisficing benchmarks/mablocks/problem-3-5-3.proto -s lazy -H ff:loc --ma-unfactor

test-satisficing codmap-2015/factored/depot/pfile1 -s lazy -H ff --ma-factor-dir
test-satisficing codmap-2015/factored/depot/pfile2 -s lazy -H ma-ff --ma-factor-dir
test-satisficing codmap-2015/factored/satellites/p05-pfile5 -s lazy -H ma-ff --ma-factor-dir

if [ "$ERROR" = "yes" ]; then
    echo "!!!!!!!!!!!"
    echo "TEST FAILED"
    echo "!!!!!!!!!!!"
else
    echo "DONE"
fi
