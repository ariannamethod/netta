CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: case_router episode

# Private staging preserves the frozen turn13 source's relative includes.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

case_router: case_router.c .build/layout
	$(CC) $(CFLAGS) case_router.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

episode: case_router
	ln -sfn case_router $@
