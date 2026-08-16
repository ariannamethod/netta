#!/usr/bin/env python3
"""Run the sealed NETTA ZERO Gutenberg arena without scientific tuning."""

from __future__ import annotations

import argparse
import hashlib
import json
import shlex
import shutil
import statistics
import subprocess
import sys
import urllib.request
from collections import Counter
from pathlib import Path


SOURCES = {
    "dracula": {
        "url": "https://www.gutenberg.org/cache/epub/345/pg345.txt",
        "file": "pg345.txt",
        "bytes": 890348,
        "sha256": "96cd16eacdbfebae8fdda5591f66e0cc8ee76be18e0cd1aca02bc00615782d28",
    },
    "frankenstein": {
        "url": "https://www.gutenberg.org/cache/epub/84/pg84.txt",
        "file": "pg84.txt",
        "bytes": 448885,
        "sha256": "7810cd483cffcf2cc8a1d8f0d5807931e69d4f48cd14149b8c76f88af82fead3",
    },
    "technical": {
        "url": "https://www.gutenberg.org/cache/epub/28335/pg28335.txt",
        "file": "pg28335.txt",
        "bytes": 258836,
        "sha256": "8308c35e7140d412f2f0d711e0ebef45c05bb941f44cecb503212a7442b577b6",
    },
}
DONOR_SEED = 160816
PROBE_SEED = 260816
SHUFFLE_SEED = 0x4E45545441475241
DONOR_EPISODES = 8
DONOR_STEPS = 8000
PROBE_STEPS = 4096
PROBE_STARTS = (4096, 65536, 131072)
MODELS = ("atomic-uni", "byte-bi", "byte-tri", "unit-uni", "move-bi", "core")


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def acquire(source_dir: Path, name: str, spec: dict[str, object]) -> bytes:
    source_dir.mkdir(parents=True, exist_ok=True)
    path = source_dir / str(spec["file"])
    if not path.exists():
        print(f"download {spec['url']}", flush=True)
        with urllib.request.urlopen(str(spec["url"]), timeout=120) as response:
            path.write_bytes(response.read())
    raw = path.read_bytes()
    if len(raw) != spec["bytes"] or sha256(raw) != spec["sha256"]:
        raise RuntimeError(f"{name}: raw source length or SHA-256 changed")
    return raw


def normalize(raw: bytes, name: str) -> bytes:
    unix = raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    lines = unix.splitlines(keepends=True)
    starts = [i for i, line in enumerate(lines)
              if line.startswith(b"*** START OF THE PROJECT GUTENBERG EBOOK")]
    if len(starts) != 1:
        raise RuntimeError(f"{name}: expected exactly one Gutenberg START line")
    ends = [i for i, line in enumerate(lines)
            if i > starts[0] and
            line.startswith(b"*** END OF THE PROJECT GUTENBERG EBOOK")]
    if len(ends) != 1:
        raise RuntimeError(f"{name}: expected exactly one Gutenberg END line")
    body = b"".join(lines[starts[0] + 1:ends[0]])
    if not body:
        raise RuntimeError(f"{name}: normalization produced an empty body")
    return body


def splitmix64(state: int) -> tuple[int, int]:
    mask = (1 << 64) - 1
    state = (state + 0x9E3779B97F4A7C15) & mask
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & mask
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & mask
    return state, (z ^ (z >> 31)) & mask


def shuffled_twin(source: bytes) -> bytes:
    twin = bytearray(source)
    state = SHUFFLE_SEED
    for i in range(len(twin) - 1, 0, -1):
        state, value = splitmix64(state)
        j = value % (i + 1)
        twin[i], twin[j] = twin[j], twin[i]
    result = bytes(twin)
    if len(result) != len(source) or Counter(result) != Counter(source):
        raise RuntimeError("shuffled twin does not conserve the byte histogram")
    return result


