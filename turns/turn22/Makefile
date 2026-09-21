CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all fixture
all: episode authority_replay

# The relocated frozen episode source still includes ../byte_recurrence.
# Recreate its old include layout privately, without editing any frozen file.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

episode: .build/layout
	$(CC) $(CFLAGS) .build/turn13/episode.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

authority_replay: replay.c authority.c authority.h
	$(CC) $(CFLAGS) replay.c authority.c $(LDLIBS) -o $@

.build/ref_authority.o: ../turn21/authority.c ../turn21/authority.h
	mkdir -p .build
	$(CC) $(CFLAGS) -Dar_init=ref_ar_init -Dar_quote=ref_ar_quote -Dar_observe=ref_ar_observe -Dar_grid=ref_ar_grid -c ../turn21/authority.c -o $@

authority_fixture: authority_fixture.c authority.c authority.h .build/ref_authority.o
	$(CC) $(CFLAGS) authority_fixture.c authority.c .build/ref_authority.o $(LDLIBS) -o $@

fixture: authority_fixture
	./authority_fixture
