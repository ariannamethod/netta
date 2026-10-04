#!/usr/bin/env python3
"""NETTA is all you need.

Statistical coherence from scratch: no transformers, no gradients, no optimizer,
no pretrained anything. Ordinary text in, earned speech out.

One file, stdlib only, deterministic. It eats a world of ordinary text, grows
units out of whatever that world repeats often enough to deserve a name, and
then speaks -- sampling only over continuations it has actually lived. A court
preregistered before the first judged byte prices the result against honest
ignorance and against copying, and prints the speech beside the numbers.

    python3 netta.py island.txt
    python3 netta.py island.txt --seed 42 --time
    python3 netta.py island.txt --out run --report court.txt

Mirror of the C organism: netta.c (Body 0, the merge law) and netta_mouth.c
(Body 1, the mouth) under MOUTH_PROTOCOL.md with amendments 2 and 3, judged by
netta_mouth_check.c under SPEECH_COURT.md. The PRNG, the tie-breaks and the
sampling order are ported from that C step for step, and PARITY.md beside this
file carries what was measured: byte-identical speech, byte-identical traces and
byte-identical court reports, over 25 streams, two islands and nine dial
settings, with the C's own independent reader accepting this file's output.

One default differs from the C on purpose: --corridor is 1 here and 3 there.
K=3 fails the anti-copy census on the canonical island; K=1 is the dial the
court's first sitting pinned. Pass --corridor 3 for the C's compiled default.

Out of scope here, and named rather than hidden: the Court-4 citizens adapter
(MOUTH_PROTOCOL §4), which needs a 36101-byte sealed capsule this file cannot
carry. The plain mouth is the one that passed the court's first sitting.
"""

import argparse
import math
import os
import sys
import time
from bisect import bisect_left
from heapq import heapify, heappop, heappush

START = time.perf_counter()

BASE_UNITS = 256
PACK = 16                       # unit id bits in a packed n-gram key
PACK_MASK = (1 << PACK) - 1
REP_WINDOW = 12
REP_PENALTY = 0.5
SPEAK_HARD = 256                # grace bytes to let a sentence finish
MAX_EM = 65536
VOID_LINE = 0.50                # body0/verdict.md: at or above this it is a quote
MIN_MATCH = 32                  # the census counts verbatim tape runs from here up

PASS_LINE = "SPEECH PASS: the mouth speaks below ignorance and above copying"


class Refusal(Exception):
    """The organism will not speak, or the court will not sign."""


class Rng:
    """xorshift64. Three shifts and an xor: the entire source of chance here."""

    MASK = (1 << 64) - 1

    def __init__(self, state):
        self.s = (state & self.MASK) or 1

    def next(self):
        x = self.s
        x ^= (x << 13) & self.MASK
        x ^= x >> 7
        x ^= (x << 17) & self.MASK
        self.s = x
        return x

    def below(self, n):
        return self.next() % n

    def double(self):
        return (self.next() >> 11) / 9007199254740992.0


def grow_units(world, merges, min_pair):
    """Body 0's merge law. The most frequent adjacent pair becomes one unit,
    ties broken by the smaller packed key, until the budget runs out or nothing
    repeats MIN_PAIR times any more.

    Nothing descends here. No weights, no initialization, no objective: the
    inventory is a fact about the world, and it is the same inventory every time
    you run it. The C rescans the whole stream every round; this keeps the same
    counts incrementally over a linked list.
    """
    n = len(world)
    sym = list(world)
    prv = list(range(-1, n - 1))
    nxt = list(range(1, n + 1))
    nxt[n - 1] = -1

    count = {}
    where = {}
    # the entire training loop:
    for i in range(n - 1):
        key = (sym[i], sym[i + 1])
        count[key] = count.get(key, 0) + 1
        where.setdefault(key, []).append(i)
    heap = [(-c, a, b) for (a, b), c in count.items()]
    heapify(heap)

    def bump(key, delta):
        c = count.get(key, 0) + delta
        if c:
            count[key] = c
            if delta > 0:
                heappush(heap, (-c, key[0], key[1]))
        else:
            del count[key]

    exp = [bytes((i,)) for i in range(BASE_UNITS)]
    while len(exp) - BASE_UNITS < merges:
        best = None
        while heap:
            negc, a, b = heappop(heap)
            live = count.get((a, b), 0)
            if live == -negc:
                best = (live, a, b)
                break
            if live:                        # stale count, same pair, still in play
                heappush(heap, (-live, a, b))
        if best is None or best[0] < min_pair:
            break

        _, a, b = best
        new = len(exp)
        exp.append(exp[a] + exp[b])
        for i in where.pop((a, b), ()):     # left to right, non-overlapping, as the C
            if sym[i] != a:
                continue
            j = nxt[i]
            if j < 0 or sym[j] != b:
                continue
            left, right = prv[i], nxt[j]
            bump((a, b), -1)
            if left >= 0:
                sl = sym[left]
                bump((sl, a), -1)
                bump((sl, new), 1)
                where.setdefault((sl, new), []).append(left)
            if right >= 0:
                sr = sym[right]
                bump((b, sr), -1)
                bump((new, sr), 1)
                where.setdefault((new, sr), []).append(i)
            sym[i] = new
            sym[j] = -1
            nxt[i] = right
            if right >= 0:
                prv[right] = i

    return [s for s in sym if s >= 0], exp


