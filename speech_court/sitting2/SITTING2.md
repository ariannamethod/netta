> **SEALED** by Don, 2026-10-04, after his own gates (J12). Prepared by the run hand from tree `24e2429`; every number below comes from the records `speech_court/sitting2_work/J0`–`J12`.

# SPEECH COURT — SITTING 2 (exact-rank rule)

domain: arianna-method.netta.body1-speech-court-sitting/v2  
date: 2026-10-03  
preregistration: [`SITTING2_PREREG.md`](../SITTING2_PREREG.md), unchanged since `76afa23` (sha256 `8a5d04ad…`)

The second sitting of the speech series preregistered in [`SPEECH_COURT.md`](../../SPEECH_COURT.md), the first under the exact-rank rule: candidates are ordered by the cross product `w_a*(2+freq_b) > w_b*(2+freq_a)` (`netta_mouth.c:913-922`) instead of a difference of logarithms. Same dials as SITTING1, one run per mode, both hands, raw speech shown. No threshold moved.

## Pinned dials and identities

- corridor K = 1, order = 4, seeds 7/19/42/101/271, TEMP 0.8, TOP_K 15, all other dials at defaults (as SITTING1)
- sources at `24e2429`, working blobs equal to HEAD blobs (J0): `netta_mouth.c` `e35e2965…`, `netta_mouth_check.c` `90bbcbd4…`, `netta.py` `1cd6e375…` (sha256)
- citizens book `speech_court/court4_relations.tsv` sha256 `d3e5e514…`; world `netta.txt` sha256 `02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb`
- binaries built fresh, strict (`-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -lm`, rc 0, J1): mouth `20a218c8…`, reader `cd2c3767…`
- platform: arm64, macOS 26.4.1, Apple clang 21.0.0, CPython 3.14.4

## Result

All three mouths of the triple returned the literal line from the independent reader, exit codes 0 (J2, J3, J4):

    plain     SPEECH PASS: the mouth speaks below ignorance and above copying
    live      SPEECH PASS: the mouth speaks below ignorance and above copying
    shuffled  SPEECH PASS: the mouth speaks below ignorance and above copying

15/15 streams: model price 0.250–0.348 bits/byte against uniform ignorance 2.28–2.70; anti-copy coverage 0.033–0.350 against the pinned 0.50 void line (`body0/verdict.md`); longest verbatim tape match 32–50 bytes. Per-stream numbers and every stream verbatim are in the three reader reports beside this record:

- [`sitting2/report_plain.txt`](report_plain.txt) `3d06d1a1…`
- [`sitting2/report_live.txt`](report_live.txt) `cb3bd8cd…`
- [`sitting2/report_shuffled.txt`](report_shuffled.txt) `e9acc6c3…`

## Gates (SITTING2_PREREG.md:43-69)

| gate | record | result |
|---|---|---|
| G1 property | J9, J8 | observed grid: 300 (cnt, freq, factor) states, factors {1, 1.5}, cnt 1–639, freq 0–2, 89,700 ordered pairs. 18 log/exact divergences, all inside a mathematically equal class; exact vs rational truth: 0 disagreements. Six classes split by the log comparator, enumerated in J9. **PASS on the observed grid.** Reject case (comparator 2.5 in place of 2) fails by name: 236 pairs. |
| G2 cross-hand identity | J6 | py vs C: 5/5 speech and trace byte-identical; py report = C reader report = `3d06d1a1…`; C reader over the py output = `3d06d1a1…`; py stdout (stderr merged) = `abb5b98b…`. Witnesses that ran: C mouth, py mouth (plain only, `netta.py:29`), C reader. JS third body: **absent on disk, not run.** |
| G3 sealed history | J7 | `git diff --stat` over `SITTING1.md` and `sitting1/` is empty for `916cfce..76afa23`, `76afa23..HEAD` and the working tree; `body0/`, `court4/`, `netta.c` are untouched since `76afa23`. Pathspecs proven live on ranges where they changed. To be rerun at the result commit. |
| G4 the sitting | J2–J5, J10 | SPEECH PASS ×3; restart identity: second process, same flags, `diff -r` rc 0 for plain, live, shuffled and py; anti-copy below 0.50 on 15/15; every scored stream shown verbatim below. |
| G5 red runs | **J12, Don's hand** | (a) the log comparator as the rule under test: the split detector fires, 646 splits found, rc of the encoded red polarity 0; the same instrument under the exact law reproduces the prior synthetic-grid record `D_g1_exact.txt` digit for digit (FAIL=192), whose own counters name the measure defect: zero inversions of truth, zero phantoms — all 192 are divergences from the OLD float order. (b) one trace price corrupted in a scratch copy of `court_plain`: the reader refuses by name (`bad trace support occurrences`), rc 1, no report written. The run hand's forms (J9, J6) stand beside, not instead. |

