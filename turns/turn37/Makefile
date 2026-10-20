CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror

.PHONY: all
all: .build/direct_router episode

.build/turn34_core.c: ../turn34/sel4_router.c ../turn33/earned_router.c
	$(MAKE) -C ../turn34 all
	mkdir -p .build
	sed 's/^int main(int argc, char \*\*argv) {$$/int turn34_router_main(int argc, char **argv) {/' $< > $@
	test "$$(diff $< $@ | grep -c '^[<>]')" = 2

.build/direct_router: direct_router.c .build/turn34_core.c
	$(CC) $(CFLAGS) -I../turn34 direct_router.c ../../byte_recurrence/frontend.c ../../portable_recurrence/recurrence.c -lm -o $@

episode: .build/direct_router
	ln -sfn .build/direct_router episode
