#!/usr/bin/env python3
"""Reproduce turn26's frozen old/new C comparison."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import statistics
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
TURN = ROOT / "turns" / "turn26"
BASE = "71389502e514a39bd1a643fb2e5992de9677253c"
EXPECTED = {
    "PROTOCOL.md": "83eaef43b0eeb76db40ac8d66584ea3d3f4e3aa7a1de92889845f8413b0ee0c5",
    "netta_mouth.c": "4494ca59864d992c5b5697fb92443f42953fe54f9b861593c051151527d01225",
    "netta_mouth_check.c": "fc477bfcf4961e658fdae86741dd8c7e0b150670388a71c7fc710284024b7097",
    "netta.py": "f3ef71d3164d895653c4bc9f521ab89557870d1d039b98574b218f0e95fb0278",
    "netta.txt": "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb",
}
CC_FLAGS = ["-O2", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
SEEDS = (7, 19, 42, 101, 271)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def checked(command: list[str], **kwargs) -> subprocess.CompletedProcess:
    p = subprocess.run(command, text=True, capture_output=True, **kwargs)
    if p.returncode:
        raise RuntimeError(
            "command failed rc=%d: %s\nstdout:\n%s\nstderr:\n%s"
            % (p.returncode, " ".join(command), p.stdout, p.stderr)
        )
    return p


def compile_c(source: Path, output: Path) -> None:
    checked(["cc", *CC_FLAGS, str(source), "-lm", "-o", str(output)])


def timed_mouth(binary: Path, world: Path, out: Path, flags: list[str]) -> dict:
    out.mkdir()
    timing = out.parent / (out.name + ".time")
    command = [
        "/usr/bin/time", "-l", "-o", str(timing), str(binary), str(world),
        "--out", str(out), *flags,
    ]
    start = time.perf_counter()
    p = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    elapsed = time.perf_counter() - start
    if p.returncode:
        raise RuntimeError("mouth failed rc=%d: %s\n%s" % (p.returncode, " ".join(command), p.stderr))
    receipt = timing.read_text()
    match = re.search(r"^\s*(\d+)\s+maximum resident set size\s*$", receipt, re.MULTILINE)
    if not match:
        raise RuntimeError("cannot parse maximum resident set size")
    return {
        "seconds": elapsed,
        "maximum_resident_bytes": int(match.group(1)),
        "stderr": p.stderr,
    }


def artifacts(path: Path) -> dict[str, dict[str, int | str]]:
    result = {}
    for item in sorted(path.iterdir()):
        if item.is_file():
            result[item.name] = {"bytes": item.stat().st_size, "sha256": sha256(item)}
    return result


def semantic_pair(
    root: Path, name: str, old: Path, new: Path, world: Path, flags: list[str]
) -> tuple[dict, Path]:
    old_out = root / (name + "_old")
    new_out = root / (name + "_new")
    old_run = timed_mouth(old, world, old_out, flags)
    new_run = timed_mouth(new, world, new_out, flags)
    old_artifacts = artifacts(old_out)
    new_artifacts = artifacts(new_out)
    return {
        "flags": flags,
        "world_bytes": world.stat().st_size,
        "old": old_run,
        "new": new_run,
        "artifacts_identical": old_artifacts == new_artifacts,
        "artifacts": new_artifacts,
    }, new_out


def benchmark(
    root: Path, name: str, old: Path, new: Path, world: Path, flags: list[str]
) -> dict:
    warm = root / (name + "_warm")
    warm.mkdir()
    timed_mouth(old, world, warm / "old", flags)
    timed_mouth(new, world, warm / "new", flags)
    rows = []
    for pair in range(7):
        order = (("old", old), ("new", new)) if pair % 2 == 0 else (("new", new), ("old", old))
        row = {"pair": pair + 1, "order": [label for label, _ in order]}
        for label, binary in order:
            row[label] = timed_mouth(binary, world, root / ("%s_%d_%s" % (name, pair + 1, label)), flags)
        rows.append(row)
        print("%s pair %d/7 old=%.6f new=%.6f" % (
            name, pair + 1, row["old"]["seconds"], row["new"]["seconds"]
        ), flush=True)
    old_seconds = [row["old"]["seconds"] for row in rows]
    new_seconds = [row["new"]["seconds"] for row in rows]
    old_median = statistics.median(old_seconds)
    new_median = statistics.median(new_seconds)
    return {
        "rows": rows,
        "old_median_seconds": old_median,
        "new_median_seconds": new_median,
        "median_speedup_fraction": (old_median - new_median) / old_median,
        "old_peak_resident_bytes": max(row["old"]["maximum_resident_bytes"] for row in rows),
        "new_peak_resident_bytes": max(row["new"]["maximum_resident_bytes"] for row in rows),
    }


def main() -> int:
    actual = {
        "PROTOCOL.md": sha256(TURN / "PROTOCOL.md"),
        "netta_mouth.c": sha256(ROOT / "netta_mouth.c"),
        "netta_mouth_check.c": sha256(ROOT / "netta_mouth_check.c"),
        "netta.py": sha256(ROOT / "netta.py"),
        "netta.txt": sha256(ROOT / "netta.txt"),
    }
    if actual != EXPECTED:
        raise RuntimeError("frozen identity mismatch:\n" + json.dumps({"expected": EXPECTED, "actual": actual}, indent=2))

    with tempfile.TemporaryDirectory(prefix="netta-turn26-") as td:
        work = Path(td)
        old_source = work / "netta_mouth_old.c"
        old_source.write_bytes(subprocess.check_output(["git", "show", BASE + ":netta_mouth.c"], cwd=ROOT))
        if sha256(old_source) != "9cc831657e6bdb3984b55d7f17dcac7040ad877c540d3d579393c642106d6dd6":
            raise RuntimeError("baseline source identity mismatch")
        old = work / "netta_mouth_old"
        new = work / "netta_mouth_new"
        reader = work / "netta_mouth_check"
        compile_c(old_source, old)
        compile_c(ROOT / "netta_mouth.c", new)
        compile_c(ROOT / "netta_mouth_check.c", reader)
        print("strict builds PASS", flush=True)

        small = work / "island2.txt"
        small.write_bytes((ROOT / "netta.txt").read_bytes()[:89509])
        semantic_root = work / "semantic"
        semantic_root.mkdir()
        guards = {}
        guards["primary"], primary_new = semantic_pair(
            semantic_root, "primary", old, new, ROOT / "netta.txt", ["--corridor", "1"]
        )
        guards["small"], _ = semantic_pair(
            semantic_root, "small", old, new, small, ["--corridor", "1"]
        )
        guards["law"], _ = semantic_pair(
            semantic_root, "law", old, new, ROOT / "netta.txt",
            ["--corridor", "1", "--merges", "1024", "--order", "3"],
        )
        print("semantic guards " + " ".join(
            "%s=%s" % (name, "PASS" if row["artifacts_identical"] else "FAIL")
            for name, row in guards.items()
        ), flush=True)

        deterministic = semantic_root / "deterministic_new"
        timed_mouth(new, ROOT / "netta.txt", deterministic, ["--corridor", "1"])
        deterministic_pass = artifacts(primary_new) == artifacts(deterministic)

        report = semantic_root / "reader_report.txt"
        read = checked([
            str(reader), str(ROOT / "netta.txt"), "--dir", str(primary_new),
            "--report", str(report), "--corridor", "1",
        ])
        sitting = ROOT / "speech_court" / "sitting1" / "report_plain.txt"
        reader_report_match = report.read_bytes() == sitting.read_bytes()
        print("determinism=%s reader=%s" % (
            "PASS" if deterministic_pass else "FAIL",
            "PASS" if reader_report_match else "FAIL",
        ), flush=True)

        bench_root = work / "benchmark"
        bench_root.mkdir()
        primary_bench = benchmark(
            bench_root, "primary", old, new, ROOT / "netta.txt", ["--corridor", "1"]
        )
        small_bench = benchmark(
            bench_root, "small", old, new, small, ["--corridor", "1"]
        )

        gates = {
            "strict_builds": True,
            "all_semantic_guards": all(row["artifacts_identical"] for row in guards.values()),
            "reader_report_byte_identical": reader_report_match,
            "deterministic": deterministic_pass,
            "primary_speedup_at_least_20_percent": primary_bench["median_speedup_fraction"] >= 0.20,
            "small_island_no_more_than_10_percent_slower":
                small_bench["new_median_seconds"] <= small_bench["old_median_seconds"] * 1.10,
        }
        result = {
            "verdict": "PASS" if all(gates.values()) else "FAIL",
            "parent_commit": BASE,
            "frozen_identities": actual,
            "compiler": checked(["cc", "--version"]).stdout.splitlines()[0],
            "python": sys.version.splitlines()[0],
            "platform": dict(zip(
                ("sysname", "nodename", "release", "version", "machine"),
                os.uname(),
            )),
            "semantic_guards": guards,
            "deterministic": deterministic_pass,
            "reader": {
                "returncode": 0,
                "stderr": read.stderr,
                "report_sha256": sha256(report),
                "committed_report_sha256": sha256(sitting),
                "byte_identical": reader_report_match,
            },
            "primary_benchmark": primary_bench,
            "small_island_benchmark": small_bench,
            "gates": gates,
        }
        output = TURN / "RESULT.json"
        output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
        print("%s %s" % (result["verdict"], output), flush=True)
        return 0 if result["verdict"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
