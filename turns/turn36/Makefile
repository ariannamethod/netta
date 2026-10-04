CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror

.PHONY: all
all: .build/hier_router episode

.build/turn34_core.c: ../turn34/sel4_router.c ../turn33/earned_router.c
	$(MAKE) -C ../turn34 all
	mkdir -p .build
	sed 's/^int main(int argc, char \*\*argv) {$$/int turn34_router_main(int argc, char **argv) {/' $< > $@
	test "$$(diff $< $@ | grep -c '^[<>]')" = 2

.build/hier_router: hier_router.c .build/turn34_core.c
	$(CC) $(CFLAGS) -I../turn34 hier_router.c ../../byte_recurrence/frontend.c ../../portable_recurrence/recurrence.c -lm -o $@

episode: .build/hier_router
	ln -sfn .build/hier_router episode
