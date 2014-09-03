#!/bin/bash

TRANSLATE=../third-party/translate/translate.py
PREPROCESS=../third-party/preprocess/preprocess
TOPDIR="$1"

if [ "$TOPDIR" = "" ]; then
    TOPDIR="."
fi

for domain_pddl in $(find -P $TOPDIR -name 'domain.pddl'); do
    echo 'DOMAIN' $domain_pddl

    dir=$(dirname $domain_pddl)
    for pddl in $dir/*.pddl; do
        if [ "$(basename $pddl)" = "domain.pddl" ]; then
            continue
        fi

        out_proto=${pddl%%.pddl}.proto
        addl=${pddl%%.pddl}.addl

        echo "$pddl" "$addl"
        python2 $TRANSLATE "$domain_pddl" "$pddl" $addl --output $out_proto --proto >/dev/null 2>&1
    done
done
