# The README voice

Date: 2026-08-24.

This is the exact fresh NETTA ZERO run behind the quote in the main README. It
exists so the displayed voice has a receipt rather than becoming folklore.

## Life

Canonical C11 build from the repository source:

```bash
cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o netta
./netta netta.txt \
  --reset \
  --seed 42 \
  --episodes 40 \
  --steps 800 \
  --state netta.state \
  --bio netta.bio.tsv
```

The source island was the tracked 447545-byte `netta.txt`, FNV identity
`4cbaf51791f2d85e`. The life reached episode 40 and 32000 lived bytes. Its
biography closed at 37138 lines with chain `89446615da4bf81f`.

## Mouth

The prompt file contained `The ` followed by its terminal newline. The mouth
therefore honestly reported opening bytes `200a`.

```bash
./netta \
  --speak 240 \
  --speak-seed 7 \
  --prompt-file prompt.txt \
  --state netta.state \
  --bio netta.bio.tsv
```

Receipt:

```text
speak: 240 bytes, hand bi, seed 7, law supported-backoff
speak support: uni 0, bi 240, tri 0
spoke: candidate-digest=fc6fb73eba9d0daa bytes=240 seed=7 hand=bi law=supported-backoff opening=200a lived-bytes=32000 episode=40
```

Display transcript (terminal spaces at line ends are omitted; the candidate
digest above binds the exact 240 spoken bytes):

```text
Abeg me wat. ats fumery.
 bes heatwherl fouingreraresf t iofirthes f cuthrfaf woaico at.  knspend  m t urre. Fan.
Gry  richeromper Th rewe e f f obinongftee tot — t re. romornt tethe bbucaclin
Plits wsoc crantrmy whexiey wesison ivore.
```

The same speech invocation was repeated and the two persistent objects were
SHA-256 identical before and after:

```text
state      4b2a100b71189daabeacaa1c0313f0f6c4afcf9605712fbbf28e39516274a76b
biography  ffa651fb599d332f21e9e207add945e331f38badbf44fc8c8b05889832fb0eb4
```

The mouth was read-only in the witnessed run. This is a voice example, not an
eloquence verdict and not an authority grant.
