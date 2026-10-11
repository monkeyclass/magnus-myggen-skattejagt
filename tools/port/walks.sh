#!/bin/sh
# usage: walks.sh PREFIX [ini lines] -> random walks from a new game, seeds 1-8
p=$1; shift
for s in 1 2 3 4 5 6 7 8; do echo $s; done | xargs -P ${JOBS:-4} -I{} sh -c "SEED={} EVENTS='8000:esc 30000:crawl:300,3000 940000:quit' /tmp/claude-0/det/run.sh ${p}w{} 15 $*"
