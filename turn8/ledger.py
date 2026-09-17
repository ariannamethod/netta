#!/usr/bin/env python3
"""Read-only, descriptive log-odds ledger for Astra's sealed turn-7 traces."""

import argparse
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path

WORLDS = range(64, 72)
REGIMES = ("mosaic", "mosaic_then_unrelated")
MODES = ("old", "fast", "cap")
SIGNS = ("equal", "favorable", "adverse")


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def blank():
    return dict(n=0, fast_withdrawal=0.0, cap_withdrawal=0.0,
                extra_fast_withdrawal=0.0, cap_minus_fast_price=0.0,
                cap_minus_old_price=0.0, cap_weight=0.0, fast_weight=0.0,
                old_weight=0.0)


def close(a, b, label, tolerance=1e-7):
    if not math.isclose(a, b, rel_tol=0.0, abs_tol=tolerance):
        raise AssertionError(f"{label}: {a} != {b}")


def run_life(root, world, regime, expected):
    path = root / "results" / "outer" / f"world{world:02d}" / f"{regime}.tsv.gz"
    phase = {name: {sign: blank() for sign in SIGNS}
             for name in ("first", "last")}
    gap = 0.0
    at_switch = None
    active_count = 0
    admission_count = 0
    final = None
    with gzip.open(path, "rt", newline="") as stream:
        reader = csv.DictReader(stream, delimiter="\t")
        for t in range(16384):
            group = {}
            for _ in range(6):
                row = next(reader)
                assert int(row["t"]) == t
                key = (row["arm"], row["mode"])
                assert key not in group
                group[key] = row
            assert set(group) == {(arm, mode) for arm in ("revise3", "null3")
                                  for mode in MODES}
            r = {mode: group[("revise3", mode)] for mode in MODES}
            assert len({r[m]["logcold"] for m in MODES}) == 1
            assert len({r[m]["logcandidate"] for m in MODES}) == 1
            assert len({r[m]["active_before"] for m in MODES}) == 1
            if t == 8192:
                at_switch = gap
                close(gap, float(r["cap"]["odds_before"]) -
                      float(r["fast"]["odds_before"]), "switch telescoping")
            if r["cap"]["activated_after"] == "1":
                admission_count += 1
                assert all(r[m]["activated_after"] == "1" for m in MODES)
            if r["cap"]["active_before"] != "1":
                assert all(r[m]["active_before"] == "0" for m in MODES)
                continue
            active_count += 1
            delta = float(r["cap"]["logcandidate"]) - float(r["cap"]["logcold"])
            sign = "equal" if delta == 0.0 else ("favorable" if delta > 0 else "adverse")
            bucket = phase["first" if t < 8192 else "last"][sign]
            withdrawal = {
                m: float(r[m]["odds_before"]) + delta - float(r[m]["odds_after"])
                for m in ("fast", "cap")
            }
            extra = withdrawal["fast"] - withdrawal["cap"]
            gap += extra
            close(gap, float(r["cap"]["odds_after"]) -
                  float(r["fast"]["odds_after"]), f"world {world} t={t} telescoping")
            bucket["n"] += 1
            bucket["fast_withdrawal"] += withdrawal["fast"]
            bucket["cap_withdrawal"] += withdrawal["cap"]
            bucket["extra_fast_withdrawal"] += extra
            bucket["cap_minus_fast_price"] += (float(r["cap"]["loglive"]) -
                                                float(r["fast"]["loglive"]))
            bucket["cap_minus_old_price"] += (float(r["cap"]["loglive"]) -
                                               float(r["old"]["loglive"]))
            for m in MODES:
                bucket[m + "_weight"] += float(r[m]["source_weight_before"])
            final = r
        assert next(reader, None) is None
    assert admission_count == 1 and active_count > 0 and at_switch is not None
    result = expected[(world, regime)]["arms"]["revise3"]
    for half, expected_price in (("first", None), ("last", result["cap"]["tail"] -
                                  result["fast"]["tail"])):
        total = math.fsum(phase[half][s]["cap_minus_fast_price"] for s in SIGNS)
        if expected_price is not None:
            close(total, expected_price, f"world {world} tail price")
    both = math.fsum(phase[p][s]["cap_minus_fast_price"]
                     for p in phase for s in SIGNS)
    close(both, result["cap"]["gain"] - result["fast"]["gain"],
          f"world {world} whole price")
    close(gap, float(final["cap"]["odds_after"]) -
          float(final["fast"]["odds_after"]), f"world {world} final gap")
    for p in phase:
        for s in SIGNS:
            b = phase[p][s]
            for m in MODES:
                b[m + "_mean_weight"] = b[m + "_weight"] / b["n"] if b["n"] else None
    return dict(world=world, regime=regime, trace_sha256=digest(path),
                admissions=admission_count, active_events=active_count,
                odds_gap_at_switch=at_switch, odds_gap_final=gap, phase=phase)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("turn7", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), "output must be new"
    root = args.turn7.resolve()
    verdict = json.loads((root / "VERDICT.json").read_text())
    assert verdict["material_verdict"] == "FAIL"
    claim = json.loads((root / "RESULT.json").read_text())
    assert digest(root / "RESULT.json") == verdict["artifact_sha256"]["RESULT.json"]
    expected = {(x["world"], x["regime"]): x for x in claim["lives"]}
    lives = [run_life(root, w, r, expected) for w in WORLDS for r in REGIMES]
    output = dict(scope="descriptive saved states; no causal ablation or new worlds",
                  verdict="turn7 material FAIL unchanged", lives=lives,
                  result_sha256=digest(root / "RESULT.json"),
                  verdict_sha256=digest(root / "VERDICT.json"))
    with args.output.open("x") as stream:
        json.dump(output, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(f"checked {len(lives)} lives; all switch/end odds and prices agree")


if __name__ == "__main__":
    main()
