CC ?= cc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror

all: recurrence_test

recurrence_test: recurrence.c recurrence.h recurrence_test.c
	$(CC) $(CFLAGS) recurrence.c recurrence_test.c -lm -o $@

test: recurrence_test
	./recurrence_test

.PHONY: all test
