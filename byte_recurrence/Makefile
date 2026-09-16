CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all test
all: byte_trace byte_live byte_collect

byte_trace: frontend.c frontend.h trace.c ../court4/transfer4_confirm_core.c
	$(CC) $(CFLAGS) frontend.c trace.c $(LDLIBS) -o $@

byte_live: frontend.c frontend.h live.c ../portable_recurrence/recurrence.c ../portable_recurrence/recurrence.h ../court4/transfer4_confirm_core.c
	$(CC) $(CFLAGS) frontend.c live.c ../portable_recurrence/recurrence.c $(LDLIBS) -o $@

byte_collect: frontend.c frontend.h collect.c ../portable_recurrence/recurrence.c ../portable_recurrence/recurrence.h ../court4/transfer4_confirm_core.c
	$(CC) $(CFLAGS) frontend.c collect.c ../portable_recurrence/recurrence.c $(LDLIBS) -o $@

test: byte_trace
	./byte_trace --selftest
