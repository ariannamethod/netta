CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: bank episode

# Recreate the frozen source's include layout only inside this turn.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

bank: bank.c .build/layout
	$(CC) $(CFLAGS) bank.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

# The unchanged source generator invokes HERE/episode for emit and trace.
episode: bank
	ln -sfn bank $@
