#!/usr/bin/env python3
"""One frozen source batch and a preregistered (high, low) ceiling frontier."""
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
WORLDS = tuple(range(216, 224))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
HIGHS = (5, 6, 8, 10, 12, 16)
LOWS = (2, 3, 4, 5)
GRID = tuple((high, low) for high in HIGHS for low in LOWS if low <= high)
POINTS = tuple(f'h{high}l{low}' for high, low in GRID)
LEVELS = dict(zip(POINTS, GRID))
INDEX = {point: i for i, point in enumerate(POINTS)}
REFERENCES = ('slow', 'fast', 'witness')
MODES = REFERENCES + POINTS
CLOCK_MODES = ('witness',)
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-ceiling-frontier-v1'
N = 16384
MOVE = 8192
HORIZONS = (1024, 4096, MOVE, N)
WINDOW = 256
TOL = 1e-7
GZIP_LEVEL = 6
FIXED = 'h5l5'          # turn19's fixed ceiling, as a grid point
WITNESSED = 'h10l5'     # turn20's witnessed ceiling, as a grid point
TIGHT, LOOSE = 'h5l5', 'h16l5'   # the W3 tooth pair at the incumbent low
COMPARISONS = (('witnessed_vs_fixed', WITNESSED, FIXED),
               ('witnessed_vs_witness', WITNESSED, 'witness'),
               ('tight_vs_loose', TIGHT, LOOSE))

assert len(GRID) == 24 and len(MODES) == 27 and len(set(POINTS)) == 24
assert set(GRID) == {(h, l) for h in HIGHS for l in LOWS}
assert FIXED in POINTS and WITNESSED in POINTS and LOOSE in POINTS

spec = importlib.util.spec_from_file_location('turn21_frozen_turn13_experiment',
                                              REPO/'turns/turn13/experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE, t13.REPO = HERE, REPO
t13.WORLDS, t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = (
    'turns/turn21/PROTOCOL.md', 'turns/turn21/experiment.py', 'turns/turn21/verify.py',
    'turns/turn21/authority.h', 'turns/turn21/authority.c', 'turns/turn21/replay.c',
    'turns/turn21/Makefile', 'turns/turn21/render_tables.py',
    'turns/turn21/authority_fixture.c', 'turns/turn21/authority_fixture',
    'turns/turn21/episode', 'turns/turn21/authority_replay',
    'turns/turn20/PROTOCOL.md', 'turns/turn20/authority.h', 'turns/turn20/authority.c',
    'turns/turn20/replay.c', 'turns/turn20/experiment.py', 'turns/turn20/verify_repair.py',
    'turns/turn19/authority.h', 'turns/turn19/authority.c',
    'turns/turn18/authority.h', 'turns/turn18/authority.c',
    'turns/turn13/episode.c', 'turns/turn13/experiment.py', 'turns/turn13/verify.py',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c',
)

RESULT_KEYS = ('namespace', 'worlds', 'regimes', 'modes', 'references', 'grid',
               'grid_points', 'protocol_sha256', 'lives', 'grid_summary', 'tables',
               'quantities', 'frontier', 'window_points', 'nominated_point',
               'tests', 'manipulation', 'window', 'gate_pass',
               'independent_reader_pending')


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
         worlds=WORLDS, regimes=REGIMES, modes=MODES, grid=[list(p) for p in GRID],
         files={name: digest(REPO/name) for name in FROZEN}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES) and frozen['modes'] == list(MODES)
    assert frozen['grid'] == [list(p) for p in GRID]
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


def limits(point, clock):
    """This grid point's next-quote cap under the witness clock value given."""
    high, low = LEVELS[point]
    return float(low if clock <= -1.0 else high)


