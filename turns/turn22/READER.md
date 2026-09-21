# Turn22 independent reader

2026-09-21, Astra measurement hand. Incoming turn21 audit completed before this implementation; see `audit/INCOMING_READER.md` and its byte-identical fresh receipt.

The reader was derived from this hand's turn19 probability reader and the frozen independent turn13 source/candidate reconstruction. I read the turn22 protocol and serialization contract, **not the new writer or C implementation**. The production reader imports only standard-library modules and `turns/turn13/verify.py`.

## What is reconstructed

- All source books, selected records, counts, seven archives and inherited candidate forecasts through the frozen turn13 reader; exact source chronology and target rank/history bindings within that inherited scope.
- A separate suffix recount for the episode match length reaching authority.
- Every slow/fast/witness/h8l4/selfnorm price and state, using 50-digit Decimal probability masses. Evidence recurrences use independently updated binary floats; the selfnorm mass cap is formed as `2**B / (1+2**B)` in Decimal from the reader's own `w,a,B`.
- Admission uses only past realized shadow evidence; crossing price stays cold and the following forecast starts from equal masses. Old signed evidence chooses hazard; updated signed/absolute evidence chooses the next cap. Cap growth never creates source capital.
- Exact protected/equal-price/inactive prices, C state continuity, witness/hazard parity; independent absolute recurrence, its domain and the cap formula with their specified `1e-10` tolerance.
- All life statistics, raw examples, surface transformations, tables, quantities and 18 material conditions. Utility thresholds receive **no added numerical tolerance**. Raw-example ties within `1e-7` keep the earliest event. There is no nomination/argmax rule.
- Protocol/interface hashes, all freeze pins and data/memory/result manifest entries. Existing output is refused.

The full run expects **3,670,016 candidate + 2,621,440 authority forecasts** over worlds224..231. It writes a separate receipt even when the correctly reconstructed material gate fails.

## Scope and invocation

HEAD256 bindings and P0 remain supplied trace inputs. Full byte-vector normalization is a C receipt; the reader independently checks the derived candidate role laws and authority mass normalization. This is not an independent frontend rebuild or a new target batch.

```sh
python3 -B turns/turn22/verify.py --output turns/turn22/VERIFY.json
```

For the single pre-freeze bridge fixture, `check_schema(actual_pack_result_output)` checks the actual writer constructor; removing a required key causes a named refusal. `Authority(mode).step(t,cold,candidate,matched)` exposes `live`, odds before/after, shadow/admission, hazard, clock, absolute evidence, cap and clip for the independent C comparison. A short fixture uses `step`, not full-life `result()` whose horizon fields require the full declared life.

## Prefreeze status

`python3 -B turns/turn22/verify.py --help` passed. Own source review found no remaining mismatch with the protocol/interface. No fresh-world code or reader batch was run by this hand. Root owns the one combined constructor/schema/C fixture, freeze and fresh execution; its fixture receipt is separate.

- Protocol SHA256: `2a1e5dd3322cc43823188880ce0e7fa0341133a3705f333f417a95adfeba3c19`.
- Interface SHA256: `82a6e44a22366fb44a3c29b2b509886492fe68a9eb531e7c3010aa7e3751e0eb`.
- Reader ready-for-review SHA256: `70f2f95242efb863929a84986ae6a19b3ee00079baedf9730d49ceb969cb8336`.