def starts_upper(e):
    return 65 <= e[0] <= 90


def ends_sentence(e):
    return e[-1] in b".!?\n"


class Tape:
    """The lived stream and every n-gram the organism has actually walked."""

    def __init__(self, stream, exp, order):
        self.stream = stream
        self.exp = exp
        n = len(stream)
        n1 = [0] * len(exp)
        for u in stream:
            n1[u] += 1
        self.alive = [u for u in range(len(exp)) if n1[u]]
        self.uni = [(u, n1[u]) for u in self.alive]
        self.bi = sorted((stream[i] << PACK) | stream[i + 1]
                         for i in range(n - 1))
        self.tri = sorted((stream[i] << (2 * PACK)) | (stream[i + 1] << PACK) | stream[i + 2]
                          for i in range(n - 2))
        self.quad = sorted((stream[i] << (3 * PACK)) | (stream[i + 1] << (2 * PACK)) |
                           (stream[i + 2] << PACK) | stream[i + 3]
                           for i in range(n - 3)) if order >= 4 else []
        self.starts = [i for i in range(n - 3)
                       if starts_upper(exp[stream[i]])
                       and (i == 0 or ends_sentence(exp[stream[i - 1]]))]
        if not self.starts:
            raise Refusal("no sentence starts")


def support(tape, em, level, order):
    """Lived continuations at one explicit level, in ascending unit id.

    No smoothing, no backoff mass, no unseen token, no prior: the mouth may only
    say what it has lived, so a level that was never walked returns nothing and
    the caller drops to a shorter memory.
    """
    nem = len(em)
    if level == 4:
        if order < 4 or nem < 3 or not tape.quad:
            return []
        keys = tape.quad
        prefix = (em[-3] << (2 * PACK)) | (em[-2] << PACK) | em[-1]
    elif level == 3:
        if nem < 2:
            return []
        keys = tape.tri
        prefix = (em[-2] << PACK) | em[-1]
    elif level == 2:
        if nem < 1:
            return []
        keys = tape.bi
        prefix = em[-1]
    else:
        return tape.uni

    lo = bisect_left(keys, prefix << PACK)
    hi = bisect_left(keys, (prefix + 1) << PACK)
    out = []
    i = lo
    while i < hi:
        tok = keys[i] & PACK_MASK
        j = i + 1
        while j < hi and keys[j] & PACK_MASK == tok:
            j += 1
        out.append((tok, j - i))
        i = j
    return out


def lawful_support(tape, em, order, corridor_k, corr):
    """The highest non-empty lived support, quad -> tri -> bi -> lived unigram,
    under the corridor law.

    Amendment 2: a support holding exactly one continuation is not a choice, it
    is a rail, and a chain of rails is a quotation in progress. After K of them
    the mouth must descend to a support with a real branch.

    Amendment 3: an exit must exit. The rail's own token is closed for that one
    choice, because changing level is not by itself changing path.

    Returns (candidates, level, vetoed token or None, new corridor counter).
    """
    cand, level = [], 0
    for lvl in (4, 3, 2, 1):
        cand = support(tape, em, lvl, order)
        if cand:
            level = lvl
            break
    if not cand:
        raise Refusal("empty lived support")

    if len(cand) >= 2:
        return cand, level, None, 0
    if corridor_k and corr >= corridor_k:
        rail = cand[0][0]
        for lvl in range(level - 1, 0, -1):
            alt = support(tape, em, lvl, order)
            if len(alt) >= 2:
                admitted = [c for c in alt if c[0] != rail]
                if len(admitted) == len(alt):
                    raise Refusal("lower support lost the corridor continuation")
                return admitted, lvl, rail, 0
    return cand, level, None, corr + 1 if corridor_k else corr