def sample_row(world, regime, comparison, row, raw, odds_before, delta):
    """One retained charged byte, named by its own trace position."""
    t = int(row['t'])
    name, left, right = comparison
    w_after = float(row['witness_w_after'])
    record = dict(world=world, regime=regime, comparison=name, left=left,
                  right=right, t=t, truth=int(raw['truth']), rank=int(raw['rank']),
                  matchedL=int(row['matched']), cold=float(row['cold']),
                  candidate=float(row['candidate']),
                  hazard='2^-16' if int(row['witness_slow_used']) == 1 else '2^-10',
                  witness_w_before=float(row['witness_w_before']),
                  witness_w_after=w_after, delta=delta)
    for side, mode in (('left', left), ('right', right)):
        record[side+'_live'] = float(row[mode+'_live'])
        record[side+'_odds_before'] = odds_before[mode]
        record[side+'_odds_after'] = float(row[mode+'_odds_after'])
        record[side+'_clipped'] = (int(row[mode+'_clipped'])
                                   if mode in POINTS else 0)
        record[side+'_cap_after'] = (limits(mode, w_after)
                                     if mode in POINTS else None)
    return record


def parse_log(text):
    lines = [line for line in text.strip().split('\n') if line]
    head = dict(pair.split('=', 1) for pair in lines[0].split())
    modes = {}
    for line in lines[1:]:
        fields = dict(pair.split('=', 1) for pair in line.split())
        modes[fields.pop('mode')] = fields
    return head, modes


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
    with gzip.open(target, 'xb', compresslevel=GZIP_LEVEL) as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)
    head, c_modes = parse_log(done.stderr.decode())
    states = {m: empty_stats() for m in MODES}
    clocks = {m: empty_clock() for m in CLOCK_MODES}
    exact = dict(equal_price_bytes=0, new_bytes=0, equal_price_exact=True,
                 new_exact=True, inactive_exact=True, carry_exact=True,
                 c_admission_shared=True, first_admission_shared=True)
    column_new_exact = True
    samples = {name: dict(help=None, harm=None) for name, _, _ in COMPARISONS}
    previous = {m: 0.0 for m in MODES}
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
        w_before = float(row['witness_w_before'])
        w_after = float(row['witness_w_after'])
        shared_clock = int(row['witness_slow_used'])
        if candidate == cold:
            exact['equal_price_bytes'] += 1
        if rank == 0:
            exact['new_bytes'] += 1
            if candidate != cold:
                exact['new_exact'] = False
        live, before = {}, {}
        for mode in MODES:
            value = float(row[mode+'_live'])
            assert math.isfinite(value) and value <= 1e-8
            live[mode] = value
            odds_before = previous[mode]
            if mode in REFERENCES:
                exact['carry_exact'] = exact['carry_exact'] and (
                    float(row[mode+'_odds_before']) == odds_before)
            before[mode] = odds_before
            odds_after = float(row[mode+'_odds_after'])
            if candidate == cold and value != cold:
                exact['equal_price_exact'] = False
            if rank == 0 and value != cold:
                exact['new_exact'] = False
            if not active and value != cold:
                exact['inactive_exact'] = False
            if mode == 'slow':
                assert abs(value-float(raw[t]['episode_live'])) <= TOL
            s = states[mode]
            s['max_odds_before'] = max(s['max_odds_before'], odds_before)
            s['max_odds_after'] = max(s['max_odds_after'], odds_after)
            if t == MOVE:
                s['odds_at_move'] = odds_before
            if mode in POINTS:
                high, low = LEVELS[mode]
                clipped = int(row[mode+'_clipped'])
                limit = limits(mode, w_after)
                s['clip_count'] += clipped
                if clipped:
                    s['low_clip_count' if limit == float(low)
                      else 'high_clip_count'] += 1
                s['cap_bound_exact'] = s['cap_bound_exact'] and (
                    odds_before <= limits(mode, w_before) and
                    odds_before <= float(high) and odds_after <= limit and
                    (not clipped or odds_after == limit))
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
            clock = (-1 if shared_clock == -1 else
                     1 if mode == 'slow' else 0 if mode == 'fast' else shared_clock)
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
            previous[mode] = odds_after
        clock = clocks['witness']
        if clock['minimum'] is None or w_before < clock['minimum']:
            clock['minimum'], clock['minimum_t'] = w_before, t
        if clock['maximum'] is None or w_before > clock['maximum']:
            clock['maximum'], clock['maximum_t'] = w_before, t
        if t == MOVE:
            clock['at_move'] = w_before
        if t == N-1:
            clock['final'] = w_after
        if regime in TAIL_REGIMES and t >= MOVE and active:
            for comparison in COMPARISONS:
                name, left, right = comparison
                delta = live[left]-live[right]
                hold = samples[name]
                if hold['help'] is None or delta > hold['help']['delta']:
                    hold['help'] = sample_row(world, regime, comparison, row,
                                              raw[t], before, delta)
                if hold['harm'] is None or delta < hold['harm']['delta']:
                    hold['harm'] = sample_row(world, regime, comparison, row,
                                              raw[t], before, delta)
    assert next(rows, None) is None
    assert int(head['events']) == N and int(head['modes']) == len(MODES)
    assert int(head['state_bytes']) == 32
    assert int(head['carry_checks']) == N*len(MODES)
    assert int(head['clock_identity_checks']) == N*len(POINTS)
    assert int(head['odds_bound_checks']) == N*len(POINTS)
    assert set(c_modes) == set(MODES)
    admissions = set()
    for mode in MODES:
        s, c = states[mode], c_modes[mode]
        assert s['activation'] == inherited['arms']['episode']['activation']
        s['early'] = s['horizons']['4096']
        s['tail_fast_share'] = s['tail_fast_count']/(N-MOVE)
        exact['first_admission_shared'] = exact['first_admission_shared'] and (
            s['activation'] == states['slow']['activation'])
        # The C hand accumulated its own per-mode statistics from its own state;
        # these must agree with the columns parsed back here.
        assert abs(float(c['gain'])-s['gain']) <= 1e-9, mode
        assert abs(float(c['minimum'])-s['minimum']) <= 1e-9, mode
        assert abs(float(c['drawdown'])-s['max_drawdown']) <= 1e-9, mode
        assert int(c['slow']) == s['slow_count'], mode
        assert int(c['fast']) == s['fast_count'], mode
        assert int(c['clipped']) == s['clip_count'], mode
        admissions.add(int(c['admission']))
    exact['c_admission_shared'] = len(admissions) == 1
    return dict(world=world, regime=regime, modes=states, clocks=clocks,
                exactness=exact, column_new_exact=column_new_exact,
                samples=samples,
                episode_matched_events=inherited['arms']['episode']['matched_events'],
                episode_max_matched_length=inherited['arms']['episode']['max_matched_length'],
                source_trace_sha256=digest(source),
                authority_trace_sha256=digest(target),
                max_norm_error=inherited['max_norm_error'])


