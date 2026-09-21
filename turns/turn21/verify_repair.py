#!/usr/bin/env python3
"""Independent turn13 source reader plus Decimal replay of 27 authority modes.

Rebuilds the source books, all seven inherited candidate arms, the surface
bijections and this turn's own matchedL sequence with its own code, then steps
the three references and all twenty-four (high, low) grid points through every
byte in Decimal mass arithmetic and checks every quote, hazard choice, odds,
clock value and cap decision the C hand recorded.

This reader refuses a RESULT.json missing any key it depends on, by name, before
it reconstructs anything: turn20's frozen reader died on its own turn's schema,
and that is a build defect rather than an incident.

Post-freeze reader-only repair of the frozen turn21 verify.py, which refused with
'nominated point' after reconstructing all 32 lives. Only the tie test of the
preregistered lexicographic nomination rule is changed; see lexicographic_best.
Nothing else differs, and the frozen reader, result, protocol and thresholds were
not edited. This copy still hashes every frozen file, turn21/verify.py included.
"""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import sys

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
WINDOW = 256
TOL = 1e-7
HORIZON = 32
FIXED, WITNESSED = 'h5l5', 'h10l5'
TIGHT, LOOSE = 'h5l5', 'h16l5'
COMPARISONS = (('witnessed_vs_fixed', WITNESSED, FIXED),
               ('witnessed_vs_witness', WITNESSED, 'witness'),
               ('tight_vs_loose', TIGHT, LOOSE))
PROTOCOL_SHA = '06243c2ba6254e92b4627091eaaa86edaf072b38ac5cd9635611f4c25d30a27a'

# Every top-level RESULT.json key this reader reads. Checked by name before any
# reconstruction runs, so a schema drift is a named refusal in under a second.
REQUIRED_RESULT_KEYS = ('namespace', 'worlds', 'regimes', 'modes', 'references',
                        'grid', 'grid_points', 'protocol_sha256', 'lives',
                        'grid_summary', 'tables', 'quantities', 'frontier',
                        'window_points', 'nominated_point', 'tests',
                        'manipulation', 'window', 'gate_pass')

spec = importlib.util.spec_from_file_location('turn21_independent_turn13_reader',
                                              REPO/'turns/turn13/verify.py')
v13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = v13
spec.loader.exec_module(v13)
v13.HERE, v13.REPO, v13.WORLDS = HERE, REPO, WORLDS

assert len(GRID) == 24 and len(MODES) == 27


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+' finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}, error={error}')
    return error


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            h.update(part)
    return h.hexdigest()


