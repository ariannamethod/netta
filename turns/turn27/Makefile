CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all references
all: episode authority_replay

# Only the private build layout is written. Frozen episode bytes and their
# relative includes remain unchanged, as in turn24 and turn25.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

episode: .build/layout
	$(CC) $(CFLAGS) .build/turn13/episode.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

# Six arms. turn25's latch.c and turn22's authority.c are linked unedited.
authority_replay: replay.c cusum.c cusum.h ../turn25/latch.c ../turn25/latch.h ../turn22/authority.c ../turn22/authority.h
	$(CC) $(CFLAGS) replay.c cusum.c ../turn25/latch.c ../turn22/authority.c $(LDLIBS) -o $@

# Equivalence references, built from frozen sources of their own turns. The
# hysteresis arm is checked against turn25's replay; turn24's oracle binary is
# already retained at its frozen sha and is used directly, not rebuilt.
references: .build/turn25_replay

.build/turn25_replay: ../turn25/replay.c ../turn25/latch.c ../turn25/latch.h ../turn22/authority.c ../turn22/authority.h
	mkdir -p .build
	$(CC) $(CFLAGS) ../turn25/replay.c ../turn25/latch.c ../turn22/authority.c $(LDLIBS) -o $@