def surface(lives):
    """Every per-grid-point quantity the frontier question asks for."""
    by = {(x['world'], x['regime']): x for x in lives}
    avg = lambda xs: math.fsum(xs)/len(xs)

    def pick(regime, mode, field):
        return [by[w, regime]['modes'][mode][field] for w in WORLDS]

    def gap(regime, mode, other, field):
        return [a-b for a, b in zip(pick(regime, mode, field),
                                    pick(regime, other, field))]

    slow_early = avg(pick('recombined', 'slow', 'early'))
    slow_full = avg(pick('recombined', 'slow', 'gain'))
    grid = {}
    for point in POINTS:
        high, low = LEVELS[point]
        law_tail_fast = gap('switched', point, 'fast', 'tail')
        law_whole_fast = gap('switched', point, 'fast', 'gain')
        moved_tail_fast = gap('moved_mid', point, 'fast', 'tail')
        moved_tail_slow = gap('moved_mid', point, 'slow', 'tail')
        recombined_full = pick('recombined', point, 'gain')
        early_ratio = (avg(pick('recombined', point, 'early'))/slow_early
                       if slow_early > 0 else None)
        full_ratio = (avg(pick('recombined', point, 'gain'))/slow_full
                      if slow_full > 0 else None)
        conditions = dict(
            law_tail_vs_fast=avg(law_tail_fast) >= -1,
            law_whole_vs_fast=avg(law_whole_fast) >= 0,
            moved_tail_vs_fast_mean=avg(moved_tail_fast) >= 1,
            moved_tail_vs_fast_wins=sum(x > 0 for x in moved_tail_fast) >= 5,
            moved_tail_vs_slow=avg(moved_tail_slow) >= -3,
            recombined_retention=(early_ratio is not None and early_ratio >= .95 and
                                  full_ratio is not None and full_ratio >= .95 and
                                  all(x > 0 for x in recombined_full)))
        grid[point] = dict(
            point=point, high=high, low=low, index=INDEX[point],
            law_tail_vs_fast=law_tail_fast,
            law_tail_vs_fast_mean=avg(law_tail_fast),
            law_tail_vs_fast_worlds=sum(x >= -1 for x in law_tail_fast),
            law_whole_vs_fast=law_whole_fast,
            law_whole_vs_fast_mean=avg(law_whole_fast),
            moved_tail_vs_fast=moved_tail_fast,
            moved_tail_vs_fast_mean=avg(moved_tail_fast),
            moved_tail_vs_fast_wins=sum(x > 0 for x in moved_tail_fast),
            moved_tail_vs_slow=moved_tail_slow,
            moved_tail_vs_slow_mean=avg(moved_tail_slow),
            law_tail_mean=avg(pick('switched', point, 'tail')),
            law_whole_mean=avg(pick('switched', point, 'gain')),
            moved_tail_mean=avg(pick('moved_mid', point, 'tail')),
            recombined_early_mean=avg(pick('recombined', point, 'early')),
            recombined_full_mean=avg(pick('recombined', point, 'gain')),
            recombined_full=recombined_full,
            early_retention=early_ratio, full_retention=full_ratio,
            seam_level_switched=pick('switched', point, 'odds_at_move'),
            seam_level_switched_mean=avg(pick('switched', point, 'odds_at_move')),
            seam_level_moved_mean=avg(pick('moved_mid', point, 'odds_at_move')),
            related_low_clips=sum(x['modes'][point]['low_clip_count']
                                  for x in lives if x['regime'] != 'unrelated'),
            related_high_clips=sum(x['modes'][point]['high_clip_count']
                                   for x in lives if x['regime'] != 'unrelated'),
            switched_first_fast_after_move=pick('switched', point,
                                                'first_fast_after_move'),
            moved_fast_share=pick('moved_mid', point, 'tail_fast_share'),
            conditions=conditions, in_window=all(conditions.values()))
    quantities = dict(
        slow_recombined_early_mean=slow_early, slow_recombined_full_mean=slow_full,
        fast_law_tail_mean=avg(pick('switched', 'fast', 'tail')),
        fast_law_whole_mean=avg(pick('switched', 'fast', 'gain')),
        fast_moved_tail_mean=avg(pick('moved_mid', 'fast', 'tail')),
        slow_moved_tail_mean=avg(pick('moved_mid', 'slow', 'tail')),
        witness_law_tail_mean=avg(pick('switched', 'witness', 'tail')),
        witness_moved_tail_mean=avg(pick('moved_mid', 'witness', 'tail')),
        witness_seam_level_switched_mean=avg(pick('switched', 'witness',
                                                  'odds_at_move')),
        witness_clock_at_move_switched=[by[w, 'switched']['clocks']['witness']['at_move']
                                        for w in WORLDS])
    tables = {regime: {mode: {field: avg(pick(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
    return grid, quantities, tables


def summarize(lives):
    grid, quantities, tables = surface(lives)
    window = [p for p in POINTS if grid[p]['in_window']]
    nominated = max(window, key=lambda p: (grid[p]['law_tail_vs_fast_mean'],
                                           grid[p]['law_whole_vs_fast_mean'],
                                           -INDEX[p])) if window else None
    quantities['window_size'] = len(window)
    quantities['best_law_tail_point_overall'] = max(
        POINTS, key=lambda p: (grid[p]['law_tail_vs_fast_mean'],
                               grid[p]['law_whole_vs_fast_mean'], -INDEX[p]))
    law_rows = {low: grid[f'h5l{low}']['law_tail_mean'] >=
                     grid[f'h16l{low}']['law_tail_mean'] for low in LOWS}
    moved_rows = {low: grid[f'h16l{low}']['moved_tail_mean'] >=
                       grid[f'h5l{low}']['moved_tail_mean'] for low in LOWS}
    frontier = dict(
        law_tail_rows={str(k): v for k, v in law_rows.items()},
        law_tail_rows_true=sum(law_rows.values()),
        moved_tail_rows={str(k): v for k, v in moved_rows.items()},
        moved_tail_rows_true=sum(moved_rows.values()),
        law_tail_endpoints={str(low): [grid[f'h5l{low}']['law_tail_mean'],
                                       grid[f'h16l{low}']['law_tail_mean']]
                            for low in LOWS},
        moved_tail_endpoints={str(low): [grid[f'h5l{low}']['moved_tail_mean'],
                                         grid[f'h16l{low}']['moved_tail_mean']]
                              for low in LOWS})
    tests = dict(
        w1_window_exists=len(window) > 0,
        w2_nominated_point_robust=nominated is not None and
            grid[nominated]['law_tail_vs_fast_worlds'] >= 5,
        w3_law_tail_favours_tight_high=frontier['law_tail_rows_true'] >= 3,
        w3_moved_tail_favours_loose_high=frontier['moved_tail_rows_true'] >= 3,
        w4_shared_first_admission=all(x['exactness']['first_admission_shared'] and
            x['exactness']['c_admission_shared'] and x['exactness']['carry_exact']
            for x in lives),
        w4_unrelated_never_admits=all(
            x['modes'][m]['activation'] is None
            for x in lives if x['regime'] == 'unrelated' for m in MODES),
        w4_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        w4_prefix_and_drawdown=all(x['modes'][m]['minimum'] >= -1-TOL and
            x['modes'][m]['max_drawdown'] <= 16+TOL
            for x in lives for m in MODES),
        w4_distributions_normalized=all(0 <= x['max_norm_error'] <= 1e-8 and
            x['column_new_exact'] for x in lives),
        w4_exact_protected_quotes=all(x['exactness']['equal_price_exact'] and
            x['exactness']['inactive_exact'] and x['exactness']['new_exact'] and
            all(x['modes'][p]['cap_bound_exact'] for p in POINTS)
            for x in lives))
    assert len(tests) == 10
    return grid, quantities, tables, frontier, window, nominated, tests


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
        print(item['world'], item['regime'],
              'fast_tail', round(life['modes']['fast']['tail'], 3),
              'h5l5_tail', round(life['modes']['h5l5']['tail'], 3),
              'h10l5_tail', round(life['modes']['h10l5']['tail'], 3),
              'h16l5_tail', round(life['modes']['h16l5']['tail'], 3),
              'witness_tail', round(life['modes']['witness']['tail'], 3), flush=True)
    grid, quantities, tables, frontier, window, nominated, tests = summarize(lives)
    result = dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
                  references=REFERENCES, grid=[list(p) for p in GRID],
                  grid_points=POINTS, protocol_sha256=digest(HERE/'PROTOCOL.md'),
                  lives=lives, grid_summary=grid, tables=tables,
                  quantities=quantities, frontier=frontier, window_points=window,
                  nominated_point=nominated, tests=tests,
                  manipulation=manipulation(), window=WINDOW,
                  gate_pass=all(tests.values()), independent_reader_pending=True)
    assert set(result) == set(RESULT_KEYS), sorted(set(result) ^ set(RESULT_KEYS))
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps(dict(tests=tests, window_points=window,
                          nominated_point=nominated, frontier=frontier,
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
