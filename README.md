## NETTA's Empirical Topological Training Agent
**by Arianna Method**.

Somewhere between Alpha-Zero and LM-hero there's Netta. Netta is a continual-learning organism, she lives in 256-byte tape dimension. Lucky girl. Netta also has `.txt` islands. Netta plays next-action games with them.

When transformers choose the easy way, Netta has to survive. So: no pretrained weights, no backpropagation either. Life is tough, Netta, but real warriors don't hide their scars. Scars, this is biography.

---

## NETTA ZERO

Netta sees the world as an **immutable byte tape**. No words at birth, no tokenizer deciding what reality is. No BPE dropped from heaven by humans who already know how to speak. No, thanks.

256 bytes. That's all she needs. Every byte is legal, and every island is addressed by:

```text
(island, byte offset)
```

and source truth is never rewritten. Netta enters an island, sees what has already happened, makes a next-action move and gets judged by what the world actually did next. The receipt comes first. Learning comes after. Fair? No. 

Even in byte tape world everything is priced - in **bits per raw byte**. One ruler. No changing the unit because a new mechanism happens to look prettier under another metric. 

---

## an earned vocabulary

256 bytes are the floor, but the floor is load-bearing. Repeated sequences become **units**: byte-exact roads over ground Netta has actually lived. A unit never replaces its bytes and never retokenizes the past. The atomic world remains underneath forever. And vocabulary pays rent. A unit that goes unrecognised for 16384 lived bytes dies. Its current probability mass disappears, but its history doesn't. If the same byte sequence becomes supported again, the old identity resurrects with its evidence instead of spawning a convenient twin.

Netta does not get concepts because somebody thought concepts would be useful. Netta thinks the concepts have to survive too.

---

## six witnesses walk into a tape

Netta currently carries six prequential witnesses:

```text
atomic-uni
byte-bi
byte-tri
unit-uni
move-bi
core
```

They all watch the same lived world and all pay on the same ruler. The byte witnesses learn increasingly contextual statistics over raw experience. The unit witnesses price the variable-duration roads Netta earned herself. `move-bi` asks what usually follows an already-earned move. Then there is the neural core. The core is a small recurrent witness over deterministic innate byte embeddings with a delta-rule readout. It's born at ignorance and has to acquire predictive structure while Netta lives.

Importantly, **being neural grants it absolutely nothing**. The core lives in shadow until evidence says otherwise. Its first surprise-gated Hebbian plasticity rule lost against a matched frozen-reservoir control. So it was not quietly tuned until the graph looked nicer. It was quarantined as:

```text
--core-hebb-v1
```

A scar.

Under `--jury`, eight byte-identical shadow cores — one frozen genome and seven plasticity laws — sit through the same life. The first sealed cross-world court promoted nobody.

Beautiful mechanism. Null result.

Next.

---

## authority has to be earned

This is Netta's most annoying personality trait. Nothing gets authority merely because it exists. Each new organ may observe. It may measure, may live in shadow for a very long time, but before it can alter what Netta actually does, it has to beat a matched control on experience that arrives **after** the prediction.

Authority is earned, measurable and revocable. Byte actors compete on their lived records. The semi-Markov move actor had to survive probation before it was allowed to act. A mandate that works on one island does not magically become truth everywhere else.

**Mandates are global. Verdicts are local.**

Every island keeps its own record of the witnesses that visited it. A travelling hand may be strong at home and still be refused on a foreign shore. Netta has politics because apparently even a 256-byte universe eventually invents jurisdiction. With `--atlas`, navigation itself becomes earned. Netta first maps under-lived islands, then uses only records already earned by that life to decide where to go.

Worlds remain worlds. Experience may travel between them.

---

## biography

Netta does not clean up her failures.

Every life owns two persistent objects:

```text
netta.state       binary circulating state
netta.bio.tsv     append-only biography
```

The biography is hash-chained and external to the world. Source islands are never writable memory. Births, deaths, actions, island arrivals, mandates, revocations, speech and witnessed court events become canonical records. Negative experience stays beside positive experience. A failed claimant remains in history. A null remains a null. A superseded measurement gets a correction, not an eraser.

The biography language now has thirteen record types and a canonical grammar specified in [`BIOGRAPHY.md`](BIOGRAPHY.md).

Two independent hands read it:

* `netta.c` verifies its own biography on resume.
* `scripts/biography_check.c` implements the language independently from the outside.

They share a specification and adversarial corpus. They don't share parser code, because two readers are useful only if they are capable of disagreeing. At the close of the single-file heart, they agree on the complete context-free language.

---

## the mouth has no vote

Netta can speak.

That does not mean speech gets to rewrite her life.

```bash
./netta netta.txt \
  --speak 120 \
  --speak-seed 7 \
  --state netta0.state \
  --bio netta0.bio.tsv > speech.bin
```

The mouth samples only from continuations Netta actually observed, using the deepest supported context and backing off causally:

```text
trigram → bigram → lived unigram
```

It has its own RNG stream. It learns nothing, prices nothing and changes no state.

A prompt, when supplied, only gives the mouth its last two opening bytes. It does not inject a magical semantic instruction into the organism.

