#!/usr/bin/env python3
"""One earned return: fixed source machinery, new C authority, one fresh batch."""
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
WORLDS = tuple(range(176, 184))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
MODES = ('slow', 'fast', 'budget', 'return')
NAMESPACE = 'netta-earned-return-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, 8192, N)

spec = importlib.util.spec_from_file_location('turn16_source', REPO/'turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE
# The inherited generator emits its three original regimes. moved_mid is derived.

FROZEN = (
    'turn16/PROTOCOL.md', 'turn16/experiment.py', 'turn16/verify.py',
    'turn16/authority.h', 'turn16/authority.c', 'turn16/replay.c',
    'turn16/Makefile', 'turn16/episode', 'turn16/authority_replay',
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
         scope='Generator only; never supplied to a predictor or authority.'))
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
                         differing_tail=sum(a != b for a, b in
                                            zip(moved[MOVE:], base[MOVE:]))))
    return rows


def empty_stats():
    return dict(gain=0., candidate_gain=0., tail=0., minimum=0., peak=0.,
                max_drawdown=0., activation=None, horizons={}, returns=[])


def replay(world, regime, inherited):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    with gzip.open(source, 'rt') as stream:
        raw = list(csv.DictReader(stream, delimiter='\t'))
    assert len(raw) == N
    payload = ''.join(' '.join([row['t'], row['logcold']] +
                              [row[a+'_candidate'] for a in ARMS]) + '\n' for row in raw)
    done = subprocess.run([str(HERE/'authority_replay')], input=payload.encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    target = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)
    states = {m: {a: empty_stats() for a in ARMS} for m in MODES}
    events = []
    rows = csv.DictReader(io.StringIO(done.stdout.decode()), delimiter='\t')
    for t in range(N):
        for arm in ARMS:
            row = next(rows)
            assert int(row['t']) == t and row['arm'] == arm
            cold, candidate = float(row['cold']), float(row['candidate'])
            assert cold == float(raw[t]['logcold'])
            assert candidate == float(raw[t][arm+'_candidate'])
            assert int(row['admitted_after']) == int(raw[t][arm+'_activated_after'])
            assert int(row['active_before']) == int(raw[t][arm+'_active_before'])
            assert abs(float(row['shadow_before']) -
                       float(raw[t][arm+'_shadow_before'])) <= 1e-7
            for mode in MODES:
                live = float(row[mode+'_live'])
                assert math.isfinite(live) and live <= 1e-8
                if candidate == cold:
                    assert live == cold
                if mode == 'slow':
                    assert abs(live-float(raw[t][arm+'_live'])) <= 1e-7
                s = states[mode][arm]
                delta = live-cold
                s['gain'] += delta
                s['candidate_gain'] += candidate-cold
                if t >= MOVE:
                    s['tail'] += delta
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['max_drawdown'] = max(s['max_drawdown'], s['peak']-s['gain'])
                if int(row['admitted_after']):
                    assert s['activation'] is None
                    s['activation'] = t+1
                if t+1 in HORIZONS:
                    s['horizons'][str(t+1)] = s['gain']
            if int(row['return_event']):
                states['return'][arm]['returns'].append(t+1)
                events.append(dict(
                    arm=arm, t=t, next_byte=t+1, truth=int(raw[t]['truth']),
                    rank=int(raw[t]['rank']), cold=cold, candidate=candidate,
                    matched_length=(int(raw[t][arm+'_matchedL']) if arm != 'row' else None),
                    old_odds=float(row['return_odds_before']),
                    support_before=float(row['return_support_before']),
                    support_at_return=max(0., float(row['return_support_before'])+candidate-cold),
                    live=float(row['return_live']), fast_live=float(row['fast_live']),
                    budget_live=float(row['budget_live']), slow_live=float(row['slow_live']),
                    odds_after=float(row['return_odds_after'])))
    assert next(rows, None) is None
    for mode in MODES:
        for arm in ARMS:
            assert len(states[mode][arm]['returns']) <= 1
            assert states[mode][arm]['activation'] == inherited['arms'][arm]['activation']
    return dict(world=world, regime=regime, modes=states, return_events=events,
                source_trace_sha256=digest(source), authority_trace_sha256=digest(target),
                max_norm_error=inherited['max_norm_error'])


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    mean = lambda xs: math.fsum(xs)/len(xs)
    def pick(regime, mode, field, arm='episode'):
        if field.startswith('h'):
            return [by[w, regime]['modes'][mode][arm]['horizons'][field[1:]] for w in WORLDS]
        return [by[w, regime]['modes'][mode][arm][field] for w in WORLDS]
    def diff(regime, other):
        return [a-b for a, b in zip(pick(regime, 'return', 'tail'), pick(regime, other, 'tail'))]
    def retention(regime, field, other):
        ref = mean(pick(regime, other, field))
        return mean(pick(regime, 'return', field))/ref if ref > 0 else None
    quantities = dict(
        surface_vs_budget=diff('moved_mid', 'budget'),
        surface_vs_fast=diff('moved_mid', 'fast'),
        surface_vs_slow=diff('moved_mid', 'slow'),
        law_vs_fast=diff('switched', 'fast'),
        law_vs_slow=diff('switched', 'slow'),
        early_retention=retention('recombined', 'h4096', 'slow'),
        full_retention=retention('recombined', 'gain', 'slow'),
        switched_retention=retention('switched', 'gain', 'fast'),
        full_positive=sum(x > 0 for x in pick('recombined', 'return', 'gain')),
        early_positive=sum(x > 0 for x in pick('recombined', 'return', 'h4096')),
        early_gain_bpb=mean(pick('recombined', 'return', 'h4096'))/4096,
        surface_returns=sum(t > MOVE for w in WORLDS for t in
                            by[w, 'moved_mid']['modes']['return']['episode']['returns']))
    allstats = [(life, mode, arm, life['modes'][mode][arm])
                for life in lives for mode in MODES for arm in ARMS]
    bounds = all(s['minimum'] >= -1-1e-7 and
                 s['max_drawdown'] <= dict(slow=16, fast=10, budget=10, **{'return':10.5})[m]+1e-7
                 and len(s['returns']) <= 1 for _, m, _, s in allstats)
    nulls = all(s['activation'] is None for life, _, arm, s in allstats
                if arm == 'permuted' or life['regime'] == 'unrelated')
    shared = all(len({life['modes'][m][a]['activation'] for m in MODES}) == 1
                 for life in lives for a in ARMS)
    archive_shared = all(
        (HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size <= 528 and
        json.loads((HERE/'memory'/f'world{w}'/'BOOKS.json').read_text())['source_bytes'] == 4*N
        for w in WORLDS)
    tests = dict(
        surface_vs_budget_mean=mean(quantities['surface_vs_budget']) > 1,
        surface_vs_budget_wins=sum(x > 0 for x in quantities['surface_vs_budget']) >= 5,
        surface_vs_fast_mean=mean(quantities['surface_vs_fast']) > 1,
        surface_vs_slow_retention=mean(quantities['surface_vs_slow']) >= -1,
        law_vs_fast_retention=mean(quantities['law_vs_fast']) >= -1,
        law_vs_slow_mean=mean(quantities['law_vs_slow']) > 1,
        law_vs_slow_wins=sum(x > 0 for x in quantities['law_vs_slow']) >= 5,
        early_retention=quantities['early_retention'] is not None and quantities['early_retention'] >= .9,
        full_retention=quantities['full_retention'] is not None and quantities['full_retention'] >= .9,
        switched_retention=quantities['switched_retention'] is not None and quantities['switched_retention'] >= .9,
        full_positive=quantities['full_positive'] == 8,
        early_gain=quantities['early_gain_bpb'] >= .005,
        early_positive=quantities['early_positive'] >= 6,
        surface_return_observed=quantities['surface_returns'] >= 1,
        archive_shared=archive_shared, first_admission_shared=shared,
        prefix_and_interval_bounds=bounds, null_admissions=nulls)
    return quantities, tests


def evaluate():
    check_freeze()
    check_manifests('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(t13.run_target, w, regime) for w in WORLDS for regime in REGIMES]
        inherited = [f.result() for f in futures]
    lives = []
    for source in inherited:
        life = replay(source['world'], source['regime'], source)
        lives.append(life)
        ep = life['modes']['return']['episode']
        print(source['world'], source['regime'], 'return gain', ep['gain'],
              'tail', ep['tail'], 'returns', ep['returns'], flush=True)
    quantities, tests = summarize(lives)
    manip = manipulation()
    result = dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
                  protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
                  quantities=quantities, tests=tests, manipulation=manip,
                  gate_pass=all(tests.values()), independent_reader_pending=True)
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps(dict(quantities=quantities, tests=tests, gate_pass=result['gate_pass']), indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'learn', 'evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    if stage in ('generate', 'extract'):
        fun = generate_world if stage == 'generate' else t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(fun, WORLDS)))
    elif stage == 'learn':
        check_manifests('DATA_MANIFEST.json')
        for w in WORLDS:
            print('learned', t13.learn_world(w), flush=True)
    else:
        evaluate()
    if stage == 'extract':
        save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))
    if stage == 'learn':
        save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


if __name__ == '__main__':
    main()
