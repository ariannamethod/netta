# THE ORGAN COURT: THE THRESHOLD SURFACE

Preregistered before any run. The first court (UNICODE_COURT.md) ruled
by its sealed rule: the byte law HOLDS on top-1 attribution and
self-retrieval, and that verdict stands — its attribution metric is
mechanically the body-1 ablation's metric A, so that question is
adjudicated and is NOT retried here. What the first court also put on
the record, as sealed metrics 3 and 4, is the cone: Hebrew gram
diversity collapsed 10.7x and the share of fragment pairs above cosine
0.5 rose 103x under the byte law. Ranking inside the cone still finds
the source; the organs that come after the parliament — the circulation
above all — will live in resonance THRESHOLDS and MARGINS, not in
top-1 ranking. This court measures that surface and nothing else.

## Arms

L-byte against L-u8b only. The first court proved L-u8b equal to L-cp
on the whole valid corpus and bit-identical to L-byte on ASCII; two
arms suffice and the third would be a name without a difference.

## Material

The same pinned corpora as the first sitting (digests in its report:
Hebrew 521dcd214159a001 / 331c97a71ec1ec4d, English ebffbd58ecd3f825 /
63504e0eafe536e8), under the same cut law. Two rooms: the Hebrew
two-source field, and the mixed field of all four sources where the
cone may bleed across languages. English-only is the control room.

## Metrics, sealed (per room)

1. **Uncapped attribution.** The first court's attribution with the
   per-source cap REMOVED (plain top-1 over all non-origin fragments).
   In the cone the cap does rescue work that thresholds will not get;
   this measures the geometry's own discrimination.
2. **Ranking margin.** For each attribution prompt: (top-1 score minus
   median score) divided by (top-1 minus minimum), the spread a
   threshold has to work with; metric = mean over prompts. A margin
   near zero means any perturbation flips the choice.
3. **Corpse composition.** For every prompt, the unfold selection law
   exactly (k=9, cap 3): metric = mean count of distinct sources
   touched, and mean within-corpse pairwise cosine of the selected
   fragments — mush detection at the corpse level.

## Sealed predictions

- Hebrew room: L-byte shows lower uncapped attribution, thinner
  ranking margin, and higher within-corpse cosine than L-u8b.
- Mixed room: under L-byte the Hebrew cone inflates Hebrew-Hebrew
  resemblance, distorting cross-language corpse composition relative
  to L-u8b.
- English control: the two arms agree to within noise.

## Decision rule, sealed

The byte law loses if, in the Hebrew room, any of:
- uncapped-attribution(L-u8b) − uncapped-attribution(L-byte) >= 0.10;
- mean ranking margin under L-byte <= half the margin under L-u8b;
- mean distinct sources touched per corpse under L-byte is lower than
  under L-u8b by >= 0.5 of a source.

If it loses, the field law bumps to `body1-utf8-or-byte-v2` exactly as
the first court's rule would have bumped it: the grave stands, every
field replays, the derived organs recompute, both batteries rerun, and
the second hand audits the bump as its own turn. If it holds, the
parliament sits on the byte law with the cone as a named limit and
L-u8b held as the proven-cheap escape. The verdict is printed by the
machine from this rule.

## The instrument

`mycelium/organ_court.cpp` — read-only, strict-built, sharing the
first court's constant text organs and atom laws; its own gates in the
battery: English-control near-identity of the two arms, determinism,
and the cap-on/cap-off consistency row (with the cap restored, its
top-1 must reproduce the first court's attribution numbers on the same
corpora, or the instrument is not measuring the same field).

## Second-hand audit after the sitting

The sealed verdict is reproduced exactly: Hebrew margin is 0.3959 under
L-byte and 0.5397 under L-u8b (ratio 0.7335), while corpse cosine is
0.6993 against 0.3449. The formal `HOLDS` remains part of the record.

Two dead clauses named by the first hand are confirmed. Removing a
per-source cap cannot change the already chosen global top-1, and a
9-fragment corpse with cap 3 in a two-source room saturates the source
count. The remaining ranking-margin clause is live, but its normalised
quantity `(top1 - median) / (top1 - minimum)` is invariant to shifts and
positive rescaling of every score. It describes rank shape, not the
absolute threshold portability threatened by the Hebrew cosine cone.
The corpse-cosine measurement directly exposed that cone, but the
sealed rule did not give it a vote.

This narrows rather than reverses the verdict: the court answered its
registered rule, while the architectural permission attached to
`HOLDS` outran the evidence. Body 4 waits for `ROOT_COURT.md`, whose
primary metric directly tests the subword inheritance at issue.
