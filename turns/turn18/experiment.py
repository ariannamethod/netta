#!/usr/bin/env python3
"""One frozen source batch and four authority clocks, the fourth fed by guilt."""
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
WORLDS = tuple(range(192, 200))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'adaptive', 'witness')
CLOCK_MODES = ('adaptive', 'witness')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-witness-clock-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, MOVE, N)
WINDOW = 256
FAST_SHARE = 0.25
TOL = 1e-7

spec = importlib.util.spec_from_file_location('turn18_frozen_turn13_experiment',
                                              REPO/'turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = (
    'turn18/PROTOCOL.md', 'turn18/experiment.py', 'turn18/verify.py',
    'turn18/authority.h', 'turn18/authority.c', 'turn18/replay.c',
    'turn18/Makefile', 'turn18/episode', 'turn18/authority_replay',
    'turn17/PROTOCOL.md', 'turn17/experiment.py', 'turn17/verify.py',
    'turn17/authority.h', 'turn17/authority.c', 'turn17/replay.c',
    'turn17/Makefile',
    'turn16/authority.h', 'turn16/authority.c',
    'turn15/experiment.py', 'turn15/verify.py',
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
    assert not (HERE/'memory').exists()
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
                         differing_tail=sum(a != b for a, b in
                                            zip(moved[MOVE:], base[MOVE:]))))
    return rows


def empty_stats():
    return dict(gain=0., tail=0., early=0., minimum=0., peak=0., max_drawdown=0.,
                activation=None, horizons={}, slow_count=0, fast_count=0,
                tail_slow_count=0, tail_fast_count=0, tail_fast_share=0.,
                first_fast_t=None, first_fast_after_move=None)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, row, raw, delta):
    """One retained charged byte, named by its own trace position."""
    t = int(row['t'])
    record = dict(world=world, regime=regime, t=t, truth=int(raw['truth']),
                  rank=int(raw['rank']), matchedL=int(row['matched']),
                  cold=float(row['cold']), candidate=float(row['candidate']),
                  hazard='2^-16' if int(row['witness_slow_used']) else '2^-10',
                  witness_w_before=float(row['witness_w_before']),
                  witness_w_after=float(row['witness_w_after']),
                  adaptive_e_before=float(row['adaptive_e_before']),
                  delta=delta)
    for mode in MODES:
        record[mode+'_live'] = float(row[mode+'_live'])
    return record


