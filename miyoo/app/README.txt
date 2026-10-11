Magnus & Myggen 2: Den store skattejagt - for Onion OS (Miyoo Mini)
=====================================================================

This is ScummVM with the "ivanoff" engine, which runs the game logic of the
original Windows program. It needs your own copy of the game, from the Nordic
retail CD (2002). Put these in the game/ folder next to this file:

  MM2.DAT, MM2.IDX     from the CD
  DK/ (or N/, S/, SF/) the language folder from the CD
  LAB/                 from the CD (labyrinth videos)
  mm2run.exe           from an installation, or from the CD's DATA1.CAB
                       (prepare-game.sh in the project does all this)

The language is Danish; for another one, write its code (nb, sv or fi) in
language.txt.

Controls (ScummVM defaults on the Miyoo Mini):
  D-pad   move the pointer        R + D-pad  move it slowly
  A       click                   B          right click
  Y       Esc (skips cut-scenes)  Start      ScummVM menu (quit)

Saves (from the game's own save screen) are kept in saves/. The log of the
last run is log.txt.
