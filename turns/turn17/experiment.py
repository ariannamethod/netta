#!/usr/bin/env python3
"""One frozen source batch and three prospective authority clocks."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import random
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(184, 192))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'adaptive')
NAMESPACE = 'netta-prospective-hazard-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, MOVE, N)

spec = importlib.util.spec_from_file_location('turn17_source', REPO/'turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = (
    'turn17/PROTOCOL.md', 'turn17/experiment.py', 'turn17/verify.py',
    'turn17/authority.h', 'turn17/authority.c', 'turn17/replay.c',
    'turn17/Makefile', 'turn17/episode', 'turn17/authority_replay',
    'turn13/episode.c', 'turn13/experiment.py', 'turn13/verify.py',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c',
)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): digest(p) for p in sorted(folder.rglob('*'))
            if p.is_file()}


def freeze():
    assert not (HERE/'data').exists() and not (HERE/'results').exists()
    save(HERE/'FREEZE.json', dict(created_utc=time.time(), namespace=NAMESPACE,
         worlds=WORLDS, regimes=REGIMES, modes=MODES,
         files={name: digest(REPO/name) for name in FROZEN}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES) and frozen['modes'] == list(MODES)
    for name, wanted in frozen['files'].items():
        assert digest(REPO/name) == wanted, name


def check_manifests(*names):
    for name in names:
        for path, wanted in json.loads((HERE/name).read_text()).items():
            assert digest(HERE/path) == wanted, path


def generate_world(world):
    t13.generate_world(world)
    folder = HERE/'data'/f'world{world}'
    order = list(range(256))
    random.Random(t13.seed(world, 'surface')).shuffle(order)
    original = (folder/'recombined.bin').read_bytes()
    moved = original[:MOVE] + bytes(order[b] for b in original[MOVE:])
    with (folder/'moved_mid.bin').open('xb') as stream:
        stream.write(moved)
    save(folder/'SURFACE.json', dict(world=world, surface_map=order,
         seed=t13.seed(world, 'surface'), move=MOVE,
         scope='Generator only; never supplied to predictor or authority.'))
    return world


def manipulation():
    rows = []
    for w in WORLDS:
        d = HERE/'data'/f'world{w}'
        base = (d/'recombined.bin').read_bytes()
        changed = (d/'switched.bin').read_bytes()
        moved = (d/'moved_mid.bin').read_bytes()
        order = list(range(256))
        random.Random(t13.seed(w, 'surface')).shuffle(order)
        declared = json.loads((d/'SURFACE.json').read_text())
        assert declared['surface_map'] == order
        assert len(base) == len(changed) == len(moved) == N
        assert changed[:MOVE] == moved[:MOVE] == base[:MOVE]
        assert moved[MOVE:] == bytes(order[b] for b in base[MOVE:])
        rows.append(dict(world=w, prefix_identical=True, bijection=True,
                         differing_tail=sum(a != b for a,b in
                                            zip(moved[MOVE:], base[MOVE:]))))
    return rows


def empty_stats():
    return dict(gain=0., tail=0., minimum=0., peak=0., max_drawdown=0.,
                activation=None, horizons={}, slow_count=0, fast_count=0)


def replay(world, regime, inherited):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    with gzip.open(source, 'rt') as stream:
        raw = list(csv.DictReader(stream, delimiter='\t'))
    assert len(raw) == N
    payload = ''.join(' '.join((r['t'], r['logcold'], r['episode_candidate']))+'\n'
                      for r in raw)
    done = subprocess.run([str(HERE/'authority_replay')], input=payload.encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    target = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)
    states = {m: empty_stats() for m in MODES}
    rows = csv.DictReader(io.StringIO(done.stdout.decode()), delimiter='\t')
    for t in range(N):
        row = next(rows)
        assert int(row['t']) == t
        cold = float(row['cold'])
        candidate = float(row['candidate'])
        assert cold == float(raw[t]['logcold'])
        assert candidate == float(raw[t]['episode_candidate'])
        assert int(row['admitted_after']) == int(raw[t]['episode_activated_after'])
        assert int(row['active_before']) == int(raw[t]['episode_active_before'])
        assert abs(float(row['shadow_before']) -
                   float(raw[t]['episode_shadow_before'])) <= 1e-7
        for mode in MODES:
            live = float(row[mode+'_live'])
            assert math.isfinite(live) and live <= 1e-8
            if candidate == cold:
                assert live == cold
            if mode == 'slow':
                assert abs(live-float(raw[t]['episode_live'])) <= 1e-7
            s = states[mode]
            s['gain'] += live-cold
            if t >= MOVE:
                s['tail'] += live-cold
            s['minimum'] = min(s['minimum'], s['gain'])
            s['peak'] = max(s['peak'], s['gain'])
            s['max_drawdown'] = max(s['max_drawdown'], s['peak']-s['gain'])
            if int(row['admitted_after']):
                assert s['activation'] is None
                s['activation'] = t+1
            if t+1 in HORIZONS:
                s['horizons'][str(t+1)] = s['gain']
        clock = int(row['adaptive_slow_used'])
        if clock == 1:
            states['adaptive']['slow_count'] += 1
        elif clock == 0:
            states['adaptive']['fast_count'] += 1
        else:
            assert clock == -1
        if int(row['active_before']):
            states['slow']['slow_count'] += 1
            states['fast']['fast_count'] += 1
    assert next(rows, None) is None
    for mode in MODES:
        assert states[mode]['activation'] == inherited['arms']['episode']['activation']
    return dict(world=world, regime=regime, modes=states,
                source_trace_sha256=digest(source), authority_trace_sha256=digest(target),
                max_norm_error=inherited['max_norm_error'])


def summarize(lives):
    by = {(x['world'],x['regime']):x for x in lives}
    avg = lambda xs: math.fsum(xs)/len(xs)
    def pick(regime, mode, field):
        if field == 'early':
            return [by[w,regime]['modes'][mode]['horizons']['4096'] for w in WORLDS]
        return [by[w,regime]['modes'][mode][field] for w in WORLDS]
    def pair(regime, other):
        return [a-b for a,b in zip(pick(regime,'adaptive','tail'),
                                  pick(regime,other,'tail'))]
    surface_fast = pair('moved_mid','fast')
    surface_slow = pair('moved_mid','slow')
    law_fast = pair('switched','fast')
    law_slow = pair('switched','slow')
    slow_early = avg(pick('recombined','slow','early'))
    slow_full = avg(pick('recombined','slow','gain'))
    quantities = dict(surface_vs_fast=surface_fast, surface_vs_slow=surface_slow,
                      law_vs_fast=law_fast, law_vs_slow=law_slow,
                      early_retention=avg(pick('recombined','adaptive','early'))/slow_early
                        if slow_early > 0 else None,
                      full_retention=avg(pick('recombined','adaptive','gain'))/slow_full
                        if slow_full > 0 else None,
                      recombined_full=pick('recombined','adaptive','gain'))
    bounds = all(s['minimum'] >= -1-1e-7 and s['max_drawdown'] <=
                 (10 if m=='fast' else 16)+1e-7 for x in lives
                 for m,s in x['modes'].items())
    shared = all(len({x['modes'][m]['activation'] for m in MODES}) == 1
                 for x in lives)
    null = all(by[w,'unrelated']['modes']['adaptive']['activation'] is None
               for w in WORLDS)
    archive = all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size <= 528
                  for w in WORLDS)
    tests = dict(
        surface_vs_fast_mean=avg(surface_fast) > 1,
        surface_vs_fast_wins=sum(x>0 for x in surface_fast) >= 5,
        surface_vs_slow_retention=avg(surface_slow) >= -3,
        law_vs_fast_retention=avg(law_fast) >= -1,
        law_vs_slow_mean=avg(law_slow) > 1,
        law_vs_slow_wins=sum(x>0 for x in law_slow) >= 5,
        early_retention=quantities['early_retention'] is not None and
                        quantities['early_retention'] >= .95,
        full_retention=quantities['full_retention'] is not None and
                       quantities['full_retention'] >= .95,
        full_positive=all(x>0 for x in quantities['recombined_full']),
        first_admission_shared=shared, episode_null=null,
        archive_cap=archive, prefix_and_interval_bounds=bounds)
    assert len(tests) == 13
    return quantities, tests


def evaluate():
    check_freeze()
    check_manifests('DATA_MANIFEST.json','MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(t13.run_target,w,r) for w in WORLDS for r in REGIMES]
        inherited = [f.result() for f in futures]
    lives = []
    for item in inherited:
        life = replay(item['world'],item['regime'],item)
        lives.append(life)
        print(item['world'], item['regime'], 'adaptive',
              life['modes']['adaptive']['gain'],
              life['modes']['adaptive']['tail'], flush=True)
    quantities,tests = summarize(lives)
    result = dict(namespace=NAMESPACE,worlds=WORLDS,regimes=REGIMES,modes=MODES,
                  protocol_sha256=digest(HERE/'PROTOCOL.md'),lives=lives,
                  quantities=quantities,tests=tests,manipulation=manipulation(),
                  gate_pass=all(tests.values()),independent_reader_pending=True)
    save(HERE/'RESULT.json',result)
    save(HERE/'RESULTS_MANIFEST.json',manifest(HERE/'results'))
    print(json.dumps(dict(quantities=quantities,tests=tests,gate_pass=result['gate_pass']),
                     indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage',choices=('freeze','generate','extract','learn','evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    if stage in ('generate','extract'):
        fun = generate_world if stage=='generate' else t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(fun,WORLDS)))
    elif stage == 'learn':
        check_manifests('DATA_MANIFEST.json')
        for w in WORLDS:
            print('learned',t13.learn_world(w),flush=True)
    else:
        evaluate()
    if stage=='extract':
        save(HERE/'DATA_MANIFEST.json',manifest(HERE/'data'))
    if stage=='learn':
        save(HERE/'MEMORY_MANIFEST.json',manifest(HERE/'memory'))


if __name__=='__main__':
    main()
