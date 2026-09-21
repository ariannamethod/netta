#!/usr/bin/env python3
"""One frozen source batch, four inherited controls and a relative-support commitment ceiling."""
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
WORLDS = tuple(range(224, 232))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'witness', 'h8l4', 'selfnorm')
CLOCK_MODES = ('witness', 'h8l4', 'selfnorm')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-relative-support-ceiling-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, MOVE, N)
WINDOW = 256
TOL = 1e-7

spec = importlib.util.spec_from_file_location('turn22_frozen_turn13_experiment',
                                              REPO/'turns/turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = (
    'turns/turn22/PROTOCOL.md', 'turns/turn22/INTERFACE.md',
    'turns/turn22/experiment.py', 'turns/turn22/verify.py',
    'turns/turn22/authority.h', 'turns/turn22/authority.c', 'turns/turn22/replay.c',
    'turns/turn22/Makefile', 'turns/turn22/episode', 'turns/turn22/authority_replay',
    'turns/turn22/authority_fixture.c', 'turns/turn22/authority_fixture',
    'turns/turn21/authority.h', 'turns/turn21/authority.c',
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
                max_odds_before=0., max_odds_after=0., odds_at_move=None,
                cap_min=None, cap_max=None, cap_at_move=None, cap_bound_exact=True)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, row, raw, delta):
    sample = dict(world=world, regime=regime, t=int(row['t']),
                  truth=int(raw['truth']), rank=int(raw['rank']),
                  matchedL=int(row['matched']), cold=float(row['cold']),
                  candidate=float(row['candidate']), delta=delta)
    for mode in MODES:
        for field in ('live', 'odds_before', 'odds_after'):
            sample[mode+'_'+field] = float(row[mode+'_'+field])
    for field in ('w_before', 'w_after', 'a_before', 'a_after',
                  'cap_before', 'cap_after'):
        sample['selfnorm_'+field] = float(row['selfnorm_'+field])
    for field in ('cap_before', 'cap_after'):
        sample['h8l4_'+field] = float(row['h8l4_'+field])
    for mode in ('h8l4', 'selfnorm'):
        sample[mode+'_clipped'] = int(row[mode+'_clipped'])
    return sample


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
    support = dict(maximum_a=0., a_at_move=None, final_a=None)
    exact = dict(equal_price_bytes=0, new_bytes=0, equal_price_exact=True,
                 new_exact=True, inactive_exact=True, clock_identical=True,
                 support_recurrence=True, support_domain=True, cap_formula=True)
    column_new_exact = True
    best = worst = None
    rows = csv.DictReader(io.StringIO(done.stdout.decode()), delimiter='\t')
    for t in range(N):
        row = next(rows)
        assert int(row['t']) == t
        cold, candidate = float(row['cold']), float(row['candidate'])
        matched, active = int(row['matched']), int(row['active_before'])
        assert cold == float(raw[t]['logcold'])
        assert candidate == float(raw[t]['episode_candidate'])
        assert matched == int(raw[t]['episode_matchedL'])
        assert active == int(raw[t]['episode_active_before'])
        assert int(row['admitted_after']) == int(raw[t]['episode_activated_after'])
        assert abs(float(row['shadow_before'])-float(raw[t]['episode_shadow_before'])) <= TOL
        column_new_exact &= raw[t]['new_exact'] == '1'
        rank = int(raw[t]['rank'])
        exact['equal_price_bytes'] += candidate == cold
        exact['new_bytes'] += rank == 0
        if rank == 0 and candidate != cold:
            exact['new_exact'] = False
        live = {}
        for mode in MODES:
            value = live[mode] = float(row[mode+'_live'])
            assert math.isfinite(value) and value <= 1e-8
            if candidate == cold and value != cold:
                exact['equal_price_exact'] = False
            if rank == 0 and value != cold:
                exact['new_exact'] = False
            if not active and value != cold:
                exact['inactive_exact'] = False
            if mode == 'slow':
                assert abs(value-float(raw[t]['episode_live'])) <= TOL
            s = states[mode]
            before, after = float(row[mode+'_odds_before']), float(row[mode+'_odds_after'])
            s['max_odds_before'] = max(s['max_odds_before'], before)
            s['max_odds_after'] = max(s['max_odds_after'], after)
            if t == MOVE:
                s['odds_at_move'] = before
            if mode in ('h8l4', 'selfnorm'):
                low, high = float(row[mode+'_cap_before']), float(row[mode+'_cap_after'])
                s['clip_count'] += int(row[mode+'_clipped'])
                s['cap_bound_exact'] &= before <= low+1e-10 and after <= high+1e-10
                if active:
                    s['cap_min'] = low if s['cap_min'] is None else min(s['cap_min'], low)
                    s['cap_max'] = low if s['cap_max'] is None else max(s['cap_max'], low)
                if t == MOVE:
                    s['cap_at_move'] = low
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
                s['tail_slow_count'] += t >= MOVE
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
            exact['clock_identical'] &= all(float(row[mode+'_'+f]) ==
                float(row['witness_'+f]) for f in ('w_before','w_after','slow_used'))
            value = float(row[mode+'_w_before'])
            c = clocks[mode]
            if c['minimum'] is None or value < c['minimum']:
                c['minimum'], c['minimum_t'] = value, t
            if c['maximum'] is None or value > c['maximum']:
                c['maximum'], c['maximum_t'] = value, t
            if t == MOVE:
                c['at_move'] = value
            if t == N-1:
                c['final'] = float(row[mode+'_w_after'])
        a0, a1 = float(row['selfnorm_a_before']), float(row['selfnorm_a_after'])
        wanted = (31./32.)*a0+abs(candidate-cold) if active and matched >= 1 else a0
        if int(row['admitted_after']):
            wanted = 0.
        exact['support_recurrence'] &= abs(a1-wanted) <= 1e-10
        for when, a in (('before', a0), ('after', a1)):
            w = float(row['selfnorm_w_'+when])
            cap = float(row['selfnorm_cap_'+when])
            exact['support_domain'] &= a >= 0 and abs(w) <= a+1e-10 and 0 <= cap <= 16
            exact['cap_formula'] &= abs(cap-16.*max(0., w)/(1.+a)) <= 1e-10
            static = 4. if float(row['h8l4_w_'+when]) <= -1. else 8.
            exact['cap_formula'] &= abs(float(row['h8l4_cap_'+when])-static) <= 1e-10
        support['maximum_a'] = max(support['maximum_a'], a0, a1)
        if t == MOVE:
            support['a_at_move'] = a0
        if t == N-1:
            support['final_a'] = a1
        if regime in TAIL_REGIMES and t >= MOVE and active:
            delta = live['selfnorm']-live['h8l4']
            if best is None or delta > best['delta']+TOL:
                best = sample_row(world, regime, row, raw[t], delta)
            if worst is None or delta < worst['delta']-TOL:
                worst = sample_row(world, regime, row, raw[t], delta)
    assert next(rows, None) is None
    for s in states.values():
        assert s['activation'] == inherited['arms']['episode']['activation']
        s['early'] = s['horizons']['4096']
        s['tail_fast_share'] = s['tail_fast_count']/(N-MOVE)
    return dict(world=world, regime=regime, modes=states, clocks=clocks,
                support=support, exactness=exact, column_new_exact=column_new_exact,
                raw_help=best, raw_harm=worst,
                episode_matched_events=inherited['arms']['episode']['matched_events'],
                episode_max_matched_length=inherited['arms']['episode']['max_matched_length'],
                source_trace_sha256=digest(source), authority_trace_sha256=digest(target),
                max_norm_error=inherited['max_norm_error'])


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    avg = lambda xs: math.fsum(xs)/len(xs)
    def pick(regime, mode, field):
        return [by[w, regime]['modes'][mode][field] for w in WORLDS]
    def pair(regime, mode, other, field):
        return [a-b for a,b in zip(pick(regime,mode,field),pick(regime,other,field))]
    law_fast = pair('switched','selfnorm','fast','tail')
    law_static = pair('switched','selfnorm','h8l4','tail')
    whole_fast = pair('switched','selfnorm','fast','gain')
    whole_static = pair('switched','selfnorm','h8l4','gain')
    moved_fast = pair('moved_mid','selfnorm','fast','tail')
    moved_slow = pair('moved_mid','selfnorm','slow','tail')
    static_fast = pair('switched','h8l4','fast','tail')
    early_ref = avg(pick('recombined','slow','early'))
    full_ref = avg(pick('recombined','slow','gain'))
    q = dict(law_vs_fast=law_fast, law_vs_static=law_static,
             law_whole_vs_fast=whole_fast, law_whole_vs_static=whole_static,
             moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
             early_retention=avg(pick('recombined','selfnorm','early'))/early_ref if early_ref>0 else None,
             full_retention=avg(pick('recombined','selfnorm','gain'))/full_ref if full_ref>0 else None,
             recombined_full=pick('recombined','selfnorm','gain'),
             law_robust_worlds=sum(v>=-1 for v in law_fast),
             static_law_vs_fast=static_fast, static_law_robust_worlds=sum(v>=-1 for v in static_fast),
             static_law_whole_vs_fast=pair('switched','h8l4','fast','gain'))
    tables = {r:{m:{f:avg(pick(r,m,f)) for f in ('gain','tail','early')}
                  for m in MODES} for r in REGIMES}
    tests = dict(
        u01_law_tail=avg(law_fast)>=-1,
        u02_law_robust=q['law_robust_worlds']>=5,
        u03_law_whole=avg(whole_fast)>=0,
        u04_moved_mean=avg(moved_fast)>=1,
        u05_moved_wins=sum(v>0 for v in moved_fast)>=5,
        u06_moved_slow=avg(moved_slow)>=-3,
        u07_early_retention=q['early_retention'] is not None and q['early_retention']>=.95,
        u08_full_retention=q['full_retention'] is not None and q['full_retention']>=.95,
        u09_intact_positive=all(v>0 for v in q['recombined_full']),
        u10_law_vs_static=avg(law_static)>1,
        u11_whole_vs_static=avg(whole_static)>=0,
        s12_admission=all(len({s['activation'] for s in x['modes'].values()})==1 for x in lives),
        s13_unrelated=all(by[w,'unrelated']['modes'][m]['activation'] is None for w in WORLDS for m in MODES),
        s14_archive=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size<=528 for w in WORLDS),
        s15_bounds=all(s['minimum']>=-1-TOL and s['max_drawdown']<=(10 if m=='fast' else 16)+TOL
                      for x in lives for m,s in x['modes'].items()),
        s16_normalized=all(0<=x['max_norm_error']<=1e-8 for x in lives),
        s17_exact_quotes=all(x['column_new_exact'] and all(x['exactness'][k] for k in ('equal_price_exact','new_exact','inactive_exact')) for x in lives),
        s18_support_cap=all(all(x['exactness'][k] for k in ('clock_identical','support_recurrence','support_domain','cap_formula'))
                            and all(s['cap_bound_exact'] for s in x['modes'].values()) for x in lives)
                         and all(by[w,'recombined']['modes']['selfnorm']['clip_count']>0 and
                                 by[w,'recombined']['modes']['selfnorm']['cap_min'] is not None and
                                 by[w,'recombined']['modes']['selfnorm']['cap_max']-
                                 by[w,'recombined']['modes']['selfnorm']['cap_min']>1 for w in WORLDS))
    assert len(tests)==18
    return q,tables,tests


