# NETTA Court 4 source capsule

This directory publishes the final C sources of the fourth transfer court.
The one-shot confirmatory run is complete and spent; publishing the code does
not authorize a rerun, redraw, repair, or a new verdict. The compact history
and result live in [`../NETTALOG0.md`](../NETTALOG0.md).

The procedural receipts, build products, and full evidence remain in the
ignored local closure archive. They are not duplicated in git. The public
source capsule contains only the six translation-unit inputs needed to inspect
and rebuild the selector, builder, and independent verifier.

## Source identities

| File | Bytes | SHA-256 | Role |
| --- | ---: | --- | --- |
| `court4_select.c` | 34298 | `4d961d1dd9d5aa4a24956d110b4f104523c49245b9f2493c3012e09d6a3077dd` | one-shot selector |
| `transfer4.c` | 79965 | `d3b10b8c74ef3285926f7ff9c99253c11531f933e743b31654b0abcc1fd282c1` | frozen development builder, included by the confirmatory wrapper |
| `transfer4_check.c` | 128179 | `29efd12ea63d0a919605cdd4a69a94a841a52fc1ce1aaa2bd2425f8943ff4bef` | frozen development verifier, included by the confirmatory verifier |
| `transfer4_confirm.c` | 48021 | `e5bbd30eefd9206da6729ac55f54a02761a411441c44f767fcb26fbfd8ab8f57` | confirmatory builder wrapper |
| `transfer4_confirm_core.c` | 86087 | `d80016369607ef2c202f28b10470e039b452cf177cf746b24320a7df0a1f4629` | additive confirmatory core |
| `transfer4_confirm_check.c` | 203671 | `a07b4b9c226d976d7b4332125ec283b3e2dfebc80553ee2dc950d07d0a439e04` | independent confirmatory verifier |

`transfer4.c` and `transfer4_check.c` remained byte-identical across repair 2.
The other three existing sources changed, and `transfer4_confirm_core.c` was
added. The final strict build used exactly these bytes.

## Strict source build

From the repository root on the recorded Apple-clang C11 toolchain:

```sh
mkdir -p court4/build
/usr/bin/cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  court4/court4_select.c -lm -o court4/build/court4_select
/usr/bin/cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  court4/transfer4_confirm.c court4/transfer4_confirm_core.c -lm \
  -o court4/build/transfer4_confirm
netta_repo_root=$(pwd)
/usr/bin/cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DCC_REPO="\"${netta_repo_root}/\"" court4/transfer4_confirm_check.c -lm \
  -o court4/build/transfer4_confirm_check
```

The recorded build used Apple clang 21.0.0 and produced empty build stdout and
stderr. Binary hashes are environment- and embedded-path-specific; the source
hashes above are the portable identity.

The publication check rebuilt all three executables from this directory under
the same strict flags with empty compiler stdout and stderr. The rebuilt
selector and builder also reproduced their sealed binary hashes exactly. No
published binary was executed; the one-shot run was not repeated.

The verifier's `--confirmatory` mode receives roots, commitment, freeze,
selection, and builder-output paths explicitly. Its full development and
self-test modes additionally require the sealed laws, fixtures, datasets, and
manifests retained in the closure archive; this source-only directory does not
pretend to replace that evidence package.

## Closed result

The only authorized confirmatory builder and verifier invocations returned
status 0. The verifier printed exactly one verdict line:

`CONFIRMATORY PASS: microscopic relation replicated in selected class`

The boundary remains unchanged: one microscopic relation replicated once, in
the drawn false-friend class, on one unseen base. Transfer at scale is not
claimed.