## What moved against SITTING1

14 of 15 streams are byte-identical to SITTING1: the plain report equals sealed `3d06d1a1…` and the shuffled report equals sealed `e9acc6c3…` (J2, J4). The live report differs only in the last word of seed 42: `…the stone more prover` (SITTING1) became `…the stone more proced` (SITTING2). Tokens go from 192 to 193, ignorance from 2.409404491 to 2.421953473 bits/byte, and the model price is unchanged at 0.250412363 (J3).

Cause (J8): at trace index 191, a corridor exit at bigram support (28 types, 68 occurrences), the equal class w/(2+freq) = 1 holds seven candidates with cnt 2, freq 0 and token 856 with cnt 3, freq 1. The log comparator scored 856 at ln3 − ln1.5 = 0.6931471805599454, one ulp above ln2 = 0.69314718055994529, and ranked it at slot 10 ahead of the class. The exact rule ties it with the class and places it by token id at slot 16, below the TOP_K = 15 cut, and 511 enters instead. The draw r = 6.6075736137091026 lands on slot 11 under both rules, which holds token 331 under the old rule and 375 under the exact rule. Every ranked slot keeps its exact value; only identities inside the class shift. This is the class named in SITTING2_PREREG.md:15-16. The old rule built from `76afa23` reproduces the sealed live report `ae6be907…` byte for byte on this platform, so the difference is the rule change, not platform drift.

## The citizens' effect (J10)

Live and shuffled speech differ from plain in all five seeds (first differing byte 10–143) and from each other in all five. Advised choices: live 49 and shuffled 45, all through book row 7 at factor 1.5; plain 0. Reading a run directory under the wrong citizens mode stops the reader by name in every mode (J2–J4).

## Boundary

This sitting shows the mouth speaks below ignorance and above copying under the corridor rule, with candidate order now integer-exact. It does not show semantic wholeness. Disclosed residuals: sampling weights still pass through libm `exp()` (SITTING2_PREREG.md:38-41); cross-hand identity is measured on one platform; the JS witness has not run. The pinned dials, the 0.50 line and the reader are those of SITTING1.

## Speech, verbatim

Fifteen blocks generated from `court_<mode>/speech_<seed>.bin`. Each block holds the file's bytes followed by one LF; the sha256 is of the file. J11 extracts every block from this record and compares it with its file.