def mass_log2(value):
    need(value > 0, 'positive probability mass')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Authority:
    """One mode's whole life. Grid points carry the witness clock and a cap."""

    def __init__(self, mode):
        need(mode in MODES, 'authority mode')
        self.mode = mode
        self.high, self.low = LEVELS.get(mode, (None, None))
        self.source = self.cold = Decimal('0.5')
        self.shadow = self.clock = self.gain = 0.0
        self.active = False
        self.minimum = self.peak = self.drawdown = 0.0
        self.activation = None
        self.horizons = {}
        self.slow_count = self.fast_count = 0
        self.tail_slow_count = self.tail_fast_count = 0
        self.first_fast_t = self.first_fast_after_move = None
        self.clip_count = 0
        self.low_clip_count = self.high_clip_count = 0
        self.cap_bound_exact = True
        self.max_odds_before = self.max_odds_after = 0.0
        self.odds_at_move = None

    def hazard_is_slow(self):
        if self.mode == 'slow':
            return 1
        if self.mode == 'fast':
            return 0
        return int(not self.clock <= -1.0)

    def cap_level(self, clock):
        return self.low if clock <= -1.0 else self.high

    def step(self, t, cold, candidate, matched, relative):
        need(math.isfinite(cold) and math.isfinite(candidate), 'finite prices')
        need(matched >= 0, 'non-negative match length')
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=mass_log2(self.source/self.cold) if self.active else 0.0,
                      clock_before=self.clock)
        delta = candidate-cold
        slow_used = -1
        increment = 0.0
        clipped = 0
        cap_after = None
        if self.active:
            slow_used = self.hazard_is_slow()
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            joint = self.source*relative
            total = self.cold+joint
            need(total > 0 and joint > 0, 'positive mixed mass')
            increment = 0.0 if candidate == cold else mass_log2(total)
            self.source = (1-hazard)*(joint/total)
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive updated masses')
            near(self.source+self.cold, 1, 'normalized authority', 1e-12)
            if self.mode != 'slow' and self.mode != 'fast' and matched >= 1:
                self.clock = (31.0/32.0)*self.clock+delta
            if self.high is not None:
                limit = self.cap_level(self.clock)
                cap_after = float(limit)
                bound = Decimal(2**limit)/Decimal(2**limit+1)
                if self.source > bound:
                    self.source = bound
                    self.cold = 1-self.source
                    clipped = 1
                    self.clip_count += 1
                    if limit == self.low:
                        self.low_clip_count += 1
                    else:
                        self.high_clip_count += 1
            if slow_used:
                self.slow_count += 1
                if t >= MOVE:
                    self.tail_slow_count += 1
            else:
                self.fast_count += 1
                if self.first_fast_t is None:
                    self.first_fast_t = t
                if t >= MOVE:
                    self.tail_fast_count += 1
                    if self.first_fast_after_move is None:
                        self.first_fast_after_move = t
        self.gain += increment
        self.shadow += delta
        admitted = False
        if not before['active_before'] and self.shadow >= 32.0:
            self.active = True
            self.activation = t+1
            self.source = self.cold = Decimal('0.5')
            self.clock = 0.0
            admitted = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        after_odds = mass_log2(self.source/self.cold) if self.active else 0.0
        self.max_odds_before = max(self.max_odds_before, before['odds_before'])
        self.max_odds_after = max(self.max_odds_after, after_odds)
        if t == MOVE:
            self.odds_at_move = before['odds_before']
        if self.high is not None:
            prior = self.cap_level(before['clock_before'])
            self.cap_bound_exact = self.cap_bound_exact and (
                before['odds_before'] <= float(prior) and
                before['odds_before'] <= float(self.high) and
                (cap_after is None or after_odds <= cap_after+1e-12))
        bound = (10.0 if self.mode == 'fast' else 16.0 if self.high is None else
                 math.log2(2.0**self.high+1.0))
        need(self.minimum >= -1-TOL and self.drawdown <= bound+TOL,
             self.mode+' prefix/interval bound')
        if t+1 in (1024, 4096, MOVE, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=cold if candidate == cold else cold+increment,
                    admitted_after=int(admitted), slow_used=slow_used,
                    clock_after=self.clock, odds_after=after_odds,
                    clipped=clipped, cap_after=cap_after)

    def result(self):
        return dict(gain=self.gain, tail=self.gain-self.horizons[str(MOVE)],
                    early=self.horizons['4096'], minimum=self.minimum,
                    peak=self.peak, max_drawdown=self.drawdown,
                    activation=self.activation, horizons=self.horizons,
                    slow_count=self.slow_count, fast_count=self.fast_count,
                    tail_slow_count=self.tail_slow_count,
                    tail_fast_count=self.tail_fast_count,
                    tail_fast_share=self.tail_fast_count/(N-MOVE),
                    first_fast_t=self.first_fast_t,
                    first_fast_after_move=self.first_fast_after_move,
                    clip_count=self.clip_count, max_odds_before=self.max_odds_before,
                    low_clip_count=self.low_clip_count,
                    high_clip_count=self.high_clip_count,
                    cap_bound_exact=self.cap_bound_exact,
                    max_odds_after=self.max_odds_after, odds_at_move=self.odds_at_move)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, comparison, row, raw, expected, delta):
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
        record[side+'_odds_before'] = expected[mode]['odds_before']
        record[side+'_odds_after'] = float(row[mode+'_odds_after'])
        record[side+'_clipped'] = (int(row[mode+'_clipped'])
                                   if mode in POINTS else 0)
        record[side+'_cap_after'] = (float(LEVELS[mode][1] if w_after <= -1
                                           else LEVELS[mode][0])
                                     if mode in POINTS else None)
    return record


