#!/usr/bin/env python3
"""One fresh, frozen experiment in parent-relative episode detail."""
import argparse
import concurrent.futures
import csv
import gzip
import importlib.util
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(320, 328))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('hier', 'coarse', 'fine', 'null', 'pooled', 'cold')
NAMESPACE = 'netta-parent-relative-detail-v1'
N, MOVE, EARLY = 16384, 8192, 4096
spec = importlib.util.spec_from_file_location('turn36_source34', ROOT/'turns/turn34/experiment.py')
p34 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p34
spec.loader.exec_module(p34)
for module in (p34, p34.p28, p34.p22, p34.t13):
    module.HERE, module.ROOT, module.REPO = HERE, ROOT, ROOT
    module.WORLDS, module.NAMESPACE = WORLDS, NAMESPACE
    module.REGIMES = REGIMES if module is not p34.t13 else ('recombined', 'unrelated', 'switched')
sha, save = p34.sha, p34.save


def manifest(folder):
    return {str(p.relative_to(HERE)): sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifest(name):
    for path, digest in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/path) == digest, path


def freeze():
    assert all(not (HERE/name).exists() for name in ('data', 'memory', 'results'))
    own = ('PROTOCOL.md', 'hier_router.c', 'Makefile', 'experiment.py', 'verify.py',
           'fixture_check.py', 'preflight.py', 'run_trial.py', 'FIXTURE.json', 'PREFLIGHT.json',
           '.build/hier_router', '.build/turn34_core.c', 'episode')
    names = set(p34.frozen_names())
    names.update('turns/turn36/'+name for name in own)
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
        arms=ARMS, files={name: sha(ROOT/name) for name in sorted(names)}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    for name, digest in frozen['files'].items():
        assert sha(ROOT/name) == digest, name