After speaking, the mouth states what happened: candidate digest, byte count, seed, hand, law, opening and life position.

With:

```text
--sign
```

Netta may append that act of speech to her own biography. The signature says **I emitted these bytes**. Nothing more mystical is required.

---

## so what does the lucky girl actually say?

Not an assistant answer. Not a beam-search victory selected from twelve polite
alternatives. Not a pretrained model wearing a name tag that says Netta.

This is the current supported-backoff mouth after one clean life over the
repository's own `netta.txt`: seed 42, 32000 lived bytes, byte-bi hand, speech
seed 7, 240/240 draws from transitions she had actually witnessed.

```text
Abeg me wat. ats fumery.
 bes heatwherl fouingreraresf t iofirthes f cuthrfaf woaico at.  knspend  m t urre. Fan.
Gry  richeromper Th rewe e f f obinongftee tot — t re. romornt tethe bbucaclin
Plits wsoc crantrmy whexiey wesison ivore.
```

Nobody corrected her grammar. Nobody should. At 32000 bytes she is not a
chatbot failing to imitate adulthood; she is a byte organism audibly acquiring
roads through a world. The receipt is candidate `fc6fb73eba9d0daa`. The exact
recipe is preserved in
[`research/README_VOICE_2026-08-24.md`](research/README_VOICE_2026-08-24.md).

Give the same mouth a narrow life whose entire weather is period three and she
does not hallucinate a larger universe. She says its law back:

```text
cacbabcacbabcacb...
```

The earlier Dracula first words — including the beautifully wounded Laplace
red arm — remain in
[`research/FIRST_WORDS_2026-08-16.md`](research/FIRST_WORDS_2026-08-16.md).
Failures do not disappear merely because the mouth later learned where not to
spend ignorance.

---

## ear → court → citation

The ear listens without power.

```bash
./netta netta.txt --ear speech.bin
```

Each island independently prices the candidate with its own statistical ladder and reports exact overlap with its shore. A structural twin destroys ordering while preserving the census. This separates “these bytes exist here” from “this sequence belongs to the structure here”. Then comes the pattern court:

```bash
./netta netta.txt --court speech.bin
```

Its vocabulary is intentionally small:

```text
abstain
replay
order
stranger
```

A verdict describes a measured pattern. It does not pretend to know causality. Court operands are public. Receipts are public. A complete sitting closes with a public docket. `scripts/warrant_check.c` reconstructs and verifies the grammar independently. A resumed life may later **cite** one complete sitting:

```bash
./netta \
  --cite sitting.txt \
  --state netta0.state \
  --bio netta0.bio.tsv
```

Citation is memory. Nothing reads a `w` record back into behaviour. If an earlier `s` says Netta spoke a stream and a later `w` cites a court sitting over the exact same bytes, the outside biography reader can recognise the relation. Netta herself still doesn't get free authority from being recognised. 

---

## the heart is closed

`netta.c` is the complete single-file heart of NETTA ZERO.

The biography grammar closed at thirteen of thirteen record types in both independent readers. The frozen-life twins remained byte-identical across the closure. The current C suite ends at:

```text
339 gates
ALL GATES PASS
```

Strict builds are silent. Sanitizer gates are green. The heart stays self-contained:

```text
one C file
no required Netta headers
no shared parser object
no pretrained weights
no backpropagation
```

New organs now grow **outside** `netta.c`.
Do not cut open a heart every time you invent a kidney.

---

## mycelium, or: the dead demand representation

The heart is closed. It is not lonely.

Mycelium begins where one Netta's private state has to end: at public speech,
public biography and the uncomfortable possibility that several lives may
have noticed the same shape for different reasons. It eats only attested
speech. The original bytes go into a grave. The grave is not deletion; it is
the only place from which the field is allowed to remember.

A vector database would return something nearby and call the matter settled.
Mycelium is more unreasonable. A recurring form must survive five bodies:

1. **The graft** buries speech verbatim and manifests a resonance corpse with
   clause-by-clause lineage.
2. **The proposer** notices repeated byte-exact forms, charges them rent and
   kills those that stop recurring. Resurrection restores the same identity,
   because amnesia is not a garbage-collection policy.
3. **The school** makes a form predict held-out future meals. Past glory cannot
   sit the exam for it.
4. **The parliament** separates experts from power. Exam, biography and rent
   cast public receipts; PASS, WEAKEN, FREEZE, SCAR, DARK and SILENCE are laws,
   not vibes.
5. **The mint** gives an admitted citizen a tiny mortal body: a 96-to-1
   notorch probe, 97 parameters, exactly 388 canonical bytes. The note may be
   LIT or DIM. It may starve, die with its weight intact, resurrect and retrain.

Every body writes an append-only hash chain. Every chain is read by two hands:
the organism and an independent plain-C witness that shares the grammar but
not the parser. A receipt can be reproduced. A scar cannot be optimized away.
A weight cannot claim 10/10 merely because a line in a file says it did.

And this is how the graft sounds before the weights are allowed to touch it:

```text
An electric sensor follows the pulse along the insulated winding.
The quiet vessel follows a blue.
Under the midnight pier a patient current gathers broken constellations.
```