def verify_manipulation():
    rows = []
    for world in WORLDS:
        folder = HERE/'data'/f'world{world}'
        raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
        need(all(len(body) == N for body in raw.values()), 'target lengths')
        seed_bytes = hashlib.sha256(f'{NAMESPACE}|{world}|surface'.encode()).digest()[:8]
        order = list(range(256))
        random.Random(int.from_bytes(seed_bytes, 'big')).shuffle(order)
        need(sorted(order) == list(range(256)), 'surface bijection')
        need(raw['switched'][:MOVE] == raw['recombined'][:MOVE],
             'switched prefix unchanged')
        need(raw['moved_mid'] == raw['recombined'][:MOVE] +
             bytes(order[b] for b in raw['recombined'][MOVE:]),
             'independent surface manipulation')
        hidden = json.loads((folder/'SURFACE.json').read_text())
        need(hidden['seed'] == int.from_bytes(seed_bytes, 'big') and
             hidden['world'] == world and hidden['move'] == MOVE and
             hidden['surface_map'] == order, 'declared surface transformation')
        rows.append(dict(world=world, prefix_identical=True, bijection=True,
                         differing_tail=sum(a != b for a, b in zip(
                             raw['moved_mid'][MOVE:], raw['recombined'][MOVE:]))))
    return rows


