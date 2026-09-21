CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: episode authority_replay split oracle

# The frozen episode source still includes ../byte_recurrence. Recreate its old
# include layout privately here; no file outside turns/turn24 is written.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

episode: .build/layout
	$(CC) $(CFLAGS) .build/turn13/episode.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

# Five reference arms, compiled from turn22's frozen law without edit.
authority_replay: ../turn22/replay.c ../turn22/authority.c ../turn22/authority.h
	$(CC) $(CFLAGS) ../turn22/replay.c ../turn22/authority.c $(LDLIBS) -o $@

# The sixth reference arm, compiled from turn23's frozen law without edit.
split: ../turn23/split.c
	$(CC) $(CFLAGS) ../turn23/split.c $(LDLIBS) -o $@

# This turn's only new law, over the same turn22 hazard substrate.
oracle: oracle.c ../turn22/authority.c ../turn22/authority.h
	$(CC) $(CFLAGS) oracle.c ../turn22/authority.c $(LDLIBS) -o $@
