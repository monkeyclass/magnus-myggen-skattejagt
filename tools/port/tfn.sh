#!/bin/sh
# Print typed decompiled function(s) by address. usage: tfn.sh 00476910 [...]
for a in "$@"; do
	awk -v a="$a" '/^\/\/ ==== /{p=($0 ~ ("@ " a "$"))} p' /home/user/analysis/decomp/typed.c
done