def verify_authority(world, regime, inherited, books):
    """Own matchedL recount, own Decimal masses, every column of every mode."""
    tables = books[0]
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    replay = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    states = {m: Authority(m) for m in MODES}
    clocks = {m: empty_clock() for m in CLOCK_MODES}
    exact = dict(equal_price_bytes=0, new_bytes=0, equal_price_exact=True,
                 new_exact=True, inactive_exact=True, carry_exact=True,
                 c_admission_shared=True, first_admission_shared=True)
    column_new_exact = True
    history = []
    samples = {name: dict(help=None, harm=None) for name, _, _ in COMPARISONS}
    previous = {m: 0.0 for m in MODES}
    grid_clock_checks = 0
    max_error = norm_error = 0.0
    with gzip.open(source, 'rt') as f, gzip.open(replay, 'rt') as g:
        candidates = csv.DictReader(f, delimiter='\t')
        authority = csv.DictReader(g, delimiter='\t')
        for t in range(N):
            raw = next(candidates, None)
            row = next(authority, None)
            need(raw is not None and row is not None and
                 int(raw['t']) == int(row['t']) == t, 'complete chronological trace')
            cold = float(raw['logcold'])
            candidate = float(raw['episode_candidate'])
            rank = int(raw['rank'])
            need(float(row['cold']) == cold and float(row['candidate']) == candidate,
                 'identical C/S price inputs')
            length, _, _ = v13.suffix_match(tables['episode'], history)
            need(int(raw['episode_matchedL']) == length,
                 'own matchedL recount against the source trace')
            need(int(row['matched']) == length, 'match length reaching the clock')
            column_new_exact = column_new_exact and raw['new_exact'] == '1'
            norm_error = max(norm_error, float(raw['max_norm_error']))
            if candidate == cold:
                exact['equal_price_bytes'] += 1
            if rank == 0:
                exact['new_bytes'] += 1
                if candidate != cold:
                    exact['new_exact'] = False
            relative = Decimal.from_float(math.exp2(candidate-cold))
            expected = {m: states[m].step(t, cold, candidate, length, relative)
                        for m in MODES}
            common = expected['slow']
            need(int(row['admitted_after']) == int(raw['episode_activated_after']) and
                 int(row['active_before']) == int(raw['episode_active_before']),
                 'inherited first admission')
            fields = {'shadow_before': common['shadow_before'],
                      'active_before': common['active_before'],
                      'admitted_after': common['admitted_after'],
                      'witness_slow_used': expected['witness']['slow_used'],
                      'witness_w_before': expected['witness']['clock_before'],
                      'witness_w_after': expected['witness']['clock_after']}
            for mode in MODES:
                need(expected[mode]['shadow_before'] == common['shadow_before'] and
                     expected[mode]['active_before'] == common['active_before'] and
                     expected[mode]['admitted_after'] == common['admitted_after'],
                     'paired first admission')
                need(expected[mode]['odds_before'] == previous[mode],
                     'carried odds continuity '+mode)
                stored = float(row[mode+'_live'])
                # Condition W4 is a gate condition, so a violation is recorded and
                # preserved here, never raised: hard equality, no tolerance.
                if candidate == cold and stored != cold:
                    exact['equal_price_exact'] = False
                if rank == 0 and stored != cold:
                    exact['new_exact'] = False
                if not common['active_before'] and stored != cold:
                    exact['inactive_exact'] = False
                fields[mode+'_live'] = expected[mode]['live']
                fields[mode+'_odds_after'] = expected[mode]['odds_after']
                if mode in REFERENCES:
                    fields[mode+'_odds_before'] = expected[mode]['odds_before']
                else:
                    fields[mode+'_clipped'] = expected[mode]['clipped']
                    # The grid point's own clock and hazard are rebuilt here and
                    # must equal the witness reference's, not be trusted from C.
                    need(expected[mode]['clock_before'] ==
                         expected['witness']['clock_before'] and
                         expected[mode]['clock_after'] ==
                         expected['witness']['clock_after'] and
                         expected[mode]['slow_used'] ==
                         expected['witness']['slow_used'],
                         'grid/witness clock identity '+mode)
                    grid_clock_checks += 1
                previous[mode] = expected[mode]['odds_after']
            for key, wanted in fields.items():
                if isinstance(wanted, int):
                    need(int(row[key]) == wanted, 'authority flag '+key)
                else:
                    max_error = max(max_error, near(row[key], wanted, 'authority '+key))
            clock = clocks['witness']
            value = expected['witness']['clock_before']
            if clock['minimum'] is None or value < clock['minimum']:
                clock['minimum'], clock['minimum_t'] = value, t
            if clock['maximum'] is None or value > clock['maximum']:
                clock['maximum'], clock['maximum_t'] = value, t
            if t == MOVE:
                clock['at_move'] = value
            if t == N-1:
                clock['final'] = expected['witness']['clock_after']
            if regime in TAIL_REGIMES and t >= MOVE and common['active_before']:
                for comparison in COMPARISONS:
                    name, left, right = comparison
                    delta = float(row[left+'_live'])-float(row[right+'_live'])
                    hold = samples[name]
                    if hold['help'] is None or delta > hold['help']['delta']:
                        hold['help'] = sample_row(world, regime, comparison, row,
                                                  raw, expected, delta)
                    if hold['harm'] is None or delta < hold['harm']['delta']:
                        hold['harm'] = sample_row(world, regime, comparison, row,
                                                  raw, expected, delta)
            history.append(rank)
            if len(history) > HORIZON:
                del history[0]
        need(next(candidates, None) is None and next(authority, None) is None,
             'no extra observations')
    need(grid_clock_checks == N*len(POINTS), 'every grid clock identity checked')
    inherited_keys = ('gain', 'tail', 'horizons', 'minimum', 'peak',
                      'max_drawdown', 'activation')
    v13.compare_tree(inherited['arms']['episode'],
                     {k: states['slow'].result()[k] for k in inherited_keys},
                     'inherited slow authority')
    for mode in MODES:
        need(states[mode].activation == inherited['arms']['episode']['activation'],
             'shared admission byte '+mode)
        exact['first_admission_shared'] = exact['first_admission_shared'] and (
            states[mode].activation == states['slow'].activation)
    life = dict(world=world, regime=regime,
                modes={m: states[m].result() for m in MODES},
                clocks=clocks, exactness=exact, column_new_exact=column_new_exact,
                samples=samples,
                episode_matched_events=inherited['arms']['episode']['matched_events'],
                episode_max_matched_length=inherited['arms']['episode']['max_matched_length'],
                source_trace_sha256=digest(source),
                authority_trace_sha256=digest(replay))
    return life, N*len(MODES), max_error, norm_error


def compare(actual, expected, label):
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+' object')
        for key, value in expected.items():
            need(key in actual, label+' missing '+key)
            compare(actual[key], value, label+'/'+key)
    elif isinstance(expected, list):
        need(isinstance(actual, list) and len(actual) == len(expected), label+' list')
        for i, (a, b) in enumerate(zip(actual, expected)):
            compare(a, b, label+f'/{i}')
    elif isinstance(expected, (bool, str, int)) or expected is None:
        need(actual == expected, label+' exact')
    else:
        near(actual, expected, label)


