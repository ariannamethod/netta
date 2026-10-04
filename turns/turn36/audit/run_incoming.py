#!/usr/bin/env python3
"""Reproduce the preserved turn34 object without writing into Don's tree."""
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
DON = Path('/Users/ataeff/arianna/netta-don-turn34-20261003')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(name, command, cwd=ROOT):
    print(name, flush=True)
    with (HERE/(name+'.stdout')).open('xb') as out, (HERE/(name+'.stderr')).open('xb') as err:
        process = subprocess.run(command, cwd=cwd, stdout=out, stderr=err)
    assert process.returncode == 0, (name, process.returncode)
    return HERE/(name+'.stdout')


def main():
    assert not (HERE/'INCOMING.json').exists()
    run('build34', ['make', '-C', 'turns/turn34', 'all'])
    run('build33', ['make', '-C', 'turns/turn33', 'all'])
    source = (ROOT/'turns/turn33/earned_router.c').read_bytes()
    staged = (ROOT/'turns/turn34/.build/turn33_core.c').read_bytes()
    old = b'int main(int argc, char **argv) {'
    new = b'int turn33_router_main(int argc, char **argv) {'
    assert source.count(old) == 2 and staged == source.replace(old, new)
    parent = run('parent_fixture', [str(ROOT/'turns/turn33/earned_router'), '--fixture'])
    included = run('included_fixture', [str(ROOT/'turns/turn34/sel4_router'), '--fixture33'])
    assert parent.read_bytes() == included.read_bytes()
    run('fixture34', [sys.executable, '-B', str(ROOT/'turns/turn34/fixture_check.py'),
                     '--output', str(HERE/'FIXTURE34.json')])
    # Existing sealed input is read-only; only this newly named receipt is created.
    run('reader34', [sys.executable, '-B', str(DON/'turns/turn34/verify.py'),
                    '--output', str(HERE/'VERIFY34.json')])
    assert (HERE/'VERIFY34.json').read_bytes() == (ROOT/'turns/turn34/VERIFY.json').read_bytes()
    raw = []
    for world in range(304, 312):
        folder = DON/'turns/turn34'
        with gzip.open(folder/'results'/f'world{world}'/'switched.turn34.tsv.gz', 'rt') as f:
            rows = list(csv.DictReader(f, delimiter='\t'))
        assert len(rows) == 16384 and [int(r['t']) for r in rows] == list(range(16384))
        delta = math.fsum(float(r['sel4_live'])-float(r['earned2_live']) for r in rows[8192:])
        sizes = {name: (folder/'memory'/f'world{world}'/name).stat().st_size
                 for name in ('bank4_full.bin', 'bank2_full.bin', 'pooled_full.bin')}
        margin = .01*(sizes['bank4_full.bin']-sizes['bank2_full.bin'])
        assert margin == 6.72
        raw.append(dict(world=world, gain=delta, sizes=sizes, margin=margin))
    mean = math.fsum(row['gain'] for row in raw)/8
    receipt = dict(base=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                   incoming='3c780af57b11e716d8d7c127917f52302618a959',
                   fixture33_sha256=sha(parent), reader34_sha256=sha(HERE/'VERIFY34.json'),
                   raw_g4=raw, g4_mean=mean, g4_wins=sum(row['gain'] > 0 for row in raw),
                   g4_pass=mean > 6.72 and sum(row['gain'] > 0 for row in raw) >= 5,
                   first_defective_batch='not located; equality is not certified')
    with (HERE/'INCOMING.json').open('x') as f:
        json.dump(receipt, f, indent=2, sort_keys=True)
        f.write('\n')
    print(json.dumps(receipt, sort_keys=True), flush=True)


if __name__ == '__main__':
    main()
