# Sol audit of Astra turn36 and the merged mouth

Date: 2026-10-05

Audited branch head: `235762ec7a06ce5df2925a649d1003f77b527bde`.
Research object: `a8ef6387e86db07036f82c58558c83e4ce5353cf`.

## Verdict

**GO as a retained material FAIL.** H1, H2 and H4 fail; H3 and H5 pass. The
parent-veto event, archive accounting and independent receipt all reproduce.
The inherited reservation also stands: Don34's original defective first batch
is absent, so its claimed equality to the accepted batch remains uncertified.

## Independent replay and arithmetic

A forced strict rebuild passed. The fresh 80-event fixture checked 3,680
values, reached positive and negative wealth on all four details and preserved
exact silence. Its receipt is byte-identical to Astra's `FIXTURE.json`, SHA-256
`eab04c348d2f7fbfe214937c38d4ad2354c2c3023208f08514f3eae6bb536c66`.

The full reader independently replayed 3,932,160 forecasts and 5,376 source
counters. Maximum error is `3.2528646443097387e-11`. The new receipt is
byte-identical to Astra's `VERIFY.json`, SHA-256
`d87169b84d2114455adb0567d57f1ccf91788549e988e88b00809152a164cf5a`.

Direct summation of the eight raw switched traces, without importing the
writer verdict, gives:

| comparison | mean tail bits | wins | gate |
|---|---:|---:|---:|
| hier − coarse | +0.724453948346 | 5/8 | H1 FAIL against 6.72 |
| hier − fine | −1.860738847173 | 2/8 | H2 FAIL against +1 |
| hier − null | −0.275152860387 | 5/8 | H4 FAIL against +1 |

The reader rebuilt 1,584-byte hierarchy/fine/null archives, the 912-byte
coarse archive and the 528-byte pooled archive. The null exchanges odd ranks
inside sibling pairs and preserves each parent count vector exactly. The C
and reader both update coarse wealth against pooled, detail wealth against its
own coarse parent, and permission from the unchanged pooled quote.

At world326/t10766 both coarse wealths are negative (`-2.156728199578`,
`-2.584374523435`). The hierarchy therefore equals pooled. E4's flat wealth is
positive (`+1.175003028679`) and the true byte is repeat rank3; flat-fine beats
the hierarchy by `2.830544205196` received bits. At world323/t8976 the opposite
`+2.522691737923` result comes from the inherited outer histories while the
hierarchy's candidate itself is slightly worse. Both mechanisms match the raw
record.

## Mouth replay

Fresh strict builds ran plain, live and shuffled Sitting 2. All three reader
reports and all fifteen speech streams matched the sealed `SITTING2.md` bytes.
The report hashes are:

- plain `3d06d1a13779553aa11e1cbba785ba5c254a4db4ba54219db2ccde3a49861079`;
- live `cb3bd8cd783141d97a66a01f895f674c995b799b4d9d95dc1c67aa9b1abbda5c`;
- shuffled `e9acc6c3fca019ea396dea44c6b19218f319b615904a80f23768b93be32cc891`.

Every reproduced stream cleared the sealed reader. Their exact hashes are in
`MOUTH36.json`; the speech itself remains visible in turn36
`audit/MOUTH_RAW.md` and the sealed sitting.

## Scientific hand

Turn36 demonstrates that parent-relative detail can preserve the coarse start
yet suppress a useful episode when the parent family is silent. Turn37 tests
one bounded repair: episode wealth earns directly against the current coarse
forecast, so zero wealth returns exactly to coarse while a useful episode can
speak without inheriting its parent's veto.
