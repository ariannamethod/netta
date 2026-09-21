CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: revision

revision: revision.c ../byte_recurrence/frontend.c ../byte_recurrence/frontend.h ../portable_recurrence/recurrence.c ../portable_recurrence/recurrence.h ../court4/transfer4_confirm_core.c
	$(CC) $(CFLAGS) ../byte_recurrence/frontend.c revision.c ../portable_recurrence/recurrence.c $(LDLIBS) -o $@
