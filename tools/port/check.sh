#!/bin/sh
# usage: check.sh [NAME] -> runs rooms, walks and fuzzing with the native logic and compares with the baselines
n=${1:-nat}
/tmp/claude-0/det/all.sh $n > /dev/null
/tmp/claude-0/det/walks.sh $n > /dev/null
/tmp/claude-0/det/fuzz.sh $n > /dev/null
/tmp/claude-0/det/fuzz.sh ${n}v fuzz_vars=true > /dev/null
echo "rooms:"; /tmp/claude-0/det/cmp.sh base $n | tail -${LINES:-20}