def speak(tape, seed, args):
    """The mouth.

    Opens on three tokens of lived tape picked by the seed, then chooses: score
    every lived continuation by how often the world already did it, damp what
    this stream just said, keep the top K, soften by temperature, sample. That
    is the whole generative model. There is no second network in here quietly
    doing the real work.
    """
    rng = Rng(seed ^ 0x9E3779B97F4A7C15)
    sp = tape.starts[rng.below(len(tape.starts))]
    em = [tape.stream[sp], tape.stream[sp + 1], tape.stream[sp + 2]]
    out = bytearray()
    trace = []
    for k in range(3):
        e = tape.exp[em[k]]
        out += e
        trace.append("%d\t%d\t%d\t0\t0\t0\t0\t%d\t0\t1\t0\t-" % (k, em[k], sp + k, len(e)))

    corr = 0
    want, hard = args.bytes, args.bytes + SPEAK_HARD
    while len(out) < want and len(em) + 1 < MAX_EM:
        before = corr
        cand, level, veto, corr = lawful_support(tape, em, args.order, args.corridor, corr)

        window = em[-REP_WINDOW:]
        occ = 0
        scored = []
        for tok, cnt in cand:
            occ += cnt
            weight = float(cnt)
            score = (math.log(weight + 1e-300)
                     - math.log(1.0 + REP_PENALTY * window.count(tok)))
            scored.append((tok, cnt, score))
        # the C runs a partial selection sort; score down, unit id up
        scored.sort(key=lambda c: (-c[2], c[0]))

        limit = min(len(scored), args.topk)
        weights = []
        top = -1e300
        for i in range(limit):
            v = scored[i][2] / args.temp
            weights.append(v)
            if v > top:
                top = v
        total = 0.0
        for i in range(limit):
            weights[i] = math.exp(weights[i] - top)
            total += weights[i]
        r = rng.double() * total
        cum = 0.0
        at = 0
        for i in range(limit):
            cum += weights[i]
            if cum > r:
                at = i
                break

        tok, cnt, _ = scored[at]
        e = tape.exp[tok]
        em.append(tok)
        out += e
        trace.append("%d\t%d\t-\t%d\t%d\t%d\t%d\t%d\t0\t1\t%d\t%s"
                     % (len(em) - 1, tok, level, len(cand), occ, cnt, len(e), before,
                        "-" if veto is None else veto))
        if len(out) >= want and not ends_sentence(e) and len(out) < hard:
            want = len(out) + 1

    return bytes(out), em, trace


def ear(tape, tokens, order, corridor_k):
    """The court's ear. Every emitted token priced inside the lived support the
    law actually selected, against honest ignorance -- uniform over every unit
    the organism has ever lived.

    The court re-derives that support from the token sequence alone. It does not
    take the mouth's word for the mouth's own paperwork, and it refuses a stream
    whose token was never a lived continuation of its own context.
    """
    model = 0.0
    ignorance = 0.0
    per_token = math.log2(len(tape.alive))
    past = []
    corr = 0
    for i, tok in enumerate(tokens):
        if i < 3:
            # the lived opening is priced, but it stands outside the corridor law
            cand = []
            for lvl in range(3 if i >= 2 else i + 1, 0, -1):
                cand = support(tape, past, lvl, order)
                if cand:
                    break
        else:
            cand, _, _, corr = lawful_support(tape, past, order, corridor_k, corr)
        occ = chosen = 0
        for t, c in cand:
            occ += c
            if t == tok:
                chosen = c
        if not chosen:
            raise Refusal("emitted token is outside lawful lived support")
        model += -math.log2(chosen / occ)
        ignorance += per_token
        past.append(tok)
    return model, ignorance


