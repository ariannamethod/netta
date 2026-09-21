CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: episode split

# Preserve the old episode source's include layout inside this private build.
.build/layout: ../turn13/episode.c ../../byte_recurrence/frontend.c ../../portable_recurrence/recurrence.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

episode: .build/layout
	$(CC) $(CFLAGS) .build/turn13/episode.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

split: split.c
	$(CC) $(CFLAGS) split.c $(LDLIBS) -o $@
