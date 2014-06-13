#!/bin/bash

for json in $(find ../data/ma-benchmarks -name '*.json' | sort); do
    problem_dir=${json%/*}
    problem_dir=${problem_dir##*/}
    problem_base=$(basename $json)
    problem_base=${problem_base%.json}

    for heur in gc add max ff; do
        problem_log="bench/${problem_dir}-${problem_base}-$heur.log"
        rm -f $problem_log
        echo $json "-->" $problem_log
        ./bench-ehc $heur $json >>$problem_log 2>&1
        if cat $problem_log | grep 'Aborting' >/dev/null; then
            continue
        fi

        ./bench-ehc $heur $json >>$problem_log 2>&1
        ./bench-ehc $heur $json >>$problem_log 2>&1
        ./bench-ehc $heur $json >>$problem_log 2>&1
        ./bench-ehc $heur $json >>$problem_log 2>&1
    done
done
