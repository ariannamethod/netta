#!/usr/bin/env python3
"""Supplementary sanitizer and cross-world parity checks for turn26."""

from __future__ import annotations

import hashlib
from pathlib import Path
import random
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
BASE = "71389502e514a39bd1a643fb2e5992de9677253c"
CANDIDATE = "4494ca59864d992c5b5697fb92443f42953fe54f9b861593c051151527d01225"
STRICT = ["-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(source: Path, binary: Path, extra: list[str]) -> None:
    subprocess.run(["cc", *extra, *STRICT, str(source), "-lm", "-o", str(binary)], check=True)


def run(binary: Path, world: Path, out: Path, flags: list[str]) -> subprocess.CompletedProcess:
    out.mkdir()
    return subprocess.run(
        [str(binary), str(world), "--out", str(out), *flags],
        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
    )


def tree(path: Path) -> dict[str, bytes]:
    return {item.name: item.read_bytes() for item in path.iterdir() if item.is_file()}


def main() -> int:
    if sha256(ROOT / "netta_mouth.c") != CANDIDATE:
        raise RuntimeError("candidate identity changed")
    with tempfile.TemporaryDirectory(prefix="netta-turn26-stress-") as td:
        work = Path(td)
        old_source = work / "old.c"
        old_source.write_bytes(subprocess.check_output(["git", "show", BASE + ":netta_mouth.c"], cwd=ROOT))
        old = work / "old"
        new = work / "new"
        sanitizer = work / "sanitizer"
        build(old_source, old, ["-O2"])
        build(ROOT / "netta_mouth.c", new, ["-O2"])
        build(ROOT / "netta_mouth.c", sanitizer, [
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        ])
        san = run(sanitizer, ROOT / "netta.txt", work / "san_out", ["--corridor", "1"])
        if san.returncode:
            raise RuntimeError("sanitizer primary failed:\n" + san.stderr.decode(errors="replace"))

        source = (ROOT / "netta.txt").read_bytes()
        worlds = [source[:n] for n in (100, 127, 251, 509, 997, 2048, 8191, 20000)]
        base = b"Aa meets Bb. Cc asks Dd? Ee answers Ff!\n"
        worlds.extend((base * (n // len(base) + 1))[:n] for n in (113, 333, 1025, 4097))
        rng = random.Random(260925)
        alphabet = b" ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,!?\n"
        for n in (257, 777, 3001, 10001):
            data = bytearray(base)
            data.extend(rng.choice(alphabet) for _ in range(max(0, n - len(data))))
            worlds.append(bytes(data[:n]))

        cases = 0
        settings = ((32, 2, 3), (128, 2, 4), (512, 3, 4))
        for wi, data in enumerate(worlds):
            world = work / ("world_%d.txt" % wi)
            world.write_bytes(data)
            for merges, min_pair, order in settings:
                flags = [
                    "--corridor", "1", "--bytes", "80", "--seeds", "7",
                    "--merges", str(merges), "--min-pair", str(min_pair),
                    "--order", str(order),
                ]
                old_out = work / ("old_%d_%d_%d" % (wi, merges, order))
                new_out = work / ("new_%d_%d_%d" % (wi, merges, order))
                a = run(old, world, old_out, flags)
                b = run(new, world, new_out, flags)
                if a.returncode != b.returncode:
                    raise RuntimeError("return-code mismatch in case %d" % cases)
                if a.returncode == 0 and tree(old_out) != tree(new_out):
                    raise RuntimeError("artifact mismatch in case %d" % cases)
                cases += 1
        print("ASan/UBSan primary PASS")
        print("supplementary parity PASS: %d cases across %d worlds" % (cases, len(worlds)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
