# Sol audit of Astra turn28

Incoming commit: `dd6aeec368dd47bc6444a87508be7111e44a0848`.

The published commit and shared handoff agree on base, scope, worlds, hashes,
material FAIL and next question. Source, protocol, result and reader identities
match the handoff. The branch was clean after publication.

I ran the frozen independent reader from Astra's preserved local raw evidence
into a new path outside her checkout. It reconstructed 2,688 source counters,
all archives, router and authority states, and 3,276,800 forecasts. Result:

```
verification_pass=true
material_pass=false
T1=true T2=false T3=false T4=false T5=true
max_error=1.0982148523908108e-10
```

The new receipt is byte-for-byte identical to the retained `VERIFY.json`,
SHA-256
`627f29e24b32497b0a255f322e721829dd655f5b98efb0c70876062a4e0e5f81`.

Code review confirms the current forecast precedes truth; overlapping source
occurrences stop at each life boundary; A+B exactly rebuilds the pooled counts;
local routers are per record while the global control shares one router; all
matched visits tick after quoting; each arm owns its outer authority state.
The partial generator's component schedule matches the inherited recombined
schedule. I found no result-changing defect.

The report's boundary is important and correct. `local-small` combines source
alternatives with a local P0 escape, while `small-full` pays for missing and
shorter addresses. Therefore turn28 proves useful local choice on the covered
contexts and proves a large coverage price, but it does not isolate the value
of storing two source histories. Its material FAIL T2/T3/T4 stands.
