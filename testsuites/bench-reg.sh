#!/bin/bash

function run_ehc() {
    all=""

    elapsed=$($1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$($1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$($1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"

    echo $all | awk '{s+=$1}END{print "'EHC-$2-$3'",s/NR}' RS=" "
}

run_ehc ./bench-ehc gc ../data/ma-benchmarks/depot/pfile2.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/depot/pfile2.json
run_ehc ./bench-ehc max ../data/ma-benchmarks/depot/pfile2.json
run_ehc ./bench-ehc ff ../data/ma-benchmarks/depot/pfile2.json
echo

run_ehc ./bench-ehc gc ../data/ma-benchmarks/depot/pfile3.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/depot/pfile3.json
run_ehc ./bench-ehc max ../data/ma-benchmarks/depot/pfile3.json
run_ehc ./bench-ehc ff ../data/ma-benchmarks/depot/pfile3.json
echo

run_ehc ./bench-ehc gc ../data/ma-benchmarks/driverlog/pfile15.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/driverlog/pfile15.json
run_ehc ./bench-ehc ff ../data/ma-benchmarks/driverlog/pfile15.json
echo

run_ehc ./bench-ehc gc  ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
#run_ehc ./bench-ehc max ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
run_ehc ./bench-ehc ff  ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
echo

run_ehc ./bench-ehc gc  ../data/ma-benchmarks/elevators08/p03.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/elevators08/p03.json
#run_ehc ./bench-ehc max ../data/ma-benchmarks/elevators08/p03.json
run_ehc ./bench-ehc ff  ../data/ma-benchmarks/elevators08/p03.json
echo

run_ehc ./bench-ehc gc  ../data/ma-benchmarks/rovers/p07.json
run_ehc ./bench-ehc add ../data/ma-benchmarks/rovers/p07.json
run_ehc ./bench-ehc max ../data/ma-benchmarks/rovers/p07.json
run_ehc ./bench-ehc ff  ../data/ma-benchmarks/rovers/p07.json

