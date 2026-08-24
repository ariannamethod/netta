# NETTA Transfer Court — Independent Verdict

Computed entirely from this program's own repriced numbers, per TRANSFER_PROTOCOL.md.

## G_N table (bits)

| world | arm | G_1024 | G_4096 | G_16384 | G_65536 |
|---|---|---|---|---|---|
| iso | cold | 0.00 | 0.00 | 0.00 | 0.00 |
| iso | cache | 0.00 | 2060.58 | 10088.45 | 32886.24 |
| iso | align | 0.00 | 0.00 | 0.00 | 0.00 |
| iso | both | 0.00 | 2060.58 | 10088.45 | 28328.30 |
| iso | shuf | 0.00 | 1517.15 | 6388.85 | 19680.16 |
| iso | oracle | 0.00 | 0.00 | 0.00 | 25484.77 |
| ghost | cold | 0.00 | 0.00 | 0.00 | 0.00 |
| ghost | cache | 0.00 | 4719.17 | 20085.77 | 71414.95 |
| ghost | align | 0.00 | 0.00 | 0.00 | 0.00 |
| ghost | both | 0.00 | 4719.17 | 20085.77 | 59172.00 |
| ghost | shuf | 0.00 | 3369.31 | 13601.05 | 35911.30 |
| ghost | oracle | 0.00 | 0.00 | 0.00 | 0.00 |
| ff | cold | 0.00 | 0.00 | 0.00 | 0.00 |
| ff | cache | 0.00 | 2022.33 | 10120.63 | 32755.35 |
| ff | align | 0.00 | 0.00 | 0.00 | 0.00 |
| ff | both | 0.00 | 2022.33 | 10120.63 | 28331.42 |
| ff | shuf | 0.00 | 1521.57 | 6660.26 | 19967.21 |
| ff | oracle | 0.00 | 0.00 | 0.00 | 6531.87 |

Deciding horizon N=16384, MARGIN=163.84 bits.

## Anti-smoothing difference D = G_16384(arm,iso) - G_16384(arm,ghost)

- cache: D = -9997.32 bits
- align: D = 0.00 bits
- both: D = -9997.32 bits
- shuf: D = -7212.20 bits
- oracle: D = 0.00 bits

## Gates


## Verdicts (competition arms: cache, align, both)

- **cache**: G_16384(iso)=10088.45 (need >=163.84: yes) | beats shuffled by 3699.60 (need >=163.84: yes) | D=-9997.32 (need >=163.84: no) | false-friend gate: passes => not earned
- **align**: G_16384(iso)=0.00 (need >=163.84: no) | beats shuffled by -6388.85 (need >=163.84: no) | D=0.00 (need >=163.84: no) | false-friend gate: passes => not earned
- **both**: G_16384(iso)=10088.45 (need >=163.84: yes) | beats shuffled by 3699.60 (need >=163.84: yes) | D=-9997.32 (need >=163.84: no) | false-friend gate: passes => not earned

## Overall

"Transfer not detected": no competition arm meets the PASS wording at N=16384.

Additionally, oracle itself fails G_16384>=MARGIN on W-iso (oracle G_16384=0.00 < 163.84): "transfer not detected: carried memory insufficient" — the matcher is exonerated, the memory itself is the failure.

## Interference (any arm, any world, G_16384 <= -MARGIN)

None.

## Ghost invariants (verifier-computed)

- Raw: unigram L1=0.005991 (bound <=0.01, holds) | bigram MI=0.005374 bits (bound <=0.01, holds)
- Consumed (post-unit) level, OPEN POINT — the protocol froze raw-level thresholds only, no bound exists at this level: unit unigram entropy=9.5793 bits, unit bigram MI=2.3938 bits (units=2304, stream=209565 tokens)

## Isomorphism (W-iso construction)

- A-train merges: 2048 | cipher(A-train) merges: 2048 | matched in lockstep: 2048 / 2048

## World reconstruction

- W-iso sha256=6f6816cfe77f952d7cba723d1068f6d637de7156a3a80a366a5002fcd22306c6 match=1
- W-ghost sha256=b422ad11aa911c650c72ca99e66356a20eeb79fdf2c07f09e316f3829f4e8af1 match=1
- W-ff sha256=9caae8c103896bea71b80276406239aa2f251b90a256f546827e02e4d88fbc47 match=1 len=445973
