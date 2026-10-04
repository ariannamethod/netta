CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: hybrid_router episode

.build/layout: ../turn13/episode.c ../turn33/earned_router.c ../turn34/sel4_router.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

# Preserve the parent stack byte-for-byte except for entry-point names.  The
# turn33 source has two textual mains (one in a provenance #if 0 block); the
# turn34 source has one.
.build/turn33_core.c: ../turn33/earned_router.c .build/layout
	sed 's/^int main(int argc, char \*\*argv) {$$/int turn33_router_main(int argc, char **argv) {/' $< > $@
	test "$$(diff $< $@ | grep -c '^[<>]')" = 4

.build/turn34_core.c: ../turn34/sel4_router.c .build/turn33_core.c
	sed 's/^int main(int argc, char \*\*argv) {$$/int turn34_router_main(int argc, char **argv) {/' $< > $@
	test "$$(diff $< $@ | grep -c '^[<>]')" = 2

hybrid_router: hybrid_router.c .build/turn34_core.c
	$(CC) $(CFLAGS) -I. hybrid_router.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

episode: hybrid_router
	ln -sfn hybrid_router $@
