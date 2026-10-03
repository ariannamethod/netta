#!/usr/bin/env python3
"""Post hoc descriptions of saved turn31 quotes; no predictor or replay.

Run: python3 -B turns/turn31/diagnose.py
Only DIAGNOSIS.json is written. Bins describe recorded pretruth states.
"""
import csv
import gzip
import hashlib
import json
import math
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
ARMS = ("factored", "flat3", "balanced", "pooled", "permuted", "cold")
PAIRS = (("factored", "flat3"), ("factored", "balanced"),
         ("factored", "pooled"), ("flat3", "balanced"),
         ("flat3", "pooled"), ("balanced", "pooled"))
RHO, U0 = 2**-10, 7/8


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def outer(row, arm):
    if not int(row[arm + "_active_before"]):
        return 0.0
    z = float(row[arm + "_odds_before"])
    if z >= 0:
        return 1/(1 + 2**-z)
    e = 2**z
    return e/(1 + e)


def newstats():
    return {"n": 0, "active": 0, "sum_abs_v_difference": 0.,
            "max_abs_v_difference": 0., "sum_abs_u_difference": 0.,
            "sum_factored_u": 0., "sum_flat_u": 0.,
            "sum_factored_effective_memory_mass": 0.,
            "sum_flat_effective_memory_mass": 0.,
            "opposite_case_majority": 0, "v_difference_gt_1e_6": 0,
            "candidate_factored_minus_flat": 0.,
            "live_factored_minus_flat": 0.,
            "abs_candidate_factored_minus_flat": 0.,
            "abs_live_factored_minus_flat": 0.,
            "factored_candidate_minus_cold": 0.,
            "flat_candidate_minus_cold": 0.,
            "factored_live_minus_cold": 0.,
            "flat_live_minus_cold": 0.,
            "balanced_live_minus_cold": 0.,
            "pooled_live_minus_cold": 0.}


def addstats(s, row, uf, vf, ux, vx):
    f, x, c = float(row["factored_live"]), float(row["flat3_live"]), float(row["logcold"])
    fc, xc = float(row["factored_candidate"]), float(row["flat3_candidate"])
    dv = abs(vf-vx)
    s["n"] += 1
    s["active"] += int(row["factored_active_before"])
    s["sum_abs_v_difference"] += dv
    s["max_abs_v_difference"] = max(s["max_abs_v_difference"], dv)
    s["sum_abs_u_difference"] += abs(uf-ux)
    s["sum_factored_u"] += uf
    s["sum_flat_u"] += ux
    s["sum_factored_effective_memory_mass"] += outer(row, "factored")*uf
    s["sum_flat_effective_memory_mass"] += outer(row, "flat3")*ux
    s["opposite_case_majority"] += (vf-.5)*(vx-.5) < 0
    s["v_difference_gt_1e_6"] += dv > 1e-6
    s["candidate_factored_minus_flat"] += fc-xc
    s["live_factored_minus_flat"] += f-x
    s["abs_candidate_factored_minus_flat"] += abs(fc-xc)
    s["abs_live_factored_minus_flat"] += abs(f-x)
    for name, value in (("factored_candidate_minus_cold", fc-c),
                        ("flat_candidate_minus_cold", xc-c),
                        ("factored_live_minus_cold", f-c),
                        ("flat_live_minus_cold", x-c),
                        ("balanced_live_minus_cold", float(row["balanced_live"])-c),
                        ("pooled_live_minus_cold", float(row["pooled_live"])-c)):
        s[name] += value


def describe(s):
    result = dict(s)
    for name, value in s.items():
        if name.startswith("sum_"):
            result["mean_"+name[4:]] = value/s["n"] if s["n"] else None
    return result


def mini(row):
    keep = ("t", "record", "matched", "history", "truth", "rank", "logcold",
            "source_a", "source_b", "source_pooled", "factored_u_before",
            "factored_v_before", "factored_u_after", "factored_v_after", "flat3_w_before",
            "flat3_w_after", "factored_candidate", "flat3_candidate", "factored_live",
            "flat3_live", "balanced_live", "pooled_live", "factored_odds_before",
            "flat3_odds_before")
    out = {k: row[k] for k in keep}
    w = list(map(float, row["flat3_w_before"].split(',')))
    out["flat_u_before"] = w[1]+w[2]
    out["flat_v_before"] = w[1]/(w[1]+w[2])
    out["received_difference"] = float(row["factored_live"])-float(row["flat3_live"])
    return out


