#!/usr/bin/env python3
"""One frozen source batch and a witnessed two-level memory commitment ceiling."""
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
REPO = HERE.parents[1]
WORLDS = tuple(range(208, 216))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'adaptive', 'witness', 'ceiling', 'witnessed_ceiling')
CLOCK_MODES = ('adaptive', 'witness', 'ceiling', 'witnessed_ceiling')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-witnessed-ceiling-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, MOVE, N)
WINDOW = 256
FAST_SHARE = 0.25
TOL = 1e-7

spec = importlib.util.spec_from_file_location('turn20_frozen_turn13_experiment',
                                              REPO/'turns/turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = (
    'turns/turn20/PROTOCOL.md', 'turns/turn20/experiment.py', 'turns/turn20/verify.py',
    'turns/turn20/authority.h', 'turns/turn20/authority.c', 'turns/turn20/replay.c',
    'turns/turn20/Makefile', 'turns/turn20/episode', 'turns/turn20/authority_replay',
    'turns/turn20/authority_fixture.c', 'turns/turn20/authority_fixture',
    'turns/turn18/authority.h', 'turns/turn18/authority.c',
    'turns/turn13/episode.c', 'turns/turn13/experiment.py', 'turns/turn13/verify.py',
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
                first_fast_t=None, first_fast_after_move=None, clip_count=0,
                low_clip_count=0, high_clip_count=0, cap_bound_exact=True,
                max_odds_before=0., max_odds_after=0., odds_at_move=None)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, row, raw, delta):
    """One retained charged byte, named by its own trace position."""
    t = int(row['t'])
    record = dict(world=world, regime=regime, t=t, truth=int(raw['truth']),
                  rank=int(raw['rank']), matchedL=int(row['matched']),
                  cold=float(row['cold']), candidate=float(row['candidate']),
                  hazard='2^-16' if int(row['ceiling_slow_used']) else '2^-10',
                  ceiling_clipped=int(row['ceiling_clipped']),
                  witnessed_ceiling_clipped=int(row['witnessed_ceiling_clipped']),
                  witnessed_ceiling_cap_after=float(row['witnessed_ceiling_cap_after']),
                  ceiling_w_before=float(row['ceiling_w_before']),
                  ceiling_w_after=float(row['ceiling_w_after']),
                  witness_w_before=float(row['witness_w_before']),
                  witness_w_after=float(row['witness_w_after']),
                  adaptive_e_before=float(row['adaptive_e_before']),
                  delta=delta)
    for mode in MODES:
        record[mode+'_live'] = float(row[mode+'_live'])
        record[mode+'_odds_before'] = float(row[mode+'_odds_before'])
        record[mode+'_odds_after'] = float(row[mode+'_odds_after'])
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
                 new_exact=True, inactive_exact=True, witness_clock_identical=True,
                 witnessed_clock_identical=True, first_admission_shared=True)
    column_new_exact = True
    saving = cost = witness_saving = witness_cost = None
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
            s['max_odds_before'] = max(s['max_odds_before'], float(row[mode+'_odds_before']))
            s['max_odds_after'] = max(s['max_odds_after'], float(row[mode+'_odds_after']))
            if t == MOVE:
                s['odds_at_move'] = float(row[mode+'_odds_before'])
            if mode in ('ceiling', 'witnessed_ceiling'):
                s['clip_count'] += int(row[mode+'_clipped'])
            if mode == 'witnessed_ceiling':
                cap = 5.0 if float(row['witnessed_ceiling_w_after']) <= -1 else 10.0
                prior_cap = 5.0 if float(row['witnessed_ceiling_w_before']) <= -1 else 10.0
                s['cap_bound_exact'] = s['cap_bound_exact'] and (
                    float(row['witnessed_ceiling_cap_after']) == cap and
                    float(row[mode+'_odds_before']) <= prior_cap and
                    float(row[mode+'_odds_after']) <= cap)
                if int(row[mode+'_clipped']):
                    s['low_clip_count' if cap == 5.0 else 'high_clip_count'] += 1
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
        exact['witness_clock_identical'] = exact['witness_clock_identical'] and (
            float(row['witness_w_before']) == float(row['ceiling_w_before']) and
            float(row['witness_w_after']) == float(row['ceiling_w_after']) and
            int(row['witness_slow_used']) == int(row['ceiling_slow_used']))
        exact['witnessed_clock_identical'] = exact['witnessed_clock_identical'] and (
            float(row['witness_w_before']) == float(row['witnessed_ceiling_w_before']) and
            float(row['witness_w_after']) == float(row['witnessed_ceiling_w_after']) and
            int(row['witness_slow_used']) == int(row['witnessed_ceiling_slow_used']))
        for mode in CLOCK_MODES:
            field = 'adaptive_e_before' if mode == 'adaptive' else mode+'_w_before'
            value = float(row[field])
            c = clocks[mode]
            if c['minimum'] is None or value < c['minimum']:
                c['minimum'], c['minimum_t'] = value, t
            if c['maximum'] is None or value > c['maximum']:
                c['maximum'], c['maximum_t'] = value, t
            if t == MOVE:
                c['at_move'] = value
            if t == N-1:
                after = 'adaptive_e_after' if mode == 'adaptive' else mode+'_w_after'
                c['final'] = float(row[after])
        if regime in TAIL_REGIMES and t >= MOVE and active:
            delta = live['witnessed_ceiling']-live['ceiling']
            if saving is None or delta > saving['delta']:
                saving = sample_row(world, regime, row, raw[t], delta)
            if cost is None or delta < cost['delta']:
                cost = sample_row(world, regime, row, raw[t], delta)
            delta = live['witnessed_ceiling']-live['witness']
            if witness_saving is None or delta > witness_saving['delta']:
                witness_saving = sample_row(world, regime, row, raw[t], delta)
            if witness_cost is None or delta < witness_cost['delta']:
                witness_cost = sample_row(world, regime, row, raw[t], delta)
    assert next(rows, None) is None
    for mode in MODES:
        s = states[mode]
        assert s['activation'] == inherited['arms']['episode']['activation']
        s['early'] = s['horizons']['4096']
        s['tail_fast_share'] = s['tail_fast_count']/(N-MOVE)
        exact['first_admission_shared'] = exact['first_admission_shared'] and (
            s['activation'] == states['slow']['activation'])
    return dict(world=world, regime=regime, modes=states, clocks=clocks,
                exactness=exact, column_new_exact=column_new_exact,
                raw_cap_help=saving, raw_cap_harm=cost,
                raw_witness_help=witness_saving, raw_witness_harm=witness_cost,
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
        return [a-b for a, b in zip(pick(regime, 'witnessed_ceiling', 'tail'),
                                    pick(regime, other, 'tail'))]

    moved_fast = pair('moved_mid', 'fast')
    moved_slow = pair('moved_mid', 'slow')
    moved_witness = pair('moved_mid', 'witness')
    law_fast = pair('switched', 'fast')
    law_slow = pair('switched', 'slow')
    law_witness = pair('switched', 'witness')
    law_ceiling = pair('switched', 'ceiling')
    slow_early = avg(pick('recombined', 'slow', 'early'))
    slow_full = avg(pick('recombined', 'slow', 'gain'))
    quantities = dict(
        moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
        moved_vs_witness=moved_witness, law_vs_fast=law_fast,
        law_vs_slow=law_slow, law_vs_witness=law_witness,
        law_vs_ceiling=law_ceiling,
        early_retention=avg(pick('recombined', 'witnessed_ceiling', 'early'))/slow_early
            if slow_early > 0 else None,
        full_retention=avg(pick('recombined', 'witnessed_ceiling', 'gain'))/slow_full
            if slow_full > 0 else None,
        recombined_full=pick('recombined', 'witnessed_ceiling', 'gain'),
        switched_first_fast_after_move=pick('switched', 'witnessed_ceiling',
                                            'first_fast_after_move'),
        moved_fast_share=pick('moved_mid', 'witnessed_ceiling', 'tail_fast_share'),
        related_low_clips=sum(x['modes']['witnessed_ceiling']['low_clip_count']
                              for x in lives if x['regime'] != 'unrelated'),
        related_high_clips=sum(x['modes']['witnessed_ceiling']['high_clip_count']
                               for x in lives if x['regime'] != 'unrelated'))
    tables = {regime: {mode: {field: avg(pick(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
    tests = dict(
        surface_fast_mean=avg(moved_fast) > 1,
        surface_fast_wins=sum(x > 0 for x in moved_fast) >= 5,
        surface_slow_retention=avg(moved_slow) >= -3,
        surface_witness_recovery=avg(moved_witness) >= -3,
        law_fast_retention=avg(law_fast) >= -1,
        law_slow_mean=avg(law_slow) > 1,
        law_slow_wins=sum(x > 0 for x in law_slow) >= 5,
        law_witness_mean=avg(law_witness) > 1,
        law_fixed_ceiling_protection=avg(law_ceiling) >= -2,
        recombined_early_retention=quantities['early_retention'] is not None and
            quantities['early_retention'] >= .95,
        recombined_full_retention=quantities['full_retention'] is not None and
            quantities['full_retention'] >= .95,
        recombined_full_positive=all(x > 0 for x in quantities['recombined_full']),
        mechanism_clock_admission=all(x['exactness']['first_admission_shared'] and
            x['exactness']['witnessed_clock_identical'] for x in lives),
        mechanism_caps=all(x['modes']['witnessed_ceiling']['cap_bound_exact'] and
            x['modes']['witnessed_ceiling']['max_odds_before'] <= 10.0 and
            x['modes']['witnessed_ceiling']['max_odds_after'] <= 10.0 for x in lives),
        mechanism_both_levels_clip=quantities['related_low_clips'] > 0 and
            quantities['related_high_clips'] > 0,
        structural_unrelated_never_admits=all(by[w, 'unrelated']['modes'][m]['activation']
                                      is None for w in WORLDS for m in MODES),
        structural_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        structural_distributions_normalized=all(0 <= x['max_norm_error'] <= 1e-8 and
                                        x['column_new_exact'] for x in lives),
        structural_exact_quotes=all(x['exactness']['equal_price_exact'] and
            x['exactness']['inactive_exact'] and x['exactness']['new_exact']
            for x in lives),
        structural_prefix_drawdown=all(
            x['modes']['witnessed_ceiling']['minimum'] >= -1-TOL and
            x['modes']['witnessed_ceiling']['max_drawdown'] <= math.log2(1025)+TOL
            for x in lives))
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
        print(item['world'], item['regime'], 'witnessed_ceiling',
              life['modes']['witnessed_ceiling']['gain'],
              life['modes']['witnessed_ceiling']['tail'],
              'first_fast_after_move',
              life['modes']['witnessed_ceiling']['first_fast_after_move'],
              'tail_fast', life['modes']['witnessed_ceiling']['tail_fast_count'], flush=True)
    quantities, tables, tests = summarize(lives)
    result = dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
                  protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
                  quantities=quantities, tables=tables, tests=tests,
                  manipulation=manipulation(), window=WINDOW,
                  witnessed_ceiling_caps=(5, 10),
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
