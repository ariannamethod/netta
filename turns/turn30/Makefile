CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: router3 episode turn29-calibrate

.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

router3: router3.c .build/layout
	$(CC) $(CFLAGS) router3.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

episode: router3
	ln -sfn router3 $@

turn29-calibrate:
	$(MAKE) -C ../turn29 all
	ln -sfn ../turn29/calibrate $@
