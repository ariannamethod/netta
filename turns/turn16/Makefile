CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all fixture
all: episode authority_replay

episode: ../turn13/episode.c ../byte_recurrence/frontend.c ../byte_recurrence/frontend.h ../portable_recurrence/recurrence.c ../portable_recurrence/recurrence.h ../court4/transfer4_confirm_core.c
	$(CC) $(CFLAGS) ../turn13/episode.c ../byte_recurrence/frontend.c ../portable_recurrence/recurrence.c $(LDLIBS) -o $@

authority_replay: replay.c authority.c authority.h
	$(CC) $(CFLAGS) replay.c authority.c $(LDLIBS) -o $@

authority_fixture: authority_fixture.c authority.c authority.h
	$(CC) $(CFLAGS) authority_fixture.c authority.c $(LDLIBS) -o $@

fixture: authority_fixture
	./authority_fixture
