#!/bin/sh
# copies the engine sources from the ScummVM tree into the repository (no build products)
rsync -a --delete --exclude '*.o' --exclude '*.dwo' --exclude '*.a' --exclude '.deps' /home/user/scummvm/engines/ivanoff/ /home/user/magnus-myggen-skattejagt/scummvm/engines/ivanoff/
