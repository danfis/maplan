#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
PYTHON=python2
TRANSLATE="${DIR}/translate.py"

size=0
sizes=""
domains=()
problems=()
agent_names=()
agent_urls=""
for domain in $(ls domain-*.pddl); do
    agent_name=${domain##domain-}
    agent_name=${agent_name%%.pddl}
    problem=
    for p in $(ls *-${agent_name}.pddl); do
        if [ "$p" != "$domain" ]; then
            problem="$p"
            break
        fi
    done
    echo $agent_name: $domain, $problem
    domains+=($domain)
    problems+=($problem)
    agent_names+=($agent_name)
    agent_urls="$agent_urls --agent-url ipc://translate-agent-$size"
    sizes="$sizes $size"
    size=$(($size + 1))
done

pids=""
for i in $sizes; do
    "$PYTHON" "$TRANSLATE" "${domains[$i]}" "${problems[$i]}" $@ \
                           $agent_urls --agent-id $i \
                           --output ${agent_names[$i]}.proto \
                           --proto >${i}.log 2>&1 &
    pids="$pids $!"
done

trap "kill $pids" SIGINT SIGTERM

for pid in $pids; do
    wait $pid
done