def main():
    result = json.loads((HERE/"RESULT.json").read_text())
    buckets = defaultdict(newstats)
    totals = defaultdict(lambda: {arm: {"candidate": 0., "live": 0.} for arm in ARMS})
    hashes, raw = {}, {}
    selected = {}
    for direction, select in (("help", max), ("harm", min)):
        life = select(result["lives"], key=lambda z: z["raw_"+direction]["difference"])
        selected[direction] = (life["world"], life["regime"], int(life["raw_"+direction]["t"]))
        raw[direction] = {"world": life["world"], "regime": life["regime"],
                          "protocol_selected_raw": life["raw_"+direction]}
    source_support = []
    for world in result["worlds"]:
        bookpath = HERE/f"memory/world{world}/BOOKS.json"
        book = json.loads(bookpath.read_text())
        hashes[str(bookpath.relative_to(HERE))] = sha(bookpath)
        for i, rec in enumerate(book["bank_selected"]):
            a, b = (sum(c[1:]) for c in rec["book_counts"])
            source_support.append({"world": world, "record": i, "prefix": rec["prefix"],
                                   "A_repeat_count": a, "B_repeat_count": b,
                                   "A_repeat_support_fraction": a/(a+b) if a+b else None})
    for life in result["lives"]:
        world, regime = life["world"], life["regime"]
        path = HERE/f"results/world{world}/{regime}.turn31.tsv.gz"
        hashes[str(path.relative_to(HERE))] = sha(path)
        selected_here = {name: t for name, (w, r, t) in selected.items()
                         if w == world and r == regime}
        selected_rows = []
        with gzip.open(path, "rt") as stream:
            for row in csv.DictReader(stream, delimiter="\t"):
                t, record = int(row["t"]), int(row["record"])
                windows = ["whole"] + (["early"] if t < 4096 else []) + (["tail"] if t >= 8192 else [])
                for window in windows:
                    dst = totals[(world, regime, window)]
                    for arm in ARMS:
                        for price in ("candidate", "live"):
                            dst[arm][price] += float(row[arm+"_"+price])-float(row["logcold"])
                if selected_here:
                    selected_rows.append(row)
                if record < 0:
                    continue
                uf, vf = float(row["factored_u_before"]), float(row["factored_v_before"])
                wx = list(map(float, row["flat3_w_before"].split(',')))
                ux, vx = wx[1]+wx[2], wx[1]/(wx[1]+wx[2])
                labels = ["matched"]
                if ux <= .5:
                    labels.append("flat_u_le_half")
                if ux <= 2*RHO*U0:
                    labels.append("flat_u_le_twice_floor")
                if (vf-.5)*(vx-.5) < 0:
                    labels.append("opposite_case_majority")
                a, b, cold = (float(row[k]) for k in ("source_a", "source_b", "logcold"))
                if a < cold and b < cold:
                    labels.append("both_sources_worse")
                if (a-cold)*(b-cold) < 0:
                    labels.append("one_source_better_one_worse")
                if a == cold and b == cold:
                    labels.append("both_sources_equal")
                for window in windows:
                    for label in labels:
                        addstats(buckets[(regime, window, label)], row, uf, vf, ux, vx)
        for window, target in (("whole", "gain"), ("early", "early"), ("tail", "tail")):
            for arm in ARMS:
                saved = totals[(world, regime, window)][arm]["live"]
                assert abs(saved-life["arms"][arm][target]) < 1e-7, (world, regime, arm, window)
        for name, t in selected_here.items():
            center = next(row for row in selected_rows if int(row["t"]) == t)
            same = [row for row in selected_rows if row["record"] == center["record"]]
            pos = next(i for i, row in enumerate(same) if int(row["t"]) == t)
            raw[name]["previous_and_next_same_record_visits"] = [mini(row) for row in same[max(0,pos-2):pos+3]]
            book = json.loads((HERE/f"memory/world{world}/BOOKS.json").read_text())
            raw[name]["source_record"] = book["bank_selected"][int(center["record"])]
    contrasts = {}
    for regime in result["regimes"]:
        contrasts[regime] = {}
        for window in ("early", "whole", "tail"):
            entries = {}
            for a, b in PAIRS:
                prices = {}
                for price in ("candidate", "live"):
                    values = [totals[(w,regime,window)][a][price]-totals[(w,regime,window)][b][price]
                              for w in result["worlds"]]
                    prices[price] = {"mean": math.fsum(values)/len(values),
                                     "wins": sum(x>0 for x in values), "per_world": values}
                entries[a+"_minus_"+b] = prices
            contrasts[regime][window] = entries
    output = {"status": "POST HOC DESCRIPTIVE ONLY; no new quotes or interventions",
              "result_sha256": sha(HERE/"RESULT.json"),
              "result_conditions": result["conditions"],
              "bin_definitions": {
                  "flat_u_le_half": "recorded pretruth flat memory mass <= 0.5",
                  "flat_u_le_twice_floor": "recorded pretruth flat memory mass <= 2*rho*u0 = 7/4096; its preceding conditional reset lambda >= 0.5",
                  "effective_memory_mass": "recorded outer source probability times recorded inner memory mass; zero before admission",
                  "both_sources_worse": "source_a < logcold AND source_b < logcold on matched truth; retained in all full metrics",
                  "opposite_case_majority": "(factored_v-0.5)*(flat_v-0.5)<0 before truth",
                  "sums": "over eight lives; contrasts are means per life; overlapping bins are not additive partitions"},
              "contrasts": contrasts,
              "descriptive_buckets": {"/".join(k): describe(v) for k,v in sorted(buckets.items())},
              "source_support": source_support, "protocol_raw_extrema": raw,
              "input_sha256": hashes}
    verification_path = HERE/"VERIFY.json"
    if verification_path.exists():
        verification = json.loads(verification_path.read_text())
        output["completed_independent_verification"] = {
            "sha256": sha(verification_path),
            **{key: verification[key] for key in
               ("conditions", "verification_pass", "material_pass", "forecasts_checked", "max_error")}}
    (HERE/"DIAGNOSIS.json").write_text(json.dumps(output, indent=2, sort_keys=True)+"\n")
    for regime in result["regimes"]:
        c = contrasts[regime]["tail"]["factored_minus_flat3"]
        print(regime, "tail candidate/live", c["candidate"]["mean"], c["live"]["mean"])
    print("Wrote DIAGNOSIS.json from saved prices only.")


if __name__ == "__main__":
    main()
