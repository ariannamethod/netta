# Body-17 plasticity court: a healthy Hebb null

Status: **SEALED COURT COMPLETE; NO PROMOTION**
Date: 2026-08-16
Specification commit: `3e57be3`
Runner: `python3 scripts/plasticity_court.py WORK_DIR --source-dir RAW_DIR`

The independently resealed court was committed before the jury existed and
before any expert score was observed. All raw Gutenberg sources were verified
against their sealed lengths and SHA-256 hashes. The deterministic alien
lattice regenerated at SHA-256
`7a3f7a161e6777a9905db5e06bbd3560a82999cdab06a09f0e3258d401ce4f40`
and re-cleared both distance floors and the structure floor.

## Instrument correction before the valid verdict

The first completed runner invocation produced result SHA-256
`c02fe970262165410b824f2352c4eee6d211309a988f1163989a0c7c870122db`,
but its `clamps` counter included the fixed delta readout `Who`. Genome zero
therefore reported thousands of clamp attempts even though recurrent `Wxh` and
`Whh` had never moved. That violated the court's quarantine: fixed readout
maturity is outside the recurrent-plasticity trial. The invocation is retained
here as an invalid instrument run, not interpreted as a verdict.

Only the counter boundary changed: fixed readout clamp behavior is no longer
charged to a recurrent genome. No weight update, gene, price, source, seed,
window, threshold, or promotion rule changed. The corrected full court was run
twice from empty work directories. `results.json`, `court.tsv`, and
`synthetic.tsv` were byte-identical between runs. Final `results.json` SHA-256:
`68999835b204fd19bfedfc47486b476a6f1ec251168f6f311e9993da4c0e7b6d`.

## Synthetic physiology

All seven plastic genomes passed every period-5 through period-8 health and
performance veto over 40000 published bytes. Every row recorded zero recurrent
clamp attempts and zero activations at `abs(h) >= 0.98`. Their maximum row-mean
hidden magnitudes stayed within the control-relative margin, and no price lost
more than 0.1 bit/raw-byte to genome zero on any world.

This is the first clean distinction between curing Hebb-v1 and promoting a
replacement. The continuous decay and per-row saturation rent cured the
diagnosed runaway. They did not yet earn selective transfer.

## Paired donor-order verdict

`K` is median shuffled-Dracula-donor price minus ordered-Dracula-donor price on
the same Frankenstein windows. `A` is the identical contrast on the sealed
alien lattice. Lower absolute price is better; positive `K` means donor order
carried useful structure.

| g | K | ordered kin | shuffled kin | A | ordered alien | shuffled alien | eligible |
|---:|---:|---:|---:|---:|---:|---:|:---:|
| 0 | 0.454029 | 4.421399 | 4.875428 | -0.239011 | 7.619160 | 7.380197 | control |
| 1 | 0.215449 | 4.194416 | 4.431553 | 0.068921 | 8.009127 | 8.077773 | no |
| 2 | 0.414400 | 4.623296 | 5.037696 | 0.198349 | 8.628030 | 8.837848 | no |
| 3 | -0.003899 | 5.110921 | 5.104953 | -0.008430 | 9.001324 | 8.989911 | no |
| 4 | 0.532586 | 4.156614 | 4.706346 | 0.063546 | 8.039890 | 8.103043 | no |
| 5 | 1.016435 | 4.009453 | 5.024940 | 0.728842 | 8.084825 | 8.811456 | no |
| 6 | 0.757334 | 4.254475 | 5.003636 | 0.678513 | 8.136986 | 8.812686 | no |
| 7 | 0.359686 | 4.316168 | 4.671281 | -0.145594 | 8.379462 | 8.234363 | no |

Genome 4 is the closest conservative result. It improves ordered kin price by
`0.264785` bit/raw-byte and does not sabotage the shuffled donor, but its
`K=0.532586` misses the sealed `K_0+0.1=0.554029` appointment threshold by
`0.021443`. More importantly, its alien contrast and both alien absolute
prices fail their gates.

Genomes 5 and 6 clear the selectivity and absolute kin-improvement margins, but
both worsen the shuffled red arm and carry a much broader Dracula-order prior
into the alien lattice. Genome 7 passes the difference-based breadth gate and
improves ordered kin just past the absolute margin, but its absolute alien
prices expose damage that the difference alone would hide. The redundant
anti-sabotage gates were load-bearing.

## Verdict

No genome passes every gate. **Frozen recurrent dynamics remain default.**
Hebb-v1 remains quarantined behind `--core-hebb-v1`; no candidate is promoted
or installed as a new red arm. The useful result is narrower and stronger:
continuous local homeostasis repairs Hebb's saturation physiology, while the
remaining defect is causal selectivity across worlds.

Exact medians live in `plasticity_results/2026-08-16-court.tsv`; all synthetic
health rows live in `plasticity_results/2026-08-16-synthetic.tsv`. The runner
recreates the full window-level JSON and refuses a reused work directory.
