#!/usr/bin/env python3
"""One frozen HEAD256/HMM-16 comparison on new synthetic worlds 24..31."""
import argparse
import concurrent.futures
import csv
import hashlib
import json
import math
from pathlib import Path
import random
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "byte_recurrence" / "scratch" / "hmm16"
WORLDS = range(24, 32)
REGIMES = ("preserved", "unrelated", "changed_tail")
N = 16384
H = 2 ** -16
MAX_STATIC_COST = -math.log2(1 - H)


def save_new(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x") as stream:
        json.dump(obj, stream, indent=2, sort_keys=True)
        stream.write("\n")


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def frozen_files():
    return [ROOT / name for name in (
        "byte_recurrence/HMM_PROTOCOL.md", "byte_recurrence/hmm16.py",
        "byte_recurrence/frontend.c", "byte_recurrence/frontend.h",
        "byte_recurrence/live.c", "byte_recurrence/collect.c",
        "portable_recurrence/recurrence.c", "portable_recurrence/recurrence.h",
        "court4/transfer4_confirm_core.c")]


def freeze():
    save_new(OUT / "FREEZE.json", {str(p.relative_to(ROOT)): digest(p)
                                   for p in frozen_files()})


def check_freeze():
    frozen = json.loads((OUT / "FREEZE.json").read_text())
    for name, expected in frozen.items():
        if digest(ROOT / name) != expected:
            raise RuntimeError(f"frozen code changed: {name}")


def seed(world, component):
    message = f"netta-recurrence-v1|{world}|{component}".encode()
    return int.from_bytes(hashlib.sha256(message).digest()[:8], "big")


def patterns(prefix=(0,)):
    if len(prefix) == 6:
        yield "".join(map(str, prefix))
    else:
        for j in range(max(prefix) + 2):
            yield from patterns(prefix + (j,))


PATTERNS = tuple(patterns())
K = {p: max(map(int, p)) + 1 for p in PATTERNS}
assert len(PATTERNS) == 203 and sum(k + 1 for k in K.values()) == 877


def canonical(context):
    unique = []
    result = ""
    for c in context:
        if c not in unique:
            unique.append(c)
        result += str(unique.index(c))
    return result, unique


def grammar(world, component):
    rng = random.Random(seed(world, component))
    return {p: rng.randrange(K[p]) for p in PATTERNS}


def stream(law, other, mode, trajectory_seed, rename_seed):
    rng = random.Random(trajectory_seed)
    data = []
    for t in range(N):
        if t < 6:
            data.append(rng.randrange(256))
            continue
        pattern, names = canonical(data[-6:])
        current = other if mode == "unrelated" or (mode == "changed_tail" and t >= N // 2) else law
        favored = current[pattern]
        z = rng.random()
        if z < .35:
            choices = [x for x in range(256) if x not in names]
            nxt = choices[rng.randrange(len(choices))]
        elif len(names) == 1 or z < .90:
            nxt = names[favored]
        else:
            choices = [j for j in range(len(names)) if j != favored]
            nxt = names[choices[rng.randrange(len(choices))]]
        data.append(nxt)
    permutation = list(range(256))
    random.Random(rename_seed).shuffle(permutation)
    return bytes(permutation[x] for x in data)


def prepare():
    check_freeze()
    if (OUT / "data").exists():
        raise RuntimeError("data already exists; no redraw")
    manifest = {}
    for world in WORLDS:
        folder = OUT / "data" / f"world{world:02d}"
        folder.mkdir(parents=True)
        law, other = grammar(world, "grammar"), grammar(world, "unrelated-grammar")
        for name in [f"source{i}" for i in range(4)] + list(REGIMES):
            source = name.startswith("source")
            trajectory = f"source-trajectory-{name[-1]}" if source else "target-trajectory"
            rename = f"source-rename-{name[-1]}" if source else "target-rename"
            raw = stream(law, other, "preserved" if source else name,
                         seed(world, trajectory), seed(world, rename))
            path = folder / f"{name}.bin"
            with path.open("xb") as output:
                output.write(raw)
            manifest[str(path.relative_to(OUT))] = digest(path)
        assert (folder / "preserved.bin").read_bytes()[:8192] == \
               (folder / "changed_tail.bin").read_bytes()[:8192]
        print("prepared", world, flush=True)
    save_new(OUT / "DATA_MANIFEST.json", manifest)


def check_data():
    check_freeze()
    manifest = json.loads((OUT / "DATA_MANIFEST.json").read_text())
    for name, expected in manifest.items():
        if digest(OUT / name) != expected:
            raise RuntimeError(f"data changed: {name}")


def collect(world):
    folder = OUT / "data" / f"world{world:02d}"
    archive = OUT / "memory" / f"world{world:02d}.bin"
    archive.parent.mkdir(parents=True, exist_ok=True)
    command = [str(ROOT / "byte_recurrence/byte_collect"), str(archive)]
    command += [str(folder / f"source{i}.bin") for i in range(4)]
    subprocess.run(command, check=True)
    return world, digest(archive)


def run_one(job):
    world, regime, mode = job
    archive = OUT / "memory" / f"world{world:02d}.bin"
    raw = OUT / "data" / f"world{world:02d}" / f"{regime}.bin"
    output = OUT / "traces" / f"world{world:02d}" / f"{regime}-{mode}.tsv"
    output.parent.mkdir(parents=True, exist_ok=True)
    command = [str(ROOT / "byte_recurrence/byte_live"), str(archive)]
    if mode == "hmm":
        command.append("--hmm")
    with raw.open("rb") as stdin, output.open("x") as stdout:
        subprocess.run(command, stdin=stdin, stdout=stdout, check=True,
                       stderr=subprocess.PIPE)
    return world, regime, mode


def la(a, b):
    top = max(a, b)
    return top + math.log2(1 + 2 ** (-abs(a - b)))


def source_weight(odds):
    if odds >= 0:
        return 1 / (1 + 2 ** -odds)
    mass = 2 ** odds
    return mass / (1 + mass)


def compare(world, regime):
    folder = OUT / "traces" / f"world{world:02d}"
    with (folder / f"{regime}-static.tsv").open() as sf, \
         (folder / f"{regime}-hmm.tsv").open() as hf:
        static = csv.DictReader(sf, delimiter="\t")
        hmm = csv.DictReader(hf, delimiter="\t")
        odds = 0.0
        gain = 0.0
        static_gain = 0.0
        at8192 = None
        min_gain = peak = drawdown = 0.0
        active = 0
        activation = None
        pre_change = post_change = None
        for t, (a, b) in enumerate(zip(static, hmm, strict=True)):
            assert t < N and int(a["t"]) == int(b["t"]) == t
            assert a["pattern"] == b["pattern"] and a["truth"] == b["truth"]
            cold = float(a["logcold"])
            source = float(a["logcandidate"])
            assert abs(cold - float(b["logcold"])) < 1e-12
            assert abs(source - float(b["logcandidate"])) < 1e-12
            now_active = bool(int(a["active_before"]))
            assert now_active == bool(int(b["active_before"]))
            assert abs(odds - float(b["odds_before"])) < 1e-8
            predicted = la(cold, odds + source) - la(0.0, odds) if now_active else cold
            assert abs(predicted - float(b["loglive"])) < 1e-8
            assert abs(float(a["loglive"]) - float(b["loglive"])) < 1e-8 or now_active
            gain += predicted - cold
            static_gain += float(a["loglive"]) - cold
            assert abs(gain - float(b["gain_after"])) < 1e-8
            assert gain >= -1.0 - 1e-7
            min_gain = min(min_gain, gain)
            peak = max(peak, gain)
            drawdown = max(drawdown, peak - gain)
            assert drawdown <= 16.0 + 1e-7
            if now_active:
                active += 1
                z = odds + source - cold
                odds = math.log2(1 - H) - la(-z, math.log2(H))
            elif int(a["activated_after"]):
                assert int(b["activated_after"])
                odds = 0.0
                activation = t + 1
            if t == 8191:
                at8192 = gain
                pre_change = dict(t=t, odds_before=float(b["odds_before"]),
                                  source_weight=source_weight(float(b["odds_before"])))
            if t == 8192:
                post_change = dict(t=t, odds_before=float(b["odds_before"]),
                                   source_weight=source_weight(float(b["odds_before"])))
        assert t == N - 1 and at8192 is not None
        extra_loss = static_gain - gain
        assert extra_loss <= max(0, active - 1) * MAX_STATIC_COST + 1e-7
        return dict(world=world, regime=regime, gain=gain, static_gain=static_gain,
                    at8192=at8192, tail_gain=gain-at8192, minimum=min_gain,
                    drawdown=drawdown, active_events=active, activation=activation,
                    extra_loss_vs_static=extra_loss,
                    extra_loss_bound=max(0, active-1)*MAX_STATIC_COST,
                    pre_change=pre_change, post_change=post_change)


def run():
    check_data()
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as workers:
        books = dict(workers.map(collect, WORLDS))
    jobs = [(w, r, mode) for w in WORLDS for r in REGIMES for mode in ("static", "hmm")]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as workers:
        for done in workers.map(run_one, jobs):
            print("run", *done, flush=True)
    results = [compare(w, r) for w in WORLDS for r in REGIMES]
    preserved = [x for x in results if x["regime"] == "preserved"]
    summary = dict(law="HEAD256-v1 plus HMM-16", worlds=list(WORLDS),
                   h=H, source_books_sha256=books, lives=results,
                   mean_preserved_gain=sum(x["gain"] for x in preserved)/8,
                   mean_preserved_gain_bpb=sum(x["gain"] for x in preserved)/(8*N),
                   positive_preserved=sum(x["gain"] > 0 for x in preserved),
                   material_pass=(sum(x["gain"] for x in preserved)/(8*N) > .01
                                  and all(x["gain"] > 0 for x in preserved)))
    save_new(OUT / "RESULT.json", summary)
    print(json.dumps({k: v for k, v in summary.items() if k != "lives"}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("phase", choices=("freeze", "prepare", "run"))
    args = parser.parse_args()
    {"freeze": freeze, "prepare": prepare, "run": run}[args.phase]()
