#!/usr/bin/env python3
"""Run the resealed body-17 plasticity court without tuning it."""

from __future__ import annotations

import argparse
import json
import math
import statistics
import sys
from collections import Counter
from pathlib import Path

from gutenberg_arena import (
    DONOR_EPISODES,
    DONOR_SEED,
    DONOR_STEPS,
    PROBE_SEED,
    PROBE_STARTS,
    PROBE_STEPS,
    SOURCES,
    acquire,
    copy_life,
    life_args,
    normalize,
    run_checked,
    sha256,
    shuffled_twin,
)


GENOMES = tuple(range(8))
MARGIN = 0.1
SAT_MARGIN = 0.01
ALIEN_LENGTH = 233688
ALIEN_SHA256 = "7a3f7a161e6777a9905db5e06bbd3560a82999cdab06a09f0e3258d401ce4f40"
TECH_BODY_SHA256 = "a1ef39364793ef3c4b5facb80b2248f43c08294ef1b0e0a011948667c37ddf20"
NETTA_TEXT_SHA256 = "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb"


def distribution_counts(data: bytes, order: int) -> tuple[Counter[object], int]:
    if order == 1:
        counts: Counter[object] = Counter(data)
    elif order == 2:
        counts = Counter(zip(data, data[1:]))
    else:
        raise ValueError("only byte and adjacent-byte distributions exist")
    return counts, sum(counts.values())


def entropy(data: bytes, order: int) -> float:
    counts, total = distribution_counts(data, order)
    if total == 0:
        raise RuntimeError("entropy needs a nonempty empirical distribution")
    return -sum((count / total) * math.log2(count / total)
                for count in counts.values())


def js_divergence(left: bytes, right: bytes, order: int) -> float:
    lc, ln = distribution_counts(left, order)
    rc, rn = distribution_counts(right, order)
    if ln == 0 or rn == 0:
        raise RuntimeError("Jensen-Shannon divergence needs two distributions")
    value = 0.0
    for key in lc.keys() | rc.keys():
        p = lc[key] / ln
        q = rc[key] / rn
        middle = 0.5 * (p + q)
        if p:
            value += 0.5 * p * math.log2(p / middle)
        if q:
            value += 0.5 * q * math.log2(q / middle)
    return value


def conditional_js(left: bytes, right: bytes) -> float:
    return (js_divergence(left, right, 2) -
            js_divergence(left[:-1], right[:-1], 1))


