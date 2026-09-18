#!/usr/bin/env python3
"""Source-only feasibility audit of immediate continuations at episode prefixes.

This never opens target traces, quotes or outcomes. It describes a possible
portable representation; it does not evaluate a recipient prediction.
"""

import argparse
import collections
import csv
import gzip
import hashlib
import json
import math
import struct
from pathlib import Path


SOURCES = ("sourceAB", "sourceBA", "sourceCD", "sourceDC")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_manifest(root, manifest_name, files):
    manifest = json.loads((root / manifest_name).read_text())
    for path in files:
        relative = str(path.relative_to(root))
        expected = manifest.get(relative)
        if expected is None or sha(path) != expected:
            raise ValueError(f"missing or changed input: {relative}")


def grammar(path):
    data = path.read_bytes()
    magic, count, reserved = struct.unpack_from("<8sII", data)
    if magic != b"NETEP001" or reserved or len(data) != 16 + count * 8:
        raise ValueError(f"bad source grammar: {path}")
    expansions = [bytes((i,)) for i in range(7)]
    definitions = []
    for i in range(count):
        left, right, _support = struct.unpack_from("<HHI", data, 16 + i * 8)
        if left >= len(expansions) or right >= len(expansions):
            raise ValueError("not a preceding-child grammar")
        expansions.append(expansions[left] + expansions[right])
        definitions.append((left, right))
    if count != 64 or max(map(len, expansions)) > 32:
        raise ValueError("unexpected grammar dimensions")
    return definitions, expansions


def proper_prefixes(expansions):
    # A prefix appearing in multiple rules has one canonical representation.
    ids = {}
    for rule_id, expansion in enumerate(expansions[7:], start=7):
        for length in range(1, len(expansion)):
            ids.setdefault(expansion[:length], (rule_id, length))
    return ids


def measure_world(root, world):
    folder = root / "data" / f"world{world}"
    book = root / "memory" / f"world{world}" / "episodes.bin"
    inputs = [book] + [folder / f"{name}.tsv.gz" for name in SOURCES]
    verify_manifest(root, "MEMORY_MANIFEST.json", [book])
    verify_manifest(root, "DATA_MANIFEST.json", inputs[1:])
    definitions, expansions = grammar(book)
    prefixes = proper_prefixes(expansions)
    max_length = max(map(len, prefixes))
    counts = collections.defaultdict(lambda: [0] * 7)
    global_repeat = collections.defaultdict(lambda: [0] * 7)
    source_repeat = 0
    for name in SOURCES:
        history = bytearray()
        with gzip.open(folder / f"{name}.tsv.gz", "rt") as stream:
            for row in csv.DictReader(stream, delimiter="\t"):
                k, rank = int(row["k"]), int(row["rank"])
                if not 0 <= rank <= k <= 6:
                    raise ValueError("invalid source relation")
                if rank:
                    global_repeat[k][rank] += 1
                    source_repeat += 1
                for length in range(1, min(max_length, len(history)) + 1):
                    prefix = bytes(history[-length:])
                    if prefix in prefixes:
                        counts[prefix, k][rank] += 1
                history.append(rank)
    scored = []
    for (prefix, k), vector in counts.items():
        repeated = sum(vector[1:k + 1])
        if repeated < 16 or not k:
            continue
        total_base = sum(global_repeat[k][1:k + 1])
        gain = math.fsum(
            c * math.log2((c / repeated) / (global_repeat[k][r] / total_base))
            for r, c in enumerate(vector[1:k + 1], start=1) if c
        )
        score = gain - 8 * 17
        if score > 0:
            scored.append((score, prefix, k, vector))
    scored.sort(key=lambda item: (-item[0], -len(item[1]), item[1], item[2]))
    chosen = scored[:15]
    selected = {(prefix, k) for _, prefix, k, _ in chosen}
    covered = 0
    for name in SOURCES:
        history = bytearray()
        with gzip.open(folder / f"{name}.tsv.gz", "rt") as stream:
            for row in csv.DictReader(stream, delimiter="\t"):
                k, rank = int(row["k"]), int(row["rank"])
                if rank and any((bytes(history[-length:]), k) in selected
                                for length in range(1, min(max_length, len(history)) + 1)):
                    covered += 1
                history.append(rank)
    records = [
        {"rule_id": prefixes[prefix][0], "length": len(prefix), "k": k,
         "prefix": list(prefix), "counts": vector, "source_score_bits": score}
        for score, prefix, k, vector in chosen
    ]
    all_rows = len(counts)
    usable_rows = sum(k > 1 and sum(vector[1:k + 1]) >= 16
                      for (_prefix, k), vector in counts.items())
    if any(n > 65535 for vector in counts.values() for n in vector):
        raise ValueError("uint16 count overflow")
    return {
        "world": world, "source_archive_sha256": sha(book),
        "source_trace_sha256": {name: sha(folder / f"{name}.tsv.gz") for name in SOURCES},
        "rule_definitions": len(definitions), "distinct_proper_prefixes": len(prefixes),
        "prefix_k_rows": all_rows, "exact_all_rows_bytes_with_grammar": 16 + 4 * 64 + 17 * all_rows,
        "usable_prefix_k_rows": usable_rows,
        "exact_usable_rows_bytes_with_grammar": 16 + 4 * 64 + 17 * usable_rows,
        "positive_mdl_rows": len(scored), "selected_rows": len(chosen),
        "portable_bytes": 16 + 4 * 64 + 17 * len(chosen),
        "source_repeat_events": source_repeat, "selected_source_repeat_coverage": covered,
        "selected_source_repeat_fraction": covered / source_repeat,
        "selected": records,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--turn9", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = args.turn9.resolve()
    result = {
        "scope": "source-only grammar-prefix immediate continuation feasibility; no target quote or gate",
        "reader_sha256": sha(Path(__file__)),
        "worlds": [measure_world(root, world) for world in range(80, 88)],
    }
    with args.output.open("x") as stream:
        json.dump(result, stream, indent=2, sort_keys=True)
        stream.write("\n")


if __name__ == "__main__":
    main()
