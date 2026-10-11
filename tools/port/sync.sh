#!/bin/sh
# Copies the engine sources from the ScummVM tree into the repository.
src=/home/user/scummvm/engines/ivanoff
dst=/home/user/magnus-myggen-skattejagt/scummvm/engines/ivanoff
rm -rf $dst && mkdir -p $dst
cd $src && find . -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.mk' -o -name 'configure.engine' -o -name 'credits.pl' -o -name 'POTFILES' \) | while read f; do mkdir -p $dst/$(dirname $f); cp $f $dst/$f; done
