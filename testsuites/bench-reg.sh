#!/bin/bash

function run_ehc() {
    all=""

    elapsed=$(./bench-ehc $1 $2 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$(./bench-ehc $1 $2 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$(./bench-ehc $1 $2 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"

    echo $all | awk '{s+=$1}END{print "'EHC-$1-$2'",s/NR}' RS=" "
}

function run_lazy() {
    all=""

    elapsed=$(./bench-lazy $1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$(./bench-lazy $1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"
    elapsed=$(./bench-lazy $1 $2 $3 2>&1 >/dev/null | grep Elapsed | cut -f3 -d' ')
    all="$all $elapsed"

    echo $all | awk '{s+=$1}END{print "'Lazy-$1-$2-$3'",s/NR}' RS=" "
}

run_ehc gc ../data/ma-benchmarks/depot/pfile2.json
run_ehc add ../data/ma-benchmarks/depot/pfile2.json
run_ehc max ../data/ma-benchmarks/depot/pfile2.json
run_ehc ff ../data/ma-benchmarks/depot/pfile2.json
echo

run_ehc gc ../data/ma-benchmarks/depot/pfile3.json
run_ehc add ../data/ma-benchmarks/depot/pfile3.json
run_ehc max ../data/ma-benchmarks/depot/pfile3.json
run_ehc ff ../data/ma-benchmarks/depot/pfile3.json
echo

run_ehc gc ../data/ma-benchmarks/driverlog/pfile15.json
run_ehc add ../data/ma-benchmarks/driverlog/pfile15.json
run_ehc ff ../data/ma-benchmarks/driverlog/pfile15.json
echo

run_ehc gc  ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
run_ehc add ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
#run_ehc max ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
run_ehc ff  ../data/ma-benchmarks/logistics00/probLOGISTICS-10-0.json
echo

run_ehc gc  ../data/ma-benchmarks/elevators08/p03.json
run_ehc add ../data/ma-benchmarks/elevators08/p03.json
#run_ehc max ../data/ma-benchmarks/elevators08/p03.json
run_ehc ff  ../data/ma-benchmarks/elevators08/p03.json
echo

run_ehc gc  ../data/ma-benchmarks/rovers/p07.json
run_ehc add ../data/ma-benchmarks/rovers/p07.json
run_ehc max ../data/ma-benchmarks/rovers/p07.json
run_ehc ff  ../data/ma-benchmarks/rovers/p07.json

echo
echo

run_lazy gc heap ../data/ma-benchmarks/depot/pfile2.json
run_lazy gc bucket ../data/ma-benchmarks/depot/pfile2.json
run_lazy add heap ../data/ma-benchmarks/depot/pfile2.json
run_lazy add bucket ../data/ma-benchmarks/depot/pfile2.json
run_lazy max heap ../data/ma-benchmarks/depot/pfile2.json
run_lazy max bucket ../data/ma-benchmarks/depot/pfile2.json
run_lazy ff heap ../data/ma-benchmarks/depot/pfile2.json
run_lazy ff bucket ../data/ma-benchmarks/depot/pfile2.json
echo

run_lazy gc heap ../data/ma-benchmarks/depot/pfile3.json
run_lazy gc bucket ../data/ma-benchmarks/depot/pfile3.json
run_lazy add heap ../data/ma-benchmarks/depot/pfile3.json
run_lazy add bucket ../data/ma-benchmarks/depot/pfile3.json
run_lazy max heap ../data/ma-benchmarks/depot/pfile3.json
run_lazy max bucket ../data/ma-benchmarks/depot/pfile3.json
run_lazy ff heap ../data/ma-benchmarks/depot/pfile3.json
run_lazy ff bucket ../data/ma-benchmarks/depot/pfile3.json
echo

run_lazy gc heap ../data/ma-benchmarks/driverlog/pfile15.json
run_lazy gc bucket ../data/ma-benchmarks/driverlog/pfile15.json
#run_lazy add heap ../data/ma-benchmarks/driverlog/pfile15.json
#run_lazy add bucket ../data/ma-benchmarks/driverlog/pfile15.json
#run_lazy max heap ../data/ma-benchmarks/driverlog/pfile15.json
#run_lazy max bucket ../data/ma-benchmarks/driverlog/pfile15.json
run_lazy ff heap ../data/ma-benchmarks/driverlog/pfile15.json
run_lazy ff bucket ../data/ma-benchmarks/driverlog/pfile15.json
echo
