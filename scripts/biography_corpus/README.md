# Canonical corpus of the biography language

Shared test data for every reader of `BIOGRAPHY.md` — a document and
data may be shared; executable parser code may not.

`canonical.rows`: one row of every type and every arm (13 types, 19
shapes). Seventeen rows are harvested from real lives; the `r` 7-field
row and the `q` null row are derived from their emit formats
(`netta.c:1308`, `netta.c:1330`), the arms a short life did not walk.

`<type>_malformed.rows`: rows every reader must refuse — separator,
sign, leading-zero, hex case and width, literal, enum, arity and
non-canonical fixed-point faults, plus the compile-time registry, unit,
move, length and role domains. All values here are context-free grammar
faults; state-relative rows (future episodes or a bounded id that this
life has not actually registered) are deliberately absent — that law
belongs to the organism's resume alone.