def generate():
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(p34.p28.generate_world, WORLDS))
    save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(p34.t13.extract_world, WORLDS))
    save(HERE/'EXTRACT_MANIFEST.json', manifest(HERE/'data'))


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for world in WORLDS:
        tapes, _ = p34.t13.source_tapes(world)
        archives, meta = p34.build_memory(tapes)
        assert meta['record_count'] == 24
        null = bytearray(archives['bank4_full.bin'])
        nr, nb = struct.unpack_from('<II', null, 8)
        for j in range(nb):
            start = 16+4*nr+60*j+4
            values = list(struct.unpack_from('<28H', null, start))
            for pair in (0, 2):
                for rank in (1, 3, 5):
                    a, b = pair*7+rank, (pair+1)*7+rank
                    values[a], values[b] = values[b], values[a]
            struct.pack_into('<28H', null, start, *values)
        del archives['permuted4_full.bin']
        archives['null4_full.bin'] = bytes(null)
        meta['bytes'] = {name: len(value) for name, value in archives.items()}
        assert (len(archives['bank4_full.bin']) == len(null) == 1584 and
            len(archives['bank2_full.bin']) == 912 and len(archives['pooled_full.bin']) == 528)
        meta['null_transform'] = 'exchange odd repeat ranks inside each parent pair'
        folder = HERE/'memory'/f'world{world}'
        folder.mkdir(parents=True)
        for name, value in archives.items():
            with (folder/name).open('xb') as f:
                f.write(value)
        save(folder/'BOOKS.json', meta)
        print('learned', world, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def summarize(world, regime):
    stats = {a: dict(gain=0., minimum=0., peak=0., drawdown=0., activation=None, horizons={}) for a in ARMS}
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    best = worst = None
    activity = dict(matched=0, detail_active=0, detail_silent=0, activated=0, revoked=0)
    with gzip.open(HERE/'results'/f'world{world}'/(regime+'.tsv.gz'), 'rt') as f:
        rows = csv.DictReader(f, delimiter='\t')
        for t in range(N):
            row = next(rows)
            assert int(row['t']) == t and int(row['truth']) == raw[t]
            before = list(map(float, row['state_before'].split(',')))
            after = list(map(float, row['state_after'].split(',')))
            if int(row['record']) >= 0:
                active, next_active = max(before[3:7]) > 0, max(after[3:7]) > 0
                activity['matched'] += 1
                activity['detail_active' if active else 'detail_silent'] += 1
                activity['activated'] += not active and next_active
                activity['revoked'] += active and not next_active
            for a in ARMS:
                s = stats[a]
                s['gain'] += float(row[a+'_live'])-float(row['logcold'])
                assert abs(s['gain']-float(row[a+'_gain'])) <= 1e-7
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
                if int(row[a+'_active']) and s['activation'] is None:
                    s['activation'] = t
                if t+1 in (1024, EARLY, MOVE, N):
                    s['horizons'][str(t+1)] = s['gain']
            if t >= MOVE:
                diff = float(row['hier_live'])-float(row['coarse_live'])
                sample = dict(row, difference=diff, raw_hex=raw[max(0,t-16):t+17].hex())
                if best is None or diff > best['difference']: best = sample
                if worst is None or diff < worst['difference']: worst = sample
        assert next(rows, None) is None
    for s in stats.values():
        s['early'], s['tail'] = s['horizons'][str(EARLY)], s['gain']-s['horizons'][str(MOVE)]
    return dict(world=world, regime=regime, arms=stats, activity=activity, raw_help=best, raw_harm=worst)


def run_target(pair):
    w, regime = pair
    folder = HERE/'results'/f'world{w}'
    folder.mkdir(parents=True, exist_ok=True)
    memory = HERE/'memory'/f'world{w}'
    cmd = [str(HERE/'.build/hier_router'), 'predict']+[str(memory/n) for n in
        ('bank4_full.bin', 'bank2_full.bin', 'pooled_full.bin', 'null4_full.bin')]
    path = folder/(regime+'.tsv.gz')
    with (HERE/'data'/f'world{w}'/(regime+'.bin')).open('rb') as inp, gzip.open(path, 'xb') as out:
        proc = subprocess.Popen(cmd, stdin=inp, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while chunk := proc.stdout.read(1 << 20): out.write(chunk)
        stderr = proc.stderr.read()
        rc = proc.wait()
    with path.with_suffix('.stderr').open('xb') as f: f.write(stderr)
    assert rc == 0, (w, regime, rc, stderr)
    print('evaluated', w, regime, flush=True)
    return summarize(w, regime)


def verdict(lives):
    lookup = {(x['world'],x['regime']): x for x in lives}
    mean = lambda xs: math.fsum(xs)/len(xs)
    vals = lambda r,a,k: [lookup[w,r]['arms'][a][k] for w in WORLDS]
    tables = {r: {a: {k: mean(vals(r,a,k)) for k in ('early','gain','tail')} for a in ARMS} for r in REGIMES}
    comparisons = {}
    for r in REGIMES:
        comparisons[r] = {}
        for a in ('coarse','fine','null','pooled'):
            comparisons[r][a] = {}
            for k in ('early','gain','tail'):
                delta = [x-y for x,y in zip(vals(r,'hier',k), vals(r,a,k))]
                comparisons[r][a][k] = dict(mean=mean(delta), wins=sum(x>0 for x in delta), per_world=delta)
    switched = comparisons['switched']
    retention = {k: tables['recombined']['hier'][k]/tables['recombined']['coarse'][k] for k in ('early','gain')}
    conditions = dict(
        H1=switched['coarse']['tail']['mean']>6.72 and switched['coarse']['tail']['wins']>=5 and switched['coarse']['gain']['mean']>=0,
        H2=switched['fine']['tail']['mean']>1 and switched['fine']['tail']['wins']>=5,
        H3=min(retention.values())>=.99 and all(tables['recombined']['coarse'][k]>0 for k in ('early','gain')) and
            all(x>0 for x in vals('recombined','hier','gain')) and comparisons['partial']['coarse']['tail']['mean']>=0,
        H4=switched['null']['tail']['mean']>1 and switched['null']['tail']['wins']>=5,
        H5=False)
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        quantities=dict(comparisons=comparisons, retention=retention,
            portable_bytes=dict(hier=1584,coarse=912,fine=1584,null=1584,pooled=528,cold=0),
            recipient_bytes=dict(hier=1344,coarse=576,fine=960,null=1344,pooled=192,cold=0)),
        conditions=conditions, material_pass=all(v for k,v in conditions.items() if k!='H5'),
        independent_reader_pending=True, gate_pass=False)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json'); check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives)
    save(HERE/'RESULT.json', result)
    print(json.dumps(result['conditions'], sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    args = parser.parse_args()
    if args.stage != 'freeze': check_freeze()
    globals()[args.stage]()