def run_checked(cmd: list[str], log: Path | None = None) -> str:
    result = subprocess.run(cmd, check=False, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    record = f"$ {shlex.join(cmd)}\n{result.stdout}"
    if log is not None:
        log.write_text(record, encoding="utf-8")
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}):\n{record}")
    return result.stdout


def extract_prices(output: str) -> dict[str, float]:
    found: dict[str, float] = {}
    prefix = "this-life model "
    for line in output.splitlines():
        if not line.startswith(prefix):
            continue
        fields = line.split()
        found[fields[2]] = float(fields[-1])
    missing = [model for model in MODELS if model not in found]
    if missing:
        raise RuntimeError(f"missing this-life prices: {', '.join(missing)}")
    return {model: found[model] for model in MODELS}


def copy_life(source_stem: Path, target_stem: Path) -> None:
    shutil.copyfile(source_stem.with_suffix(".state"),
                    target_stem.with_suffix(".state"))
    shutil.copyfile(source_stem.with_suffix(".bio"),
                    target_stem.with_suffix(".bio"))


def life_args(island: Path, stem: Path) -> list[str]:
    return [str(island), "--state", str(stem.with_suffix(".state")),
            "--bio", str(stem.with_suffix(".bio"))]


def parse_unit_life(lines: list[str]) -> tuple[dict[int, bytes], dict[int, bool]]:
    identities: dict[int, bytes] = {}
    living: dict[int, bool] = {}
    for line in lines:
        fields = line.split("\t")
        if fields[0] == "b":
            unit = int(fields[2])
            identities[unit] = bytes.fromhex(fields[4])
            living[unit] = True
        elif fields[0] == "d":
            living[int(fields[2])] = False
        elif fields[0] == "u":
            living[int(fields[2])] = True
    return identities, living


