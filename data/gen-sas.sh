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

        out_sas=${pddl%%.pddl}.sas

        echo "$pddl"
        $TRANSLATE "$domain_pddl" "$pddl" --output tmp.sas >/dev/null 2>&1
        $PREPROCESS >/dev/null 2>&1 <tmp.sas
        mv output $out_sas
        rm tmp.sas
    done
done
