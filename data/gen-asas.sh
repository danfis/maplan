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

        out_asas=${pddl%%.pddl}.asas
        addl=${pddl%%.pddl}.addl

        if [ ! -f "$addl" ]; then
            continue
        fi

        echo "$pddl" "$addl"
        $TRANSLATE "$domain_pddl" "$pddl" "$addl" --output tmp.asas >/dev/null 2>&1
        $PREPROCESS >/dev/null 2>&1 <tmp.asas
        mv output $out_asas
        rm tmp.asas
    done
done