def events_after(path: Path, line_count: int) -> list[str]:
    return path.read_text(encoding="utf-8").splitlines()[line_count:]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("work_dir", type=Path,
                        help="new directory for corpus, states, logs, and results")
    parser.add_argument("--source-dir", type=Path,
                        help="raw download cache (defaults to WORK_DIR/raw)")
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

    source_record: dict[str, dict[str, object]] = {}
    corpora: dict[str, Path] = {}
    bodies: dict[str, bytes] = {}
    for name, spec in SOURCES.items():
        raw = acquire(raw_dir, name, spec)
        body = normalize(raw, name)
        path = corpus_dir / f"{name}.bytes"
        path.write_bytes(body)
        corpora[name] = path
        bodies[name] = body
        source_record[name] = {
            "url": spec["url"], "raw_bytes": len(raw),
            "raw_sha256": sha256(raw), "body_bytes": len(body),
            "body_sha256": sha256(body),
        }
    shuffled = shuffled_twin(bodies["frankenstein"])
    corpora["shuffled"] = corpus_dir / "frankenstein-shuffled.bytes"
    corpora["shuffled"].write_bytes(shuffled)
    source_record["shuffled"] = {
        "parent": "frankenstein", "seed": f"0x{SHUFFLE_SEED:016x}",
        "body_bytes": len(shuffled), "body_sha256": sha256(shuffled),
        "histogram_equal": True,
    }

    repo = Path(__file__).resolve().parent.parent
    netta = work / "netta"
    run_checked(["cc", "-O2", "-std=c11", "-Wall", "-Wextra",
                 "-Wpedantic", str(repo / "netta.c"), "-lm", "-o", str(netta)],
                log_dir / "build.log")

    donors: dict[str, Path] = {}
    for name in ("dracula", "technical"):
        stem = life_dir / f"donor-{name}"
        cmd = [str(netta), *life_args(corpora[name], stem), "--reset",
               "--seed", str(DONOR_SEED), "--episodes", str(DONOR_EPISODES),
               "--steps", str(DONOR_STEPS)]
        run_checked(cmd, log_dir / f"donor-{name}.log")
        donors[name] = stem

    transfer_rows: list[dict[str, object]] = []
    arms = (
        ("kin", donors["dracula"], corpora["frankenstein"]),
        ("alien", donors["technical"], corpora["frankenstein"]),
        ("shuffle", donors["dracula"], corpora["shuffled"]),
    )
    for arm, donor, target in arms:
        for start in PROBE_STARTS:
            travelled = life_dir / f"probe-{arm}-{start}-travelled"
            newborn = life_dir / f"probe-{arm}-{start}-newborn"
            copy_life(donor, travelled)
            tr_out = run_checked(
                [str(netta), *life_args(target, travelled), "--seed",
                 str(PROBE_SEED), "--episodes", "1", "--steps",
                 str(PROBE_STEPS), "--start", str(start)],
                log_dir / f"probe-{arm}-{start}-travelled.log")
            nb_out = run_checked(
                [str(netta), *life_args(target, newborn), "--reset", "--seed",
                 str(PROBE_SEED), "--episodes", "1", "--steps",
                 str(PROBE_STEPS), "--start", str(start)],
                log_dir / f"probe-{arm}-{start}-newborn.log")
            tr_prices, nb_prices = extract_prices(tr_out), extract_prices(nb_out)
            for model in MODELS:
                transfer_rows.append({
                    "arm": arm, "start": start, "model": model,
                    "travelled": tr_prices[model], "newborn": nb_prices[model],
                    "gain": nb_prices[model] - tr_prices[model],
                })

    median_gain: dict[str, dict[str, float]] = {}
    for arm, _, _ in arms:
        median_gain[arm] = {}
        for model in MODELS:
            values = [float(row["gain"]) for row in transfer_rows
                      if row["arm"] == arm and row["model"] == model]
            median_gain[arm][model] = statistics.median(values)

    unit_stem = life_dir / "unit-voyage"
    copy_life(donors["dracula"], unit_stem)
    unit_bio = unit_stem.with_suffix(".bio")
    donor_lines = unit_bio.read_text(encoding="utf-8").splitlines()
    identities, donor_living = parse_unit_life(donor_lines)
    donor_units = {u for u, alive in donor_living.items() if alive}
    shared = {u for u in donor_units if identities[u] in bodies["frankenstein"]}
    specific = donor_units - shared
    run_checked(
        [str(netta), *life_args(corpora["frankenstein"], unit_stem),
         "--episodes", "4", "--steps", "8000"],
        log_dir / "unit-frankenstein.log")
    crossing_events = events_after(unit_bio, len(donor_lines))
    _, crossing_living = parse_unit_life(donor_lines + crossing_events)
    shared_survivors = {u for u in shared if crossing_living.get(u, False)}
    specific_dead = {u for u in specific if not crossing_living.get(u, False)}
    before_return = len(donor_lines) + len(crossing_events)
    run_checked(
        [str(netta), *life_args(corpora["dracula"], unit_stem),
         "--episodes", "2", "--steps", "8000"],
        log_dir / "unit-return.log")
    return_events = events_after(unit_bio, before_return)
    resurrected_specific = {
        int(fields[2]) for fields in (line.split("\t") for line in return_events)
        if fields[0] == "u" and int(fields[2]) in specific_dead
    }
    unit_record = {
        "donor_living": len(donor_units), "initial_shared": len(shared),
        "initial_dracula_specific": len(specific),
        "shared_survivors": len(shared_survivors),
        "dracula_specific_dead": len(specific_dead),
        "dracula_specific_resurrected": len(resurrected_specific),
        "shared_survivor_ids": sorted(shared_survivors),
        "specific_dead_ids": sorted(specific_dead),
        "resurrected_specific_ids": sorted(resurrected_specific),
    }

    court_record: dict[str, dict[str, object]] = {}
    for arm, target in (("kin", corpora["frankenstein"]),
                        ("alien", corpora["technical"])):
        stem = life_dir / f"court-{arm}"
        copy_life(donors["dracula"], stem)
        bio = stem.with_suffix(".bio")
        baseline = len(bio.read_text(encoding="utf-8").splitlines())
        run_checked(
            [str(netta), *life_args(target, stem), "--episodes", "8",
             "--steps", "800"], log_dir / f"court-{arm}.log")
        events = events_after(bio, baseline)
        refusals = [line for line in events if line.startswith(("r\t", "q\t"))]
        target_episodes = [int(line.split("\t")[1]) - DONOR_EPISODES
                           for line in refusals]
        court_record[arm] = {
            "refusals": len(refusals), "target_episodes": target_episodes,
            "receipts": refusals,
        }

    kin = median_gain["kin"]
    alien = median_gain["alien"]
    shuffle = median_gain["shuffle"]
    verdicts = {
        "kin_transfer_grows_with_context":
            kin["atomic-uni"] > 0 and
            kin["byte-bi"] > kin["atomic-uni"] and
            kin["byte-tri"] > kin["byte-bi"],
        "alien_context_negative":
            alien["byte-bi"] - alien["atomic-uni"] < 0 and
            alien["byte-tri"] - alien["atomic-uni"] < 0,
        "shuffle_kills_contextual_excess":
            shuffle["byte-bi"] - shuffle["atomic-uni"] < 0.1 and
            shuffle["byte-tri"] - shuffle["atomic-uni"] < 0.1,
        "shared_vocabulary_survives": len(shared_survivors) > 0,
        "dracula_specific_vocabulary_dies": len(specific_dead) > 0,
        "dracula_specific_vocabulary_resurrects":
            len(resurrected_specific) > 0,
        "kin_court_never_refuses": court_record["kin"]["refusals"] == 0,
        "alien_court_refuses_early": any(
            episode in (2, 3) for episode in court_record["alien"]["target_episodes"]),
    }

    record = {
        "protocol": {
            "donor_seed": DONOR_SEED, "probe_seed": PROBE_SEED,
            "donor_episodes": DONOR_EPISODES, "donor_steps": DONOR_STEPS,
            "probe_steps": PROBE_STEPS, "probe_starts": PROBE_STARTS,
            "models": MODELS, "core_hebb_v1": False,
        },
        "sources": source_record, "transfer_windows": transfer_rows,
        "median_gain": median_gain, "unit_voyage": unit_record,
        "court": court_record, "verdicts": verdicts,
    }
    (work / "results.json").write_text(
        json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    with (work / "transfer.tsv").open("w", encoding="utf-8") as out:
        out.write("arm\tstart\tmodel\ttravelled\tnewborn\tgain\n")
        for row in transfer_rows:
            out.write(f"{row['arm']}\t{row['start']}\t{row['model']}\t"
                      f"{row['travelled']:.6f}\t{row['newborn']:.6f}\t"
                      f"{row['gain']:.6f}\n")
    with (work / "verdicts.tsv").open("w", encoding="utf-8") as out:
        out.write("prediction\tverdict\n")
        for name, passed in verdicts.items():
            out.write(f"{name}\t{'PASS' if passed else 'FAIL'}\n")

    print("\nmedian transfer gains (newborn - travelled), bits/raw-byte")
    for arm in ("kin", "alien", "shuffle"):
        values = " ".join(f"{model}={median_gain[arm][model]:.6f}"
                          for model in MODELS)
        print(f"{arm}: {values}")
    print("\nunit voyage:", json.dumps(unit_record, sort_keys=True))
    print("court:", json.dumps(court_record, sort_keys=True))
    print("\nverdicts")
    for name, passed in verdicts.items():
        print(f"{'PASS' if passed else 'FAIL'} {name}")
    print(f"\nrecord: {work / 'results.json'}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"gutenberg_arena: {error}", file=sys.stderr)
        raise SystemExit(1)
