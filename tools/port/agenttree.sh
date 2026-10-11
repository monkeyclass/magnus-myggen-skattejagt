#!/bin/sh
# usage: agenttree.sh NAME -> /home/user/agents/NAME: a copy of the ScummVM build tree for one porting agent
set -e
d=/home/user/agents/$1
rm -rf $d; mkdir -p $d
cd /home/user/scummvm
tar cf - --exclude=./.git --exclude=./dists --exclude=./doc --exclude=./devtools --exclude=./po --exclude=./test \
	--exclude='*.dwp' --exclude=./scummvm --exclude='./engines/*/*' . | tar xf - -C $d
mkdir -p $d/engines/ivanoff
cd engines && tar cf - ivanoff | tar xf - -C $d/engines
for e in */; do [ -f $e/configure.engine ] && mkdir -p $d/engines/$e && cp $e/configure.engine $d/engines/$e/; done
echo "$d: $(du -sh $d | cut -f1)"