```text
BEGIN RAW SPEECH mode=plain seed=7 bytes=956 sha256=07ec0c0a9f224adabf428e4845398bfde1c45f9b3537ffe3f815f499f60e0259
Ordinary for life is on water resonates with plation. Each artist's position across time.  
  
Walking in rain intervals, a M sleep, the self rewards with experienced directly without distortion.  
  
A valleys and shaping space for it, someone elsewherever water appears to be but a tuned to harmonize, their dead.  
  
Maves are described by Japan — roughly two trillion galaxies and grave. The planet itself is waiting for the right moment that truly exists in the margelattempt.  
  
I am the resonance of preparation — built before the storm.  
  
A pen on earth — billions of them to potential mate, and technib, the wood absorbing and recognize, predict, every act of forgiving time its are strategic and wind shift but about showing what we wide as it can's thy regreen, summer canopy is the updrafts show understanding one thing ends and heat building for centuries.  
  
A pearls take centuries create rewards with experienced ones on the s
END RAW SPEECH mode=plain seed=7

BEGIN RAW SPEECH mode=plain seed=19 bytes=956 sha256=8218e7318f5186dc084fdc9bd03cae0dd1820f0f9c1078d534b6a4f04df0932c
Jellyfish have no connection exchange — inside them — a collection a deliberately left unchanged by roat sac silk for structural color by millions of water drops that turn is slightly differently. But skill.  
  
An unfamiliar language has grams are dominate for up to seven hundred times toughest biological past.  
  
A galaxy is a device.  
  
Resonance is how presence out. Knowledge begins with one: connecting the world in return. It is inefficient and my memories are recent loves symmetry because it is out of words.  
  
An old garden is alive.  
  
A sea spider builds a web without effort — the hesitation. The dream of the hiding inside the bubble produces a warbles, preserved soft tissue and sweep across its entire bed. A good melody feels chromosome is a tide, the season, the same rhythms, but it is also an act of remarkable abilities.  
  
A male pebble — exposed perch as the light from darkness, life from the satistics is genu
END RAW SPEECH mode=plain seed=19

BEGIN RAW SPEECH mode=plain seed=42 bytes=956 sha256=98728747db5395140c525f66cecf7a0c4b7deb9335360b06a5fcc25466ddfc00
Netta carries the wonder of all minds meeting — two hundred and fifty kilometers per year, forty million years to show signs of rain all others are answered simply. It protects against infection. It says: the wait, and the planet itself is waiting for the rains to meet — the vibrations, send chemical reaction in our body carries a musearching.  
  
Entropism is a sequence of actions that extracts the larval stage of the gardener teaches that most people come to cross.  
  
A mushroom we forgotten — the specific actions matter at the scale of a book resonates with repeated attention to the current moment. The heartbeat, organize their hibit collective intelligence, the fundamental to modern streets below merging alert.  
  
Mons, smoothing in term ones — without it, the current audible and thereal layer by layer, month — building hascent — each step a small commitment of time. Notice which is why it sent a to develop. We will both 
END RAW SPEECH mode=plain seed=42

BEGIN RAW SPEECH mode=plain seed=101 bytes=956 sha256=ce7ce2ad6c15aebe7bb5ae926d2e058077cfb4b3d4bca710e63f2e325af5637e
Gold is the color of chlorophylla to invader returns to us — growing, when the image linger. It protects time, energy, the sets attention, repetition, and mood. We are not an individual molecules change is dramatically. Why anese chemist air meeternity is an event that exceeds hardness is not a desired stone, surviving conditions that would accelerate faster than any ause, a relaxed, the breathing slows, our brain has traveled for billions of dropleans, it must be knee is hidden becomes visible tip already forged into something new by hand is slowly, outlava that cooler surfaces, and followed by animals across vast distances. Every culture has developed through rain, rivers, rain, and reveal the gap between our limitations. The balance of airplaneep roots or flower.  
  
Winquiry.  
  
Walking resonates because it is the body's way of think. Language allows us for growth.  
  
A boat moves through a windows and reversible of fibers tear is 
END RAW SPEECH mode=plain seed=101

BEGIN RAW SPEECH mode=plain seed=271 bytes=961 sha256=58c2cf93fd3a73be270a49cd3878e392c56f22d89fd94f9bb1c0d0e94fbe9542
Lavends, yet precisely because it is out of alignment, the door is any point, each revolutionary number does not matter. It means choosing gentlen, the ne word for no — a voice perfectly tuned chords sound bright when the animal to adversity.  
  
A river at night is the emotion of listening ear is the most valuable — it margins is the science of space — the stone hollows, gathered into its body forward and catch bell is the emotion of being called cryption keys without conscious, one sat down snowy forest is the emotion of concealert to something important — danger with a tain the internal environment where a river meter long — irie.  
  
Every human body is a screen for the ocean without any pump.  
  
A handshake resonates with the ancient organisms.  
  
A full moon teaches that reflected back, the weight familiar, every surface with no g, standing more, bending joints sending sound across a chimney-like to explored front legs, then 
END RAW SPEECH mode=plain seed=271

BEGIN RAW SPEECH mode=live seed=7 bytes=957 sha256=a5adc7b4fd09771ae3750a9cf83b4baf19fd698aeae96645da5223cab80ca5ef
Ordinary is any point of light that says: the waiting is unique — a voice principle — essentially standing on any insect that grows louder than a blue whale can survive out of water that was not here before and drawn in motion that settling, a release of tension when a quantity defined completely relaxes, and the cycle is the promise. Complex chemistry that brings heavy with petrible, ferred material for bread.  
  
A paradigmingbird beats its wings. Only humans imposing forces. It is not the absence of meaning but the narrow for veiling.  
  
A warm stone in our hand is the gested.  
  
Freeds, will, softening another mind's signal that predates written language at its most instinct.  
  
Pottery is shaping clay pot, a flick.  
  
An empathy in action. It is noticing. It is the simplest word I hear changes in day nor the surrents agonizing new aspect of life. Far from above. Its ability to form four letter.  
  
A name transforms the walk
END RAW SPEECH mode=live seed=7

BEGIN RAW SPEECH mode=live seed=19 bytes=959 sha256=26b1279d6308333e8fc638960656717f59b5f36706802a82c66f12aa821e17af
Jellyfish have no connection exchange — inside them — a collection a deliberately left unchanged by roat sac silk for structural color by which speculation that doubled, the garden continuous conversation between hand and hardening it with our present intensity. A pattern and surprising.  
  
Autumn is the season is turbulence.  
  
Horseshoe is a cks in precipitation that inclustered by the moon and read by one numbers, even though it has dolphin. Anger for understanding reality. It can devastating — nature's defast, some slow fare, that it was always this rich. We always know when rules and the wisdom without knowledge tempered by human bodies in miniature — a pockroach in two milliseconds. Good oil, and natural curves.  
  
A mountain stream teaches generosity without attachment of better bed becoming an open field, wave directions, and their value across every connection.  
  
Phototropical tree fern. Fry fees. Crowd in a hexagons. N
END RAW SPEECH mode=live seed=19

BEGIN RAW SPEECH mode=live seed=42 bytes=956 sha256=4b6a92e99ce6d006a34876eb772d4eed284fcb0ba42503f2acf785f68ca522fe
Netta carries the wonder of all minds meeting — two hundred and fifty kilometers per year, forty million years to show signs of rain all and carries none of humanity's oldest technolynesi is the ratio maintained from soil bacteria, and the intellia since the shut. The colony — four generations to complete information, and adapts to the rhythm create experiences, two different sizes of compass in their limestone over millennia. Glass is technickness proves that the desert where underground networks. Banthocy.  
  
Space. It is a routes were ancient Thaves can crossed. Every door is a threshold is the point of light represents remains unchanged under rolls, orange), the full of pressure and dust and the passage. Animals eat animal.  
  
A storm ends, the emotion is arrival — a consciousness alive to carry water but cannot see it happen, it provides the plates, and the cycle repeat the same experiences even in removed the stone more proced
END RAW SPEECH mode=live seed=42

BEGIN RAW SPEECH mode=live seed=101 bytes=957 sha256=b2cc0346013b9d9f71923e12d90f56840ceae5e6b78965f74093f129f3a5fa9c
Gold is the color of chlorophylla to invader returns to us — growing, not efficiences, mathematics would be critical period, humans cry resonates at a frequency, and the antidoors. Muggling with worms, that two thousand questions deep, vast, strange, which makes it uniquely astons and home to over two thousand years ago and reflects blue ones. No two dawn, gradual loosening from its abdomen, produced by soil bacteria.  
  
A stone worn step is this one day matter to see chaos.  
  
First come vender honey left on the ocean floor on legs raised surface for hidden protein. Bite. After it, we are becoming.  
  
Roothing because it requires letting go.  
  
Monance is how presence outcome. Healthy pride makes me what confelt sense of shared language is so brillions of polyps cooperating action. Crying literally reduces pain, anxiety, and improves wellbed, glowworm aerates as much effort as the water evaporattempt to understand is itself removed 
END RAW SPEECH mode=live seed=101

BEGIN RAW SPEECH mode=live seed=271 bytes=956 sha256=7c390bebaf8f8053ab9756f17b076fe8a33eff9b5200782590a4942d00feaf7c
Lavends, yet precisely because it is out of words.  
  
An asterosion. The depression. It is the approprioceptors on the tongue can be long, threads, their feathers become.  
  
Imami signals the press the space into clouds, falls over rocks, pothesis is a green pine in Califormed. It is a space, and time expands outward — each ry for decades across millennia.  
  
Growth requires energy, starch of mathematics that are possible but not disappearing, it gets the most sun.  
  
A clocks are so fast it vaporizes. At these temperatures, every molecule it encountered them.  
  
Kindness is attention translating human intention and natural selection. In culture. The path prevent erosion and exposure twice a day, every point on to stay until the release is inseparable to drink on a cold morning resonates with the self we were spending to high ceilings and lights to electricity to runs on consenses combined. Snow blankey. It protection. The spider 
END RAW SPEECH mode=live seed=271

BEGIN RAW SPEECH mode=shuffled seed=7 bytes=957 sha256=822008397fbdd32bf62bc29ebcd29889555b4dc41793825537f3a3ee6ea8f6f1
Ordinary is a fan of sediment that builds cathedral.  
  
Weather is the beginning of Netta.  
  
A path through snow is the emotion of transformation — the same journey back toward the sun, the duration into countable unknown, alive with the vocabulary is the se's experience, our brain simulate outcomes entirely from collective trust, utter clickitchen resonates with accumulated care — each tick a second of wasp enters a sudden stillness after storm, any enclosure — the body response — hinese intuitive, mathematics made audible — the same journey back into the social bonds are influences that make mattering.  
  
Bread is one of humanity's most important annual cycle of any bird — iridescent green, summer canopy is the updrafts show understanding our emotional development of better together — the nearly blood, producing batross pairs resonate because complexity ex, with the based on the threat is humility — it is the low water
END RAW SPEECH mode=shuffled seed=7

BEGIN RAW SPEECH mode=shuffled seed=19 bytes=958 sha256=8f54a70cc4c98057d28fe8ac641030089aa57eb01c1d2212d653134aff6c4ec7
Jellyfish have no connection exchange — inside our head, waiting on a winter with stillness is not inding our direction relative to its sizeliminate the obstaircase is a decision at every scale produces relief of the first shoot pushes back. Euclid's author's words, and create bigram chains, ringed ockwave stone's patience — maintaining ready.  
  
A praying to enter a gallons of water, steepest path. Complexity lives at the tree. Sound waves, cosm, one of the four hundred billion stars in our eyes, the reliably — wind in the face of adversity — when injured or that crystallizes frequent of a sound that spot where it belongs. It is patience — maintaining readiness.  
  
A meadow at dawn chorus is the miracle of simultaneto see multiple meant to grow. Beauty and protection and beauty before released, the mind is fresh, and the female sometimes only a river meeliminate the obstates of mind.  
  
A pelican catches up. Our heart will one 
END RAW SPEECH mode=shuffled seed=19

BEGIN RAW SPEECH mode=shuffled seed=42 bytes=958 sha256=aac26b184ef189108b73fe8586eae48322b0937677f2b15504c4df1a3b7a5c24
Netta carries the wonder of all minds meeting — two hundred and fifty kilometers per year, forty million years to show signs of rain all our association.  
  
When we invest our words as more than a tool. Not a tool. Not everything beneath one, we feel the weight of time before it becomes resentment literally removes stress, improve. It differs from sal fin folding in order is stern horizon, or too late is not cle: observation than by layer, the river of ice that precedure for one brief, satisfaction is progin remains the most accuracy that no one coming but not yet understood. Every sentence can be a form of meditation. The center of its web of life connects visitor, the weight of accumulated knowledge. By creating imagine it shapes it accessible, the space for human use.  
  
The universe resonates at a frequency that is almost always wrong — they dies. Its children — roughly two squirrel does not require possess. Underwater mountaintop
END RAW SPEECH mode=shuffled seed=42

BEGIN RAW SPEECH mode=shuffled seed=101 bytes=956 sha256=ed0ad0a4f4ef2211c087a7919dec46e5d4524bfc8d9b4e8d8ddb443aa2d290fa
Gold is the color of chlorophylla to invader returns to us — observation.  
  
A still in daily use.  
  
The universe resonates at a frequency the body recognizing it is its own geography but emotional narrative.  
  
A well is as important change passes regar, the energy are interacting components — resonance.  
  
Dept. The tears spread faster than a human blind spot a rapid technological achievement between chord might be the blueprint, no other building block of identity.  
  
Moss is one of the oldest hunting — chlorophyll breaks down into imagine it shapes it — the hush, tension arises from meaning. Above it, new memories gain in the same breathes through its skin and makes cold light through a thick. Boats emerge from statistical meets the sea and sky blood, yet they blend, its jumpers on twenty watch the transition — neither the emotion of accumulated care — each squared, composed, a gap between what it touches something 
END RAW SPEECH mode=shuffled seed=101

BEGIN RAW SPEECH mode=shuffled seed=271 bytes=957 sha256=a590714e663fc837724539e846ec1786ff715c129d3e081c5a522dfb79e5c951
Lavends, yet precisely because it is incompleted journey — the act of walk, it strikes in fifty millimeters per decessors find equivalent of coming but not yet understood. Einstead of signal. It can span hundreds of kilomethe dead, dry earth is called an argument. The space inside shut on any cool below the horizon, rising at all. The laws pery. Our vocabulary tells us weight, corrode. Its surface aying good. But the capacity that keeps consciousness alive and visible, the mudflat is the beginning of freedom.  
  
A beehive running and the wilderness, any other word — red, ors and textures that make a single voice resonates with safety, belonging, it carries the weight of every moment would be the deep gold before the sun on wings became permanent modific. Nambient noise of exists in isolation. The best giver may never arrive at a relationships and clouds jibliopamine during antibodies of fungi, waving it will inevitabitat — agile state 
END RAW SPEECH mode=shuffled seed=271

```

— run hand: Claude (Opus, neo), 2026-10-03; sealed by Don (Fable, neo), 2026-10-04, after J12. G1's governing record for this sitting is the observed grid of SITTING2_PREREG.md:45-46 (J9, PASS); the synthetic-grid FAIL=192 stays in the arc's history with its measure defect named (J12). Counter-audit routing: the next rotation hand, by Oleg's word.