def lexicographic_best(points, grid):
    """Greatest law-tail mean, then greater law whole-life mean, then grid order.

    The frozen reader wrote this as max() over the tuple key, so two points
    counted as tied only if their means agreed bit for bit and the grid-index
    key could break the tie. The two hands compute the same tail by different
    formulas -- the writer accumulates live-cold directly over t >= 8192, this
    reader subtracts the 8192 horizon from the whole-life gain -- which agree to
    about 1e-13 but not bitwise. On this batch h8l3, h8l4 and h8l5 tie exactly in
    the writer and spread over 4.263e-14 bit here, so the hands disagreed on the
    nominated point's name while agreeing on every number and every gate boolean.
    Points whose means agree within the gate's own TOL are treated as tied; the
    earlier grid index then keeps the nomination, since `points` arrives in grid
    order. A nomination decided on the fourteenth digit is not a nomination.
    """
    best = None
    for point in points:
        if best is None:
            best = point
            continue
        for key in ('law_tail_vs_fast_mean', 'law_whole_vs_fast_mean'):
            if abs(grid[point][key]-grid[best][key]) > TOL:
                if grid[point][key] > grid[best][key]:
                    best = point
                break
    return best


def independent_gate(lives):
    lookup = {(life['world'], life['regime']): life for life in lives}
    need(len(lookup) == len(lives) == 32, 'complete paired worlds')
    mean = lambda values: math.fsum(values)/len(values)

    def read(regime, mode, field):
        return [lookup[w, regime]['modes'][mode][field] for w in WORLDS]

    def gap(regime, mode, other, field):
        return [x-y for x, y in zip(read(regime, mode, field),
                                    read(regime, other, field))]

    slow_early = mean(read('recombined', 'slow', 'early'))
    slow_full = mean(read('recombined', 'slow', 'gain'))
    grid = {}
    for point in POINTS:
        high, low = LEVELS[point]
        law_tail = gap('switched', point, 'fast', 'tail')
        law_whole = gap('switched', point, 'fast', 'gain')
        moved_fast = gap('moved_mid', point, 'fast', 'tail')
        moved_slow = gap('moved_mid', point, 'slow', 'tail')
        recombined_full = read('recombined', point, 'gain')
        early_ratio = (mean(read('recombined', point, 'early'))/slow_early
                       if slow_early > 0 else None)
        full_ratio = (mean(read('recombined', point, 'gain'))/slow_full
                      if slow_full > 0 else None)
        conditions = dict(
            law_tail_vs_fast=mean(law_tail) >= -1,
            law_whole_vs_fast=mean(law_whole) >= 0,
            moved_tail_vs_fast_mean=mean(moved_fast) >= 1,
            moved_tail_vs_fast_wins=sum(x > 0 for x in moved_fast) >= 5,
            moved_tail_vs_slow=mean(moved_slow) >= -3,
            recombined_retention=(early_ratio is not None and early_ratio >= .95 and
                                  full_ratio is not None and full_ratio >= .95 and
                                  all(x > 0 for x in recombined_full)))
        grid[point] = dict(
            point=point, high=high, low=low, index=INDEX[point],
            law_tail_vs_fast=law_tail, law_tail_vs_fast_mean=mean(law_tail),
            law_tail_vs_fast_worlds=sum(x >= -1 for x in law_tail),
            law_whole_vs_fast=law_whole, law_whole_vs_fast_mean=mean(law_whole),
            moved_tail_vs_fast=moved_fast,
            moved_tail_vs_fast_mean=mean(moved_fast),
            moved_tail_vs_fast_wins=sum(x > 0 for x in moved_fast),
            moved_tail_vs_slow=moved_slow,
            moved_tail_vs_slow_mean=mean(moved_slow),
            law_tail_mean=mean(read('switched', point, 'tail')),
            law_whole_mean=mean(read('switched', point, 'gain')),
            moved_tail_mean=mean(read('moved_mid', point, 'tail')),
            recombined_early_mean=mean(read('recombined', point, 'early')),
            recombined_full_mean=mean(read('recombined', point, 'gain')),
            recombined_full=recombined_full,
            early_retention=early_ratio, full_retention=full_ratio,
            seam_level_switched=read('switched', point, 'odds_at_move'),
            seam_level_switched_mean=mean(read('switched', point, 'odds_at_move')),
            seam_level_moved_mean=mean(read('moved_mid', point, 'odds_at_move')),
            related_low_clips=sum(life['modes'][point]['low_clip_count']
                                  for life in lives if life['regime'] != 'unrelated'),
            related_high_clips=sum(life['modes'][point]['high_clip_count']
                                   for life in lives if life['regime'] != 'unrelated'),
            switched_first_fast_after_move=read('switched', point,
                                                'first_fast_after_move'),
            moved_fast_share=read('moved_mid', point, 'tail_fast_share'),
            conditions=conditions, in_window=all(conditions.values()))
    window = [p for p in POINTS if grid[p]['in_window']]
    nominated = lexicographic_best(window, grid) if window else None
    quantities = dict(
        slow_recombined_early_mean=slow_early, slow_recombined_full_mean=slow_full,
        fast_law_tail_mean=mean(read('switched', 'fast', 'tail')),
        fast_law_whole_mean=mean(read('switched', 'fast', 'gain')),
        fast_moved_tail_mean=mean(read('moved_mid', 'fast', 'tail')),
        slow_moved_tail_mean=mean(read('moved_mid', 'slow', 'tail')),
        witness_law_tail_mean=mean(read('switched', 'witness', 'tail')),
        witness_moved_tail_mean=mean(read('moved_mid', 'witness', 'tail')),
        witness_seam_level_switched_mean=mean(read('switched', 'witness',
                                                   'odds_at_move')),
        witness_clock_at_move_switched=[lookup[w, 'switched']['clocks']['witness']['at_move']
                                        for w in WORLDS],
        window_size=len(window),
        best_law_tail_point_overall=lexicographic_best(POINTS, grid))
    tables = {regime: {mode: {field: mean(read(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
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
        w4_shared_first_admission=all(life['exactness']['first_admission_shared'] and
            life['exactness']['c_admission_shared'] and life['exactness']['carry_exact']
            for life in lives),
        w4_unrelated_never_admits=all(
            life['modes'][m]['activation'] is None
            for life in lives if life['regime'] == 'unrelated' for m in MODES),
        w4_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        w4_prefix_and_drawdown=all(life['modes'][m]['minimum'] >= -1-TOL and
            life['modes'][m]['max_drawdown'] <= 16+TOL
            for life in lives for m in MODES),
        w4_distributions_normalized=all(0 <= life['max_norm_error'] <= 1e-8 and
            life['column_new_exact'] for life in lives),
        w4_exact_protected_quotes=all(life['exactness']['equal_price_exact'] and
            life['exactness']['inactive_exact'] and life['exactness']['new_exact'] and
            all(life['modes'][p]['cap_bound_exact'] for p in POINTS)
            for life in lives))
    need(len(tests) == 10, 'ten preregistered conditions')
    return grid, quantities, tables, frontier, window, nominated, tests


def check_schema(claim):
    """Named refusal before any reconstruction: the reader must not die later."""
    need(isinstance(claim, dict), 'RESULT.json object')
    for key in REQUIRED_RESULT_KEYS:
        need(key in claim, 'RESULT.json schema is missing required key: '+key)
    for key in ('lives', 'window_points', 'grid_points', 'worlds', 'regimes',
                'modes', 'references', 'grid', 'manipulation'):
        need(isinstance(claim[key], list), 'RESULT.json list field: '+key)
    for key in ('grid_summary', 'tables', 'quantities', 'frontier', 'tests'):
        need(isinstance(claim[key], dict), 'RESULT.json object field: '+key)
    need(isinstance(claim['gate_pass'], bool), 'RESULT.json boolean field: gate_pass')
    return sorted(REQUIRED_RESULT_KEYS)


def identity():
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA, 'frozen protocol identity')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS) and
         frozen['regimes'] == list(REGIMES) and frozen['modes'] == list(MODES) and
         frozen['grid'] == [list(p) for p in GRID], 'freeze scope')
    for name, wanted in frozen['files'].items():
        need(digest(REPO/name) == wanted, 'frozen file '+name)
    pins = dict(protocol_sha256=PROTOCOL_SHA, reader_sha256=digest(Path(__file__)),
                freeze_sha256=digest(HERE/'FREEZE.json'),
                result_sha256=digest(HERE/'RESULT.json'))
    for name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for path, wanted in manifest.items():
            need(digest(HERE/path) == wanted, 'retained artifact '+path)
        pins[name] = digest(HERE/name)
    return pins


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'verification output already exists')
    claim = json.loads((HERE/'RESULT.json').read_text())
    schema = check_schema(claim)
    pins = identity()
    need(claim['worlds'] == list(WORLDS) and claim['regimes'] == list(REGIMES) and
         claim['modes'] == list(MODES) and claim['references'] == list(REFERENCES) and
         claim['grid'] == [list(p) for p in GRID] and
         claim['grid_points'] == list(POINTS) and
         claim['namespace'] == NAMESPACE and
         claim['protocol_sha256'] == PROTOCOL_SHA, 'claimed scope')
    need(claim['window'] == WINDOW, 'claimed mechanism thresholds')
    surface = verify_manipulation()
    lives = []
    candidate_count = authority_count = 0
    max_error = max_norm = 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            books = v13.verify_books(world)
            for regime in REGIMES:
                inherited, count, error, norm, _ = v13.verify_life(world, regime, books)
                candidate_count += count
                max_error = max(max_error, error)
                max_norm = max(max_norm, norm)
                life, count, error, own_norm = verify_authority(world, regime,
                                                                inherited, books)
                authority_count += count
                max_error = max(max_error, error)
                max_norm = max(max_norm, own_norm)
                life['max_norm_error'] = norm
                lives.append(life)
                print('verified', world, regime, flush=True)
    need(candidate_count == 8*4*N*7, 'all inherited candidate forecasts')
    need(authority_count == 8*4*N*len(MODES), 'all authority forecasts')
    compare(claim['lives'], lives, 'all life statistics')
    compare(claim['manipulation'], surface, 'surface manipulation')
    grid, quantities, tables, frontier, window, nominated, tests = independent_gate(lives)
    compare(claim['grid_summary'], grid, 'grid surface')
    compare(claim['quantities'], quantities, 'gate quantities')
    compare(claim['tables'], tables, 'regime tables')
    compare(claim['frontier'], frontier, 'frontier shape')
    need(claim['window_points'] == window, 'window membership')
    need(claim['nominated_point'] == nominated, 'nominated point')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'material gate reconstruction')
    reader_tests = dict(
        w5_result_schema_accepted=schema == sorted(REQUIRED_RESULT_KEYS),
        w5_all_candidate_arms_rebuilt=candidate_count == 8*4*N*7,
        w5_all_modes_replayed=authority_count == 8*4*N*len(MODES),
        w5_agreement_within_tolerance=max_error <= TOL,
        w5_retained_digests_intact=True)
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  complete_gate_pass=all(tests.values()) and all(reader_tests.values()),
                  grid=[list(p) for p in GRID], grid_points=list(POINTS),
                  candidate_forecasts=candidate_count,
                  authority_forecasts=authority_count,
                  maximum_numeric_error=max_error, maximum_normalization_error=max_norm,
                  worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
                  references=list(REFERENCES), identities=pins,
                  grid_summary=grid, quantities=quantities, tables=tables,
                  frontier=frontier, window_points=window, nominated_point=nominated,
                  tests=tests, reader_tests=reader_tests,
                  required_result_keys=schema,
                  scope='Independent frozen turn13 source books and seven candidates; '
                        'own suffix recount of the episode matchedL sequence; '
                        'independent Decimal mass replay of slow/fast/witness and all '
                        'twenty-four (high, low) grid points, every quote, hazard '
                        'choice, clip, odds and clock value, with each grid point\'s '
                        'clock and hazard rebuilt and compared against the witness '
                        'reference rather than taken from C. HEAD256 bindings and P0 '
                        'are supplied trace inputs; full-vector receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: result[k] for k in
                      ('verification_pass', 'gate_pass', 'complete_gate_pass',
                       'candidate_forecasts', 'authority_forecasts',
                       'maximum_numeric_error', 'maximum_normalization_error',
                       'window_points', 'nominated_point', 'frontier',
                       'tests', 'reader_tests')},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
