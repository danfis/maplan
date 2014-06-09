#!/bin/bash

if [ $# -lt 4 ]; then
    echo "Usage: $0 translate.py preprocess-bin fd-to-json.py dirs..."
    exit -1
fi

TRANSLATE="$1"
PREPROCESS="$2"
FD_TO_JSON="$3"
shift
shift
shift

for dir in $@; do
    domain="$dir/domain.pddl"
    if [ ! -f $domain ]; then
        continue
    fi

    echo "Directory: $dir"

    for pddl in $(find $dir -name '*.pddl'); do
        base=${pddl%%.pddl}
        addl=${base}.addl
        # TODO: Ignore addl just for now

        if [ "${base##*/}" = "domain" ];then
            continue
        fi

        echo "$TRANSLATE $domain $pddl --> output.sas"
        if ! $TRANSLATE $domain $pddl; then
            exit -1
        fi

        echo "$PREPROCESS <output.sas --> output"
        if ! $PREPROCESS <output.sas; then
            exit -1
        fi

        echo "$FD_TO_JSON <output >$base.json"
        if ! $FD_TO_JSON <output >$base.json; then
            exit -1
        fi

        echo "cp output $base.sas"
        cp output $base.sas
        echo
    done;
done
