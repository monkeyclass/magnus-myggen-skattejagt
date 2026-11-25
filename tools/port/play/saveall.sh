#!/bin/sh
for r in ${ROOMS:-22 23 24 25 26 27 28 29 31 32 34 35 37 38 39 41 43 44 45 49 50 51 52 53 54 56 57 58 59}; do
  /tmp/claude-0/cmp/room.sh $r >/dev/null 2>&1
  /tmp/claude-0/cmp/clicks.sh $r > /tmp/claude-0/cmp/$r/clicks.txt 2>&1
  /tmp/claude-0/cmp/savecmp.sh $r
done