def census(speech, world):
    """Anti-copy. Coverage of the stream by verbatim tape runs of 32 bytes or
    more, against the void line frozen in body0/verdict.md. Copying is not
    speech, and an organism that only quotes is cheap to price precisely because
    it is saying nothing.
    """
    sn = len(speech)
    covered = bytearray(sn)
    longest = z = 0
    for at in range(sn):
        remain = sn - at
        # a match of length z at at-1 guarantees z-1 here; start from what is owed
        z = min(z - 1 if z else 0, remain)
        while z < remain and speech[at:at + z + 1] in world:
            z += 1
        if z > longest:
            longest = z
        if z >= MIN_MATCH:
            covered[at:at + z] = b"\x01" * z
    return longest, covered.count(1) / sn


class Sitting:
    __slots__ = ("seed", "speech", "tokens", "trace",
                 "model", "ignorance", "longest", "coverage")

    @property
    def model_bpb(self):
        return self.model / len(self.speech)

    @property
    def ignorance_bpb(self):
        return self.ignorance / len(self.speech)

    @property
    def ear_ok(self):
        return self.model_bpb < self.ignorance_bpb

    @property
    def copy_ok(self):
        return self.coverage < VOID_LINE

    def line(self):
        return ("seed=%d\ttokens=%d\tmodel_bits_per_byte=%.9f\t"
                "ignorance_bits_per_byte=%.9f\tlongest_match_bytes=%d\t"
                "coverage_ge32=%.9f\tear=%s\tanti_copy=%s\tsupport=PASS\tstream=%s\n"
                % (self.seed, len(self.tokens), self.model_bpb, self.ignorance_bpb,
                   self.longest, self.coverage,
                   "PASS" if self.ear_ok else "FAIL",
                   "PASS" if self.copy_ok else "FAIL",
                   "PASS" if self.ear_ok and self.copy_ok else "FAIL"))


def write_report(path, tape, world_n, args, sittings, verdict):
    """The report carries every judged stream verbatim. A speech verdict made of
    numbers alone is unlawful under the contract it is reporting on.
    """
    with open(path, "wb") as f:
        f.write("NETTA BODY 1 — INDEPENDENT EAR\n".encode())
        f.write(("world_bytes=%d\tlived_units=%d\tmerges=%d\tinventory=%d\talive=%d\t"
                 "order=%d\tcitizens_mode=none\tcorridor=%d\n"
                 % (world_n, len(tape.stream), len(tape.exp) - BASE_UNITS,
                    len(tape.exp), len(tape.alive), args.order, args.corridor)).encode())
        for s in sittings:
            f.write(("\nBEGIN RAW SPEECH seed=%d bytes=%d\n" % (s.seed, len(s.speech))).encode())
            f.write(s.speech)
            if not s.speech.endswith(b"\n"):
                f.write(b"\n")
            f.write(("END RAW SPEECH seed=%d\n" % s.seed).encode())
            f.write(s.line().encode())
        f.write(("\n%s\n" % verdict).encode())


def parse_args(argv):
    p = argparse.ArgumentParser(
        prog="netta.py",
        description="NETTA: earned speech from ordinary text, no gradients involved.")
    p.add_argument("island", help="a world of ordinary text, at least 100 bytes")
    p.add_argument("--seed", type=int, help="speak one stream from this seed")
    p.add_argument("--seeds", default="7,19,42,101,271", help="the seed set to judge")
    p.add_argument("--sittings", type=int, help="judge only the first N seeds")
    p.add_argument("--merges", type=int, default=4096, help="unit budget")
    p.add_argument("--min-pair", type=int, default=4, dest="min_pair",
                   help="a pair below this never earns a unit")
    p.add_argument("--order", type=int, default=4, choices=(3, 4), help="deepest context")
    p.add_argument("--bytes", type=int, default=700, help="speech length per sitting")
    p.add_argument("--temp", type=float, default=0.8)
    p.add_argument("--topk", type=int, default=15)
    p.add_argument("--corridor", type=int, default=1,
                   help="rails tolerated before the mouth must branch; 0 disables. "
                        "1 is the court's pinned dial, 3 is netta_mouth.c's compiled default")
    p.add_argument("--out", help="write speech_<seed>.bin and trace_<seed>.tsv here")
    p.add_argument("--report", help="write the court report, speech verbatim, here")
    p.add_argument("--time", action="store_true", dest="timed",
                   help="report wall clock from cold start to verdict")
    args = p.parse_args(argv)

    if BASE_UNITS + args.merges > (1 << PACK):
        p.error("merge budget exceeds packed id space")
    if args.min_pair < 2:
        p.error("--min-pair must be at least 2")
    if not 1 <= args.bytes <= 65000:
        p.error("--bytes outside 1..65000")
    if not (args.temp > 0.0 and math.isfinite(args.temp)):
        p.error("--temp must be finite and positive")
    if not 1 <= args.topk <= 256:
        p.error("--topk outside 1..256")
    if not 0 <= args.corridor <= 4096:
        p.error("--corridor outside 0..4096")
    return args