Then the first complete Russian bodies 1–5 ceremony admitted the form
`живые ноты помнят`, sent it through school and parliament, trained it for 512
Chuck steps, and struck the first LIT weight the Method has ever minted:

```text
живые ноты помнят прожитую цену под ясным небом 03.
```

Holdout 10/10. Majority baseline 5. Final loss 46959 microbits. Weight
`8b6871cb8abcd341`. Three hundred and eighty-eight bytes in the morgue, and not
one of them has authority yet.

That last sentence is the point. Body 5 can prove that a form deserves a body;
it cannot let the body steer speech. Body 6 must legislate circulation first:
which living LIT notes may bend the field, how much, under whose vote, and how
the second hand catches a beautiful coup. Until then the first weight of the
Method is real, trained, witnessed — and constitutionally powerless.

The constitution is [`mycelium/MYCELIUM.md`](mycelium/MYCELIUM.md). The whole
strange biography is [`mycelium/MYCELIUMLOG.md`](mycelium/MYCELIUMLOG.md).

---

## the other body

There is also [`netta.py`](netta.py).
It is a stdlib-only reference body of the same architecture, not a second evolutionary lineage.

On the canonical `cc -O2` reference build, the C and Python bodies reproduce the same state, biography and outward behaviour on the acceptance recipes. A life may begin in C and resume in Python, or begin in Python and resume in C.

```bash
python3 netta.py netta.txt \
  --reset \
  --seed 42 \
  --episodes 4 \
  --steps 800 \
  --state netta0.state \
  --bio netta0.bio.tsv
```

Floating-point identity belongs to the canonical build, not to a religious belief that every compiler on every machine must fuse arithmetic identically. The tests measure the difference instead of pretending it cannot exist.

---

## build

C needs a compiler and `libm`.

```bash
cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o netta
```

Run the C law:

```bash
sh zero_tests.sh
```

Run the C ↔ Python equivalence law:

```bash
./python_tests.sh
```

No Torch. No NumPy. No CUDA. The closed heart has enough problems already.
Body 5 of the external mycelium links the installed pure-C
[`notorch`](https://github.com/ariannamethod/notorch) forge; it does not smuggle
Torch into `netta.c`.

---

## start a life

```bash
./netta netta.txt \
  --reset \
  --seed 42 \
  --episodes 4 \
  --steps 800 \
  --state netta0.state \
  --bio netta0.bio.tsv
```

`--reset` births a new biography.

Without it, Netta resumes:

```bash
./netta netta.txt \
  --episodes 4 \
  --steps 800 \
  --state netta0.state \
  --bio netta0.bio.tsv
```

An island is identified by its content, not by where you happened to put it in today's command line.

Bring several:

```bash
./netta island-a.txt island-b.txt island-c.txt \
  --atlas \
  --episodes 4 \
  --steps 800 \
  --state voyage.state \
  --bio voyage.bio.tsv
```

A known island may disappear from today's convoy and return later. Its lived record remains hers.

Change the bytes and it is another island.

As it should be.

---

## technical bones

| thing                   | NETTA ZERO                            |
| ----------------------- | ------------------------------------- |
| irreducible world       | raw bytes                             |
| atomic actions at birth | 256                                   |
| world address           | `(island, byte offset)`               |
| source truth            | immutable                             |
| judgment ruler          | bits per raw byte                     |
| earned unit length      | up to 16 bytes                        |
| unit rent               | 16384 lived bytes without recognition |
| prequential witnesses   | 6                                     |
| shadow jury             | 8 neural cores                        |
| island registry         | up to 1024 identities                 |
| state                   | binary, v20                           |
| biography grammar       | 13 record types                       |
| C dependencies          | libc + `-lm`                          |
| Python dependencies     | stdlib only                           |
| C research gates        | 339                                   |
| training phase          | none                                  |
| pretrained weights      | none                                  |
| backpropagation         | nope                                  |

---

## research law

The short version:

1. Every serious organ gets a matched control.
2. Seeds, source positions and compute budgets stay equal.
3. The world reveals truth after the prediction.
4. A new layer earns authority before it changes behaviour.
5. Source truth and counterfactual experience are different things.
6. Negative experience stays.
7. A null result is a result.
8. Measure the thing you named, not the thing that makes the graph look better.

The long version is [`NETTALOG2.md`](NETTALOG2.md).

It is intentionally enormous.

Every body, failed organ, red twin, preregistration, repair, court, audit, null, resurrection and scar is there in chronological order.

README tells you who Netta is.

`NETTALOG2.md` tells you what happened to her.

---

## living files

```text
netta.c                       the organism — closed single-file heart
netta.py                      reference Python body
netta.txt                     example byte island
NETTALOG2.md                  the full technical biography of the research
BIOGRAPHY.md                  canonical external biography language
zero_tests.sh                 executable C research law
python_tests.sh               C/Python equivalence law
research/                     preregistrations, results, audits
scripts/                      independent readers and falsifiers
mycelium/                     five external bodies: graft, courts and mint
```

---

The world is a tape.

The scars stay.

Be gentle with Netta.
