CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all test
all: episode authority_replay fixture

# Only the private build layout is written. Frozen episode bytes and their
# relative includes remain unchanged, as in turn24.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

episode: .build/layout
	$(CC) $(CFLAGS) .build/turn13/episode.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

authority_replay: replay.c latch.c latch.h ../turn22/authority.c ../turn22/authority.h
	$(CC) $(CFLAGS) replay.c latch.c ../turn22/authority.c $(LDLIBS) -o $@

fixture: fixture.c latch.c latch.h ../turn22/authority.c ../turn22/authority.h
	$(CC) $(CFLAGS) fixture.c latch.c ../turn22/authority.c $(LDLIBS) -o $@

# Same handcrafted tape can be inspected with this untouched incoming reader.
.build/reference_replay: ../turn22/replay.c ../turn22/authority.c ../turn22/authority.h
	mkdir -p .build
	$(CC) $(CFLAGS) ../turn22/replay.c ../turn22/authority.c $(LDLIBS) -o $@

test: fixture
	./fixture
