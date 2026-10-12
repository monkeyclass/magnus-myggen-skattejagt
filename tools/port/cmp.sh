#!/bin/sh
# usage: cmp.sh A B -> compares the traces of runs with prefixes A and B; prints the first difference
f() { grep -v "^Using configuration\|native logic\|^Machine:\|^TextSprite \|^WritePrivateProfileString" $1; }
same=0; diff=0
for a in /tmp/claude-0/det/$1*.log; do
	n=${a#/tmp/claude-0/det/$1}; b=/tmp/claude-0/det/$2$n
	[ -f $b ] || continue
	if f $a | cmp -s - $b.f 2>/dev/null || { f $b > $b.f; f $a | cmp -s - $b.f; }; then same=$((same+1)); else
		diff=$((diff+1)); echo "== ${n%.log}: first difference:"; f $a | diff - $b.f | head -${3:-6}; fi
done
echo "same $same, different $diff"