def replay(world, regime, inherited):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    with gzip.open(source, 'rt') as stream:
        raw = list(csv.DictReader(stream, delimiter='\t'))
    assert len(raw) == N
    payload = ''.join(' '.join((r['t'], r['logcold'], r['episode_candidate'],
                                r['episode_matchedL']))+'\n' for r in raw)
    done = subprocess.run([str(HERE/'authority_replay')], input=payload.encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    target = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)
    states = {m: empty_stats() for m in MODES}
    clocks = {m: empty_clock() for m in CLOCK_MODES}
    exact = dict(equal_price_bytes=0, new_bytes=0, equal_price_exact=True,
                 new_exact=True, inactive_exact=True)
    column_new_exact = True
    saving = cost = None
    rows = csv.DictReader(io.StringIO(done.stdout.decode()), delimiter='\t')
    for t in range(N):
        row = next(rows)
        assert int(row['t']) == t
        cold = float(row['cold'])
        candidate = float(row['candidate'])
        matched = int(row['matched'])
        assert cold == float(raw[t]['logcold'])
        assert candidate == float(raw[t]['episode_candidate'])
        assert matched == int(raw[t]['episode_matchedL'])
        assert int(row['admitted_after']) == int(raw[t]['episode_activated_after'])
        assert int(row['active_before']) == int(raw[t]['episode_active_before'])
        assert abs(float(row['shadow_before']) -
                   float(raw[t]['episode_shadow_before'])) <= TOL
        column_new_exact = column_new_exact and raw[t]['new_exact'] == '1'
        rank = int(raw[t]['rank'])
        active = int(row['active_before'])
        if candidate == cold:
            exact['equal_price_bytes'] += 1
        if rank == 0:
            exact['new_bytes'] += 1
            if candidate != cold:
                exact['new_exact'] = False
        live = {}
        for mode in MODES:
            value = float(row[mode+'_live'])
            assert math.isfinite(value) and value <= 1e-8
            live[mode] = value
            if candidate == cold and value != cold:
                exact['equal_price_exact'] = False
            if rank == 0 and value != cold:
                exact['new_exact'] = False
            if not active and value != cold:
                exact['inactive_exact'] = False
            if mode == 'slow':
                assert abs(value-float(raw[t]['episode_live'])) <= TOL
            s = states[mode]
            s['gain'] += value-cold
            if t >= MOVE:
                s['tail'] += value-cold
            s['minimum'] = min(s['minimum'], s['gain'])
            s['peak'] = max(s['peak'], s['gain'])
            s['max_drawdown'] = max(s['max_drawdown'], s['peak']-s['gain'])
            if int(row['admitted_after']):
                assert s['activation'] is None
                s['activation'] = t+1
            if t+1 in HORIZONS:
                s['horizons'][str(t+1)] = s['gain']
            clock = int(row[mode+'_slow_used'])
            if clock == 1:
                s['slow_count'] += 1
                if t >= MOVE:
                    s['tail_slow_count'] += 1
            elif clock == 0:
                s['fast_count'] += 1
                if s['first_fast_t'] is None:
                    s['first_fast_t'] = t
                if t >= MOVE:
                    s['tail_fast_count'] += 1
                    if s['first_fast_after_move'] is None:
                        s['first_fast_after_move'] = t
            else:
                assert clock == -1 and not active
        for mode in CLOCK_MODES:
            field = 'adaptive_e_before' if mode == 'adaptive' else 'witness_w_before'
            value = float(row[field])
            c = clocks[mode]
            if c['minimum'] is None or value < c['minimum']:
                c['minimum'], c['minimum_t'] = value, t
            if c['maximum'] is None or value > c['maximum']:
                c['maximum'], c['maximum_t'] = value, t
            if t == MOVE:
                c['at_move'] = value
            if t == N-1:
                after = 'adaptive_e_after' if mode == 'adaptive' else 'witness_w_after'
                c['final'] = float(row[after])
        if regime in TAIL_REGIMES and t >= MOVE and active:
            if int(row['witness_slow_used']) == 0:
                delta = live['witness']-live['slow']
                if saving is None or delta > saving['delta']:
                    saving = sample_row(world, regime, row, raw[t], delta)
            else:
                delta = live['witness']-live['fast']
                if cost is None or delta < cost['delta']:
                    cost = sample_row(world, regime, row, raw[t], delta)
    assert next(rows, None) is None
    for mode in MODES:
        s = states[mode]
        assert s['activation'] == inherited['arms']['episode']['activation']
        s['early'] = s['horizons']['4096']
        s['tail_fast_share'] = s['tail_fast_count']/(N-MOVE)
    return dict(world=world, regime=regime, modes=states, clocks=clocks,
                exactness=exact, column_new_exact=column_new_exact,
                raw_guilt_saving=saving, raw_hedge_cost=cost,
                episode_matched_events=inherited['arms']['episode']['matched_events'],
                episode_max_matched_length=inherited['arms']['episode']['max_matched_length'],
                source_trace_sha256=digest(source),
                authority_trace_sha256=digest(target),
                max_norm_error=inherited['max_norm_error'])


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    avg = lambda xs: math.fsum(xs)/len(xs)

    def pick(regime, mode, field):
        return [by[w, regime]['modes'][mode][field] for w in WORLDS]

    def pair(regime, other):
        return [a-b for a, b in zip(pick(regime, 'witness', 'tail'),
                                    pick(regime, other, 'tail'))]

    moved_fast = pair('moved_mid', 'fast')
    moved_slow = pair('moved_mid', 'slow')
    law_fast = pair('switched', 'fast')
    law_slow = pair('switched', 'slow')
    law_adaptive = pair('switched', 'adaptive')
    slow_early = avg(pick('recombined', 'slow', 'early'))
    slow_full = avg(pick('recombined', 'slow', 'gain'))
    first_fast = pick('switched', 'witness', 'first_fast_after_move')
    offsets = [None if x is None else x-MOVE for x in first_fast]
    prompt = [x is not None and x < WINDOW for x in offsets]
    shares = pick('moved_mid', 'witness', 'tail_fast_share')
    quiet = [x <= FAST_SHARE for x in shares]
    quantities = dict(
        moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
        law_vs_fast=law_fast, law_vs_slow=law_slow, law_vs_adaptive=law_adaptive,
        early_retention=avg(pick('recombined', 'witness', 'early'))/slow_early
            if slow_early > 0 else None,
        full_retention=avg(pick('recombined', 'witness', 'gain'))/slow_full
            if slow_full > 0 else None,
        recombined_full=pick('recombined', 'witness', 'gain'),
        switched_first_fast_after_move=first_fast,
        switched_first_fast_offsets=offsets,
        switched_prompt_worlds=sum(prompt),
        moved_fast_share=shares, moved_quiet_worlds=sum(quiet))
    tables = {regime: {mode: {field: avg(pick(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
    tests = dict(
        c1_moved_vs_fast_mean=avg(moved_fast) > 1,
        c1_moved_vs_fast_wins=sum(x > 0 for x in moved_fast) >= 5,
        c1_moved_vs_slow_retention=avg(moved_slow) >= -3,
        c2_law_vs_fast_retention=avg(law_fast) >= -1,
        c2_law_vs_slow_mean=avg(law_slow) > 1,
        c2_law_vs_slow_wins=sum(x > 0 for x in law_slow) >= 5,
        c3_law_vs_adaptive_mean=avg(law_adaptive) > 1,
        c4_recombined_early_retention=quantities['early_retention'] is not None and
            quantities['early_retention'] >= .95,
        c4_recombined_full_retention=quantities['full_retention'] is not None and
            quantities['full_retention'] >= .95,
        c4_recombined_full_positive=all(x > 0 for x in quantities['recombined_full']),
        c5_switched_first_fast_within_256=sum(prompt) >= 6,
        c6_moved_fast_share_at_most_quarter=sum(quiet) >= 6,
        c7_first_admission_shared=all(len({x['modes'][m]['activation']
                                           for m in MODES}) == 1 for x in lives),
        c7_unrelated_never_admits=all(by[w, 'unrelated']['modes'][m]['activation']
                                      is None for w in WORLDS for m in MODES),
        c7_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        c7_prefix_bound=all(s['minimum'] >= -1-TOL for x in lives
                            for s in x['modes'].values()),
        c7_drawdown_bound=all(s['max_drawdown'] <= 16+TOL for x in lives
                              for s in x['modes'].values()),
        c7_distributions_normalized=all(0 <= x['max_norm_error'] <= 1e-8 and
                                        x['column_new_exact'] for x in lives),
        c8_equal_price_exact=all(x['exactness']['equal_price_exact'] and
                                 x['exactness']['inactive_exact'] for x in lives),
        c8_protected_new_exact=all(x['exactness']['new_exact'] for x in lives))
    assert len(tests) == 20
    return quantities, tables, tests


def evaluate():
    check_freeze()
    check_manifests('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(t13.run_target, w, r) for w in WORLDS for r in REGIMES]
        inherited = [f.result() for f in futures]
    lives = []
    for item in inherited:
        life = replay(item['world'], item['regime'], item)
        lives.append(life)
        print(item['world'], item['regime'], 'witness',
              life['modes']['witness']['gain'], life['modes']['witness']['tail'],
              'first_fast_after_move',
              life['modes']['witness']['first_fast_after_move'],
              'tail_fast', life['modes']['witness']['tail_fast_count'], flush=True)
    quantities, tables, tests = summarize(lives)
    result = dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
                  protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
                  quantities=quantities, tables=tables, tests=tests,
                  manipulation=manipulation(), window=WINDOW,
                  fast_share_bound=FAST_SHARE,
                  gate_pass=all(tests.values()), independent_reader_pending=True)
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps(dict(quantities=quantities, tests=tests,
                          gate_pass=result['gate_pass']), indent=2, sort_keys=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage',
                        choices=('freeze', 'generate', 'extract', 'learn', 'evaluate'))
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
