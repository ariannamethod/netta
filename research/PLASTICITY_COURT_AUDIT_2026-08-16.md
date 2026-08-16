# Independent audit of the first body-17 plasticity seal

Date: 2026-08-16
Auditor: Codex / Sol
Verdict: **REFUSED BEFORE ANY EXPERT RUN**

The refused file was `research/PLASTICITY_COURT.md` at commit `c246fcf`,
SHA-256
`20b353a48246604c9fd7770cd6d17e7e25f99ff12b01b374cc4df76536b964a5`.
The merged body-16 implementation reproduced all 131 gates before this review.
No body-17 jury existed and no candidate score had been observed. Resealing is
therefore a preregistration repair, not post-hoc tuning.

## Findings

1. **Different newborn bodies confounded genes with reservoir luck.** The seal
   derived initialization from genome index. A candidate could win because its
   innate `Wxh`/`Whh` draw was better, even with identical plasticity. Every
   genome must instead begin byte-identical; index may select genes only.

2. **The sealed search space was absent.** Six gene classes were named, but no
   ranges, values, or eight genome rows were specified. The prohibition on
   changing ranges after a score could not protect ranges that did not exist.

3. **Readout maturity was still a gene.** `K-H` was meant to subtract maturity,
   yet the court allowed each genome to change the readout learning rate that
   creates that maturity. The recurrent-plasticity trial must hold the accepted
   delta readout fixed.

4. **`K-H` was neither paired nor sabotage-proof.** `K` probed Frankenstein,
   while `H` probed shuffled Frankenstein. Their difference includes target
   tape, target online adaptation, and local census trajectory. A rule can also
   increase apparent selectivity by degrading the shuffled or alien arms. The
   repair compares ordered and shuffled versions of the *same donor* on the
   same target window and adds absolute performance gates.

5. **The homeostat could not reverse runaway.** Discounting new potentiation
   near a target does not shrink weight already accumulated, especially after
   the surprise gate closes. Decay in v1 also ran only when the gate opened.
   The repair applies decay and per-row saturation rent on every byte.

6. **Mean hidden magnitude could hide a dead row.** A few saturated hidden
   units can disappear inside the mean over 32. The repair publishes and gates
   maximum row-mean magnitude and the fraction of activations above `0.98`,
   both against genome zero.

7. **The alien predicate was not computable.** Smoothing, conditional-JS
   weighting, aggregation over references, structure margin, candidate bytes,
   and length were unspecified. The reseal gives exact estimators, a strict
   min-versus-max floor, a deterministic candidate, a hash, and measured
   preregistration figures.

8. **Restart exactness contradicted the open invocation law.** A v19 state made
   under `--core-hebb-v1` can be resumed without the flag and silently changes
   learning law. A persistent jury cannot be restart-exact while its presence
   is similarly forgotten. State v20 must bind the neural-law tuple and refuse
   a mismatch.

9. **The moved arena no longer found the source.** After repository cleanup,
   `scripts/gutenberg_arena.py` treated `scripts/` as the repository root and
   sought `scripts/netta.c`; its documentation still showed the pre-move
   invocation. This is a reproducibility defect independent of the scientific
   verdict.

## Disposition

The first seal remains in git and is cited by hash; it is not silently edited
out of history. The operative court is the replacement
`PLASTICITY_COURT.md`, committed before jury code or scores. If implementation
cannot realize that specification literally, the court aborts and must not
publish a promotion verdict.
