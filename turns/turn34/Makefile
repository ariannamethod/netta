CC = cc
CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror
LDLIBS = -lm

.PHONY: all
all: sel4_router episode

# Private staging preserves the frozen turn13 and turn33 sources' relative
# includes; nothing frozen is copied or edited.
.build/layout: ../turn13/episode.c ../turn33/earned_router.c ../../byte_recurrence/frontend.c ../../byte_recurrence/frontend.h ../../portable_recurrence/recurrence.c ../../portable_recurrence/recurrence.h ../../court4/transfer4_confirm_core.c
	mkdir -p .build/turn13
	ln -sfn ../../../turn13/episode.c .build/turn13/episode.c
	ln -sfn ../../../byte_recurrence .build/byte_recurrence
	ln -sfn ../../../portable_recurrence .build/portable_recurrence
	ln -sfn ../../../court4 .build/court4
	touch $@

# One mechanical change, verified: the frozen turn33 router staged with its
# main renamed so sel4_router.c owns the entry point. The rename hits two
# lines — 507 (inside the #if 0 turn31 provenance block) and 868 (live) —
# and any drift beyond those fails the build.
.build/turn33_core.c: ../turn33/earned_router.c .build/layout
	sed 's/^int main(int argc, char \*\*argv) {$$/int turn33_router_main(int argc, char **argv) {/' $< > $@
	test "$$(diff $< $@ | grep -c '^[<>]')" = 4

sel4_router: sel4_router.c .build/turn33_core.c
	$(CC) $(CFLAGS) -I. sel4_router.c .build/byte_recurrence/frontend.c .build/portable_recurrence/recurrence.c $(LDLIBS) -o $@

episode: sel4_router
	ln -sfn sel4_router $@
