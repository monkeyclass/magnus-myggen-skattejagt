#!/bin/sh
# usage: all.sh PREFIX [ini lines...] -> runs every room in parallel
p=$1; shift
for r in 15 16 18 19 20 21 22 23 24 25 26 27 28 29 31 32 34 35 36 37 38 39 40 41 42 43 44 45 46 47 49 50 51 52 53 54 55 56 57 58 59 62 63 64 66 67 68 69 70 71 72 73 74 75 76 77 78 79 100 101 102 103; do
  echo $r
done | xargs -P ${JOBS:-8} -I{} sh -c "/tmp/claude-0/det/run.sh $p{} {} $*"
