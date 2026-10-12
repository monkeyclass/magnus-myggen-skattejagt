#!/bin/sh
# usage: fuzz.sh PREFIX [ini lines] -> message fuzzing in several rooms and seeds, with coverage
p=$1; shift
for r in ${ROOMS:-22 23 24 27 29 34 35 36 38 43 44 49 52 53 55 18 40 46 47 63 25 26 28 30 31 32 33 37 39 41 42 45 50 51 54 56 57 58 59 62}; do for s in 1 2 3; do echo "$r $s"; done; done | \
xargs -P ${JOBS:-4} -n 2 sh -c 'SEED=$(($0*10+$1)) EVENTS="8000:esc 20000:click:80,58 23000:click:160,457 36000:fuzz:${FUZZ:-1000,100} 40000:crawl:30,3000 140000:quit" /tmp/claude-0/det/run.sh '$p'f$0-$1 $0 fuzz_dict=/home/user/analysis/decomp/fuzz.dict coverage=/tmp/claude-0/det/'$p'f$0-$1.cov '"$*"
