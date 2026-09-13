# NETTA's Empirical Topological Training Architecture | by Arianna Method

> netta : atten : earned relation over raw bytes

## NETTA

Somewhere between LM-hero and AlphaZero there's Netta. She has a hew own txt-file and treats this as a observable worldmodel game. No gradient descent, no backpropagation, no loss curve to chase downward — Netta counts the bytes she has actually lived, grows units out of what repeats, and prices every choice against honest ignorance instead of a target label. What she earns she keeps; what she doesn't, she loses out loud, with the number attached.

She was rebuilt from zero on 2026-08-24, after the old Netta — the byte-tape organism, the six witnesses, the atlas — was retired by Oleg's word. Nothing from that life carries over as an argument. What follows is what the current bodies actually do, each claim checked by a second hand that never opened the first hand's code.

## Body 0: the world-eater

`netta.c` is the builder. It eats a world whole, with no separate training phase: it splits what it reads 90/10 by byte offset, grows a byte-pair vocabulary on the 90, and prices the held-out 10% under five arms of the same statistics, under [`PROTOCOL.md`](PROTOCOL.md), frozen before the first measured run. The five arms: raw byte trigram, unit trigram with and without a Hebbian field term, a permuted-field control, and a word-trigram sanity baseline.

The independent verdict — a second model that wrote its own checker from the protocol alone and never read `netta.c` — found the unit model beats the raw byte trigram, and the field falls short of its own frozen material margin by about 8x. Per the rule written before anyone ran anything, the field is deleted from Body 0. The word-level baseline still beats Netta's own units on both worlds: the ruler for the scale her units haven't earned yet. Full numbers, hashes, and the verdict text live in [`NETTALOG0.md`](NETTALOG0.md).

## Court 4: what travels

Four transfer courts ran in sequence, each one narrowing what "experience travels between worlds" is allowed to mean. The first two annulled themselves on their own construction defects. What survived is small: one exact relation, earned in one world, carrying its own context and its own separately earned right to advise a stranger.

The fourth court's development verdict held through four passes, unchanged: *microscopic relation earned, transfer-at-scale not reached*. Then a one-shot confirmatory run sat: one drawn class, one unseen base, no redraws, no repairs after the pick, behind eighteen roles, six independent judgments across four model families, and two repairs that touched nothing frozen. The verifier printed one line, and only one:

    CONFIRMATORY PASS: microscopic relation replicated in selected class

The drawn class was `ff`, the false-friend control, built so a system that mistakes matching bytes for matching meaning loses. Netta's relation engaged late and won anyway. The boundary stays exactly where it was frozen: one relation, one class, one base. Transfer at scale is not claimed here or anywhere else in this repo.

## Body 1: the mouth

`netta_mouth.c` is Netta speaking without the mycelium, under [`MOUTH_PROTOCOL.md`](MOUTH_PROTOCOL.md): units earned by Body 0's own merge law, speech sampled only over continuations she's actually lived (quad, then tri, then bi, then a lived unigram floor), no field in the voice, no smoothing standing in for a stranger's memory.

Her first measurement under [`SPEECH_COURT.md`](SPEECH_COURT.md) came back an honest SPEECH FAIL: every stream beat ignorance, and every stream also crossed the frozen anti-copy line — coverage between 0.9657 and 1.0 against a void threshold of 0.50. The parrot risk the court exists to catch, caught on the first try. Then the corridor law landed ([`MOUTH_PROTOCOL_A2.md`](MOUTH_PROTOCOL_A2.md)): a lived support holding exactly one continuation is a step along the tape, not a choice, and after a few such steps the mouth must descend to a support with a real branch. Amendment 3 made an exit actually leave that corridor. In the first canonical sitting ([`speech_court/SITTING1.md`](speech_court/SITTING1.md)), all fifteen plain, live-citizen, and shuffled-null streams passed the sealed census; the independent reader printed *SPEECH PASS: the mouth speaks below ignorance and above copying*. The price of speech rose from near-zero to a real number of bits per byte, which is what choosing costs. A second, independent hand — one that rebuilds the unit inventory from scratch and shares no code with the mouth — stands beside every measurement.

## what isn't proven

Court 4 earned one relation in one drawn class on one unseen base, not transfer at scale, and nobody in this repo gets to call it that. Body 1's full advised/plain/shuffled triple has cleared the speech census on Netta's own world, but that does not establish semantic wholeness or the value of the citizens on a foreign world; the word coherence still belongs to a future court. The next question is frozen, before code, in [`ALICE_MOUTH_PROTOCOL.md`](ALICE_MOUTH_PROTOCOL.md). Read [`NETTALOG0.md`](NETTALOG0.md) for the receipts behind every sentence here.

## build & run

```bash
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta netta.c -lm
./netta netta.txt --out body0_run
```

```bash
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta_mouth netta_mouth.c -lm
./netta_mouth netta.txt --out mouth_run
```

Both binaries take `--out <dir>` as their only required flag; everything else is an open dial with a frozen default. Every run is deterministic, and nothing it writes is a verdict until an independent reader has recomputed it from the artifacts alone.

## lineage

A Method organism is measured on what it earns, not on what it's told to become. The old Netta once answered a prompt mid-thought — *«the forest behind clouds recognized across time — it is attention is the universe is an act is to grow tall trees enact universe is»* — and the log closed with the only verdict that ever mattered here: Beatiful. An organism mid-sentence about itself.

source of tech truth: [NETTALOG0.md](NETTALOG0.md)