def byte_lattice(length: int) -> bytes:
    return bytes((((i % 251) * 73 + ((i // 251) % 17) * 19) & 255)
                 for i in range(length))


def parse_jury(output: str) -> tuple[dict[int, dict[str, float | int]], float]:
    found: dict[int, dict[str, float | int]] = {}
    core_price: float | None = None
    for line in output.splitlines():
        if line.startswith("this-life model core bits/byte "):
            core_price = float(line.rsplit(" ", 1)[1])
        if not line.startswith("this-life jury genome "):
            continue
        fields = line.split()
        genome = int(fields[3])
        found[genome] = {
            "price": float(fields[5]),
            "max_row_abs_h": float(fields[7]),
            "near_sat": float(fields[9]),
            "clamps": int(fields[11]),
            "gates_pos": int(fields[13][1:]),
            "gates_neg": int(fields[14][1:]),
        }
    missing = [g for g in GENOMES if g not in found]
    if missing or core_price is None:
        raise RuntimeError(f"missing jury output: genomes={missing}, core={core_price}")
    if found[0]["price"] != core_price:
        raise RuntimeError("jury genome zero disagrees with the ordinary frozen core")
    return found, core_price


def run_jury(netta: Path, island: Path, stem: Path, args: list[str],
             log: Path) -> dict[int, dict[str, float | int]]:
    output = run_checked(
        [str(netta), *life_args(island, stem), "--jury", *args], log)
    record, _ = parse_jury(output)
    return record


def median(values: list[float]) -> float:
    return float(statistics.median(values))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("work_dir", type=Path,
                        help="new directory for corpora, states, logs, results")
    parser.add_argument("--source-dir", type=Path,
                        help="raw Gutenberg cache (defaults to WORK_DIR/raw)")
    args = parser.parse_args()

    work = args.work_dir.resolve()
    if work.exists():
        raise RuntimeError(f"refusing reused work directory: {work}")
    work.mkdir(parents=True)
    raw_dir = (args.source_dir or (work / "raw")).resolve()
    corpus_dir = work / "corpus"
    life_dir = work / "lives"
    log_dir = work / "logs"
    for directory in (corpus_dir, life_dir, log_dir):
        directory.mkdir()

    repo = Path(__file__).resolve().parent.parent
    bodies: dict[str, bytes] = {}
    corpora: dict[str, Path] = {}
    source_record: dict[str, dict[str, object]] = {}
    for name, spec in SOURCES.items():
        raw = acquire(raw_dir, name, spec)
        body = normalize(raw, name)
        bodies[name] = body
        corpora[name] = corpus_dir / f"{name}.bytes"
        corpora[name].write_bytes(body)
        source_record[name] = {
            "raw_bytes": len(raw), "raw_sha256": sha256(raw),
            "body_bytes": len(body), "body_sha256": sha256(body),
            "url": spec["url"],
        }
    if len(bodies["technical"]) != ALIEN_LENGTH or \
            sha256(bodies["technical"]) != TECH_BODY_SHA256:
        raise RuntimeError("normalized technical floor changed")

    netta_text = (repo / "netta.txt").read_bytes()
    if sha256(netta_text) != NETTA_TEXT_SHA256:
        raise RuntimeError("tracked netta.txt reference changed")
    references = {
        "dracula": bodies["dracula"],
        "frankenstein": bodies["frankenstein"],
        "netta.txt": netta_text,
    }

    lattice = byte_lattice(ALIEN_LENGTH)
    if sha256(lattice) != ALIEN_SHA256:
        raise RuntimeError("sealed alien lattice hash changed")
    corpora["alien"] = corpus_dir / "alien-lattice.bytes"
    corpora["alien"].write_bytes(lattice)

    distance_record: dict[str, object] = {"references": {}}
    tech_js1: list[float] = []
    tech_cjs: list[float] = []
    alien_js1: list[float] = []
    alien_cjs: list[float] = []
    for name, reference in references.items():
        tj1 = js_divergence(bodies["technical"], reference, 1)
        tcj = conditional_js(bodies["technical"], reference)
        aj1 = js_divergence(lattice, reference, 1)
        acj = conditional_js(lattice, reference)
        tech_js1.append(tj1)
        tech_cjs.append(tcj)
        alien_js1.append(aj1)
        alien_cjs.append(acj)
        distance_record["references"][name] = {
            "technical_js1": tj1, "technical_cjs": tcj,
            "alien_js1": aj1, "alien_cjs": acj,
        }
    census_h = entropy(lattice, 1)
    conditional_h = entropy(lattice, 2) - entropy(lattice[:-1], 1)
    structure_gap = census_h - conditional_h
    distance_record.update({
        "technical_max_js1": max(tech_js1),
        "technical_max_cjs": max(tech_cjs),
        "alien_min_js1": min(alien_js1),
        "alien_min_cjs": min(alien_cjs),
        "alien_census_entropy": census_h,
        "alien_conditional_entropy": conditional_h,
        "alien_structure_gap": structure_gap,
        "alien_sha256": sha256(lattice),
    })
    if min(alien_js1) <= max(tech_js1) or \
            min(alien_cjs) <= max(tech_cjs) or structure_gap < 0.25:
        raise RuntimeError("sealed alien no longer clears its measured floor")

    shuffled_dracula = shuffled_twin(bodies["dracula"])
    corpora["dracula-shuffled"] = corpus_dir / "dracula-shuffled.bytes"
    corpora["dracula-shuffled"].write_bytes(shuffled_dracula)
    source_record["dracula-shuffled"] = {
        "parent": "dracula", "body_bytes": len(shuffled_dracula),
        "body_sha256": sha256(shuffled_dracula),
        "histogram_equal": Counter(shuffled_dracula) == Counter(bodies["dracula"]),
        "seed": "0x4e45545441475241",
    }

    netta = work / "netta"
    run_checked(["cc", "-O2", "-std=c11", "-Wall", "-Wextra",
                 "-Wpedantic", str(repo / "netta.c"), "-lm", "-o", str(netta)],
                log_dir / "build.log")

    donors: dict[str, Path] = {}
    for name, corpus in (("ordered", corpora["dracula"]),
                         ("shuffled", corpora["dracula-shuffled"])):
        stem = life_dir / f"donor-{name}"
        run_jury(
            netta, corpus, stem,
            ["--reset", "--seed", str(DONOR_SEED),
             "--episodes", str(DONOR_EPISODES), "--steps", str(DONOR_STEPS)],
            log_dir / f"donor-{name}.log")
        donors[name] = stem

    probe_rows: list[dict[str, object]] = []
    for target, corpus in (("kin", corpora["frankenstein"]),
                           ("alien", corpora["alien"])):
        for donor_name, donor in donors.items():
            for start in PROBE_STARTS:
                stem = life_dir / f"probe-{target}-{donor_name}-{start}"
                copy_life(donor, stem)
                scores = run_jury(
                    netta, corpus, stem,
                    ["--seed", str(PROBE_SEED), "--episodes", "1",
                     "--steps", str(PROBE_STEPS), "--start", str(start)],
                    log_dir / f"probe-{target}-{donor_name}-{start}.log")
                for genome, values in scores.items():
                    probe_rows.append({
                        "target": target, "donor": donor_name,
                        "start": start, "genome": genome, **values,
                    })

    patterns = {
        "p5": b"aabab",
        "p6": b"aababa",
        "p7": b"aababab",
        "p8": b"aabababa",
    }
    synthetic_rows: list[dict[str, object]] = []
    for name, pattern in patterns.items():
        tape = (pattern * (15000 // len(pattern) + 1))[:15000]
        corpus = corpus_dir / f"{name}.bytes"
        corpus.write_bytes(tape)
        stem = life_dir / f"synthetic-{name}"
        scores = run_jury(
            netta, corpus, stem,
            ["--reset", "--seed", "5", "--episodes", "5",
             "--steps", "2000"], log_dir / f"synthetic-{name}.log")
        for genome, values in scores.items():
            synthetic_rows.append({"world": name, "genome": genome, **values})

    def probe_values(target: str, donor: str, genome: int,
                     key: str) -> list[float]:
        rows = [row for row in probe_rows
                if row["target"] == target and row["donor"] == donor and
                row["genome"] == genome]
        rows.sort(key=lambda row: int(row["start"]))
        return [float(row[key]) for row in rows]

    verdict_rows: list[dict[str, object]] = []
    for genome in GENOMES:
        kin_r = probe_values("kin", "ordered", genome, "price")
        kin_q = probe_values("kin", "shuffled", genome, "price")
        alien_r = probe_values("alien", "ordered", genome, "price")
        alien_q = probe_values("alien", "shuffled", genome, "price")
        k_value = median([q - r for q, r in zip(kin_q, kin_r)])
        a_value = median([q - r for q, r in zip(alien_q, alien_r)])
        row: dict[str, object] = {
            "genome": genome,
            "K": k_value,
            "kin_ordered": median(kin_r),
            "kin_shuffled": median(kin_q),
            "A": a_value,
            "alien_ordered": median(alien_r),
            "alien_shuffled": median(alien_q),
        }
        verdict_rows.append(row)

    incumbent = verdict_rows[0]
    for row in verdict_rows:
        genome = int(row["genome"])
        health_failures: list[str] = []
        for world in patterns:
            candidate = next(r for r in synthetic_rows
                             if r["world"] == world and r["genome"] == genome)
            control = next(r for r in synthetic_rows
                           if r["world"] == world and r["genome"] == 0)
            if float(candidate["price"]) > float(control["price"]) + MARGIN:
                health_failures.append(f"{world}:price")
            if (float(candidate["max_row_abs_h"]) >
                    float(control["max_row_abs_h"]) + MARGIN):
                health_failures.append(f"{world}:row")
            if (float(candidate["near_sat"]) >
                    float(control["near_sat"]) + SAT_MARGIN):
                health_failures.append(f"{world}:saturation")
            if int(candidate["clamps"]) != 0:
                health_failures.append(f"{world}:clamp")
        gates = {
            "kin_selectivity": float(row["K"]) >= float(incumbent["K"]) + MARGIN,
            "kin_improves": (float(row["kin_ordered"]) <=
                             float(incumbent["kin_ordered"]) - MARGIN),
            "red_not_sabotaged": (float(row["kin_shuffled"]) <=
                                   float(incumbent["kin_shuffled"]) + MARGIN),
            "breadth": float(row["A"]) <= float(incumbent["A"]) + MARGIN,
            "alien_ordered": (float(row["alien_ordered"]) <=
                              float(incumbent["alien_ordered"]) + MARGIN),
            "alien_shuffled": (float(row["alien_shuffled"]) <=
                               float(incumbent["alien_shuffled"]) + MARGIN),
            "synthetic_health": not health_failures,
        }
        row["gates"] = gates
        row["health_failures"] = health_failures
        row["eligible"] = genome != 0 and all(gates.values())

    eligible = [row for row in verdict_rows if bool(row["eligible"])]
    eligible.sort(key=lambda row: (-float(row["K"]), int(row["genome"])))
    winner = int(eligible[0]["genome"]) if eligible else None
    result = {
        "protocol": {
            "state_version": 20, "genomes": GENOMES, "margin": MARGIN,
            "sat_margin": SAT_MARGIN, "donor_seed": DONOR_SEED,
            "probe_seed": PROBE_SEED, "donor_episodes": DONOR_EPISODES,
            "donor_steps": DONOR_STEPS, "probe_steps": PROBE_STEPS,
            "probe_starts": PROBE_STARTS,
        },
        "sources": source_record,
        "alien_measurement": distance_record,
        "probe_rows": probe_rows,
        "synthetic_rows": synthetic_rows,
        "verdict_rows": verdict_rows,
        "winner": winner,
        "promotion": winner is not None,
    }
    (work / "results.json").write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    with (work / "court.tsv").open("w", encoding="utf-8") as out:
        out.write("genome\tK\tkin_ordered\tkin_shuffled\tA\t"
                  "alien_ordered\talien_shuffled\tfailed_gates\t"
                  "health_failures\teligible\n")
        for row in verdict_rows:
            failed = [name for name, passed in row["gates"].items()
                      if not passed]
            out.write(
                f"{row['genome']}\t{row['K']:.6f}\t"
                f"{row['kin_ordered']:.6f}\t{row['kin_shuffled']:.6f}\t"
                f"{row['A']:.6f}\t{row['alien_ordered']:.6f}\t"
                f"{row['alien_shuffled']:.6f}\t{','.join(failed) or '-'}\t"
                f"{','.join(row['health_failures']) or '-'}\t"
                f"{'PASS' if row['eligible'] else 'FAIL'}\n")
    with (work / "synthetic.tsv").open("w", encoding="utf-8") as out:
        out.write("world\tgenome\tprice\tmax_row_abs_h\tnear_sat\tclamps\t"
                  "gates_pos\tgates_neg\n")
        for row in synthetic_rows:
            out.write(
                f"{row['world']}\t{row['genome']}\t{row['price']:.6f}\t"
                f"{row['max_row_abs_h']:.6f}\t{row['near_sat']:.9f}\t"
                f"{row['clamps']}\t{row['gates_pos']}\t{row['gates_neg']}\n")

    print("plasticity court (bits/raw-byte)")
    for row in verdict_rows:
        print(f"g{row['genome']}: K={row['K']:.6f} "
              f"kin={row['kin_ordered']:.6f}/{row['kin_shuffled']:.6f} "
              f"A={row['A']:.6f} "
              f"alien={row['alien_ordered']:.6f}/{row['alien_shuffled']:.6f} "
              f"eligible={'yes' if row['eligible'] else 'no'}")
    print("verdict:", f"PROMOTE g{winner}" if winner is not None else
          "NULL; FROZEN INCUMBENT REMAINS")
    print("record:", work / "results.json")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"plasticity_court: {error}", file=sys.stderr)
        raise SystemExit(1)