def main(argv=None):
    args = parse_args(argv)
    seeds = ([args.seed] if args.seed is not None
             else [int(s) for s in args.seeds.split(",")])
    if args.sittings is not None:
        seeds = seeds[:args.sittings]
    if not seeds:
        raise Refusal("no seeds to speak from")

    with open(args.island, "rb") as f:
        world = f.read()
    if len(world) < 100:
        raise Refusal("world too small")

    stream, exp = grow_units(world, args.merges, args.min_pair)
    tape = Tape(stream, exp, args.order)
    sys.stderr.write("netta: world %d B | lived stream %d units | merges %d | "
                     "V %d (avg %.2f B/unit) | order %d\n"
                     % (len(world), len(stream), len(exp) - BASE_UNITS, len(exp),
                        len(world) / len(stream), args.order))

    out = sys.stdout.buffer
    sittings = []
    for seed in seeds:
        s = Sitting()
        s.seed = seed
        s.speech, s.tokens, s.trace = speak(tape, seed, args)
        s.model, s.ignorance = ear(tape, s.tokens, args.order, args.corridor)
        s.longest, s.coverage = census(s.speech, world)
        sittings.append(s)

        out.write(("\n── seed %d ── %d bytes ──\n" % (seed, len(s.speech))).encode())
        out.write(s.speech)
        if not s.speech.endswith(b"\n"):
            out.write(b"\n")
        out.write(("ear %.6f bits/byte against ignorance %.6f | longest verbatim run "
                   "%d B | quoted %.4f of the stream | %s\n"
                   % (s.model_bpb, s.ignorance_bpb, s.longest, s.coverage,
                      "PASS" if s.ear_ok and s.copy_ok else "FAIL")).encode())

        if args.out:
            os.makedirs(args.out, exist_ok=True)
            with open(os.path.join(args.out, "speech_%d.bin" % seed), "wb") as f:
                f.write(s.speech)
            with open(os.path.join(args.out, "trace_%d.tsv" % seed), "wb") as f:
                f.write(b"index\ttoken_id\tstart_position\tbackoff\tsupport_types\t"
                        b"support_occurrences\tchosen_occurrences\texpansion_bytes\t"
                        b"advice_book_row\tadvice_factor\tcorridor\tcorridor_veto\n")
                f.write(("\n".join(s.trace) + "\n").encode())

    all_ear = all(s.ear_ok for s in sittings)
    all_copy = all(s.copy_ok for s in sittings)
    if all_ear and all_copy:
        verdict = PASS_LINE
    elif not all_ear and not all_copy:
        verdict = "SPEECH FAIL: ignorance and frozen anti-copy gates failed"
    elif not all_ear:
        verdict = "SPEECH FAIL: one or more streams did not beat honest ignorance"
    else:
        verdict = "SPEECH FAIL: one or more streams reached frozen anti-copy coverage 0.50"

    out.write(("\n%s\n" % verdict).encode())
    if args.timed:
        out.write(("netta: cold start to verdict in %.2f s | island %d B | %d sitting%s\n"
                   % (time.perf_counter() - START, len(world), len(sittings),
                      "" if len(sittings) == 1 else "s")).encode())
    out.flush()

    if args.report:
        write_report(args.report, tape, len(world), args, sittings, verdict)
    return 0 if all_ear and all_copy else 2


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Refusal as why:
        sys.stderr.write("netta: %s\n" % why)
        sys.exit(1)

# Arianna Method — github.com/ariannamethod/netta (the C organism this file mirrors)