def pack_result(lives, quantities, tables, tests, manipulation):
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
                protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
                quantities=quantities, tables=tables, tests=tests, manipulation=manipulation,
                gate_pass=bool(tests) and all(tests.values()), independent_reader_pending=True)


def evaluate():
    check_freeze()
    check_manifests('DATA_MANIFEST.json','MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        inherited = list(pool.map(lambda wr:t13.run_target(*wr),
                                  [(w,r) for w in WORLDS for r in REGIMES]))
    lives=[]
    for item in inherited:
        life=replay(item['world'],item['regime'],item)
        lives.append(life)
        print(item['world'],item['regime'],life['modes']['selfnorm']['gain'],flush=True)
    q,tables,tests=summarize(lives)
    save(HERE/'RESULT.json',pack_result(lives,q,tables,tests,manipulation()))
    save(HERE/'RESULTS_MANIFEST.json',manifest(HERE/'results'))
    print(json.dumps(dict(quantities=q,tests=tests,gate_pass=all(tests.values())),indent=2))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage',choices=('freeze','generate','extract','learn','evaluate'))
    stage=parser.parse_args().stage
    if stage=='freeze':
        freeze(); return
    check_freeze()
    if stage in ('generate','extract'):
        fun=generate_world if stage=='generate' else t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(fun,WORLDS)))
    elif stage=='learn':
        check_manifests('DATA_MANIFEST.json')
        for w in WORLDS:
            print('learned',t13.learn_world(w),flush=True)
    else:
        evaluate()
    if stage=='extract': save(HERE/'DATA_MANIFEST.json',manifest(HERE/'data'))
    if stage=='learn': save(HERE/'MEMORY_MANIFEST.json',manifest(HERE/'memory'))


if __name__=='__main__':
    main()
