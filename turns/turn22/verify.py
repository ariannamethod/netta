#!/usr/bin/env python3
"""Independent turn22 source reader and relative-support probability replay.

Rebuilds the source books, all seven inherited candidate arms, the surface
bijections and this turn's own matchedL sequence with its own code, then steps
slow/fast/witness/h8l4/selfnorm through every byte in Decimal mass arithmetic and
checks every quote, hazard choice, odds and clock value the C hand recorded.
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
WORLDS = tuple(range(224, 232))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'witness', 'h8l4', 'selfnorm')
CLOCK_MODES = ('witness', 'h8l4', 'selfnorm')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-relative-support-ceiling-v1'
N = 16384
MOVE = 8192
TOL = 1e-7
HORIZON = 32
PROTOCOL_SHA = '2a1e5dd3322cc43823188880ce0e7fa0341133a3705f333f417a95adfeba3c19'

spec = importlib.util.spec_from_file_location('turn22_independent_turn13_reader',
                                              REPO/'turns/turn13/verify.py')
v13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = v13
spec.loader.exec_module(v13)
v13.HERE, v13.REPO, v13.WORLDS = HERE, REPO, WORLDS


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


INTERFACE_SHA = '82a6e44a22366fb44a3c29b2b509886492fe68a9eb531e7c3010aa7e3751e0eb'
REQUIRED_RESULT_KEYS = frozenset(('namespace worlds regimes modes protocol_sha256 '
    'lives quantities tables tests manipulation gate_pass independent_reader_pending').split())
COLUMNS = (['t', 'cold', 'candidate', 'matched', 'shadow_before', 'active_before',
            'admitted_after'] +
           [m+'_'+f for m in MODES for f in
            ('live', 'odds_before', 'slow_used', 'odds_after')] +
           [m+'_w_'+f for m in CLOCK_MODES for f in ('before', 'after')] +
           ['selfnorm_a_before', 'selfnorm_a_after'] +
           [m+'_'+f for m in ('h8l4', 'selfnorm') for f in
            ('cap_before', 'cap_after', 'clipped')])


class Authority:
    """Independent probability masses; clocks use the declared binary recurrence."""

    def __init__(self, mode):
        need(mode in MODES, 'authority mode')
        self.mode = mode
        self.source = self.cold = Decimal('0.5')
        self.shadow = self.clock = self.a = self.gain = 0.0
        self.active = False
        self.minimum = self.peak = self.drawdown = 0.0
        self.activation = None
        self.horizons = {}
        self.slow_count = self.fast_count = 0
        self.tail_slow_count = self.tail_fast_count = 0
        self.first_fast_t = self.first_fast_after_move = None
        self.clip_count = 0
        self.max_odds_before = self.max_odds_after = 0.0
        self.odds_at_move = None
        self.cap_min = self.cap_max = self.cap_at_move = None
        self.cap_bound_exact = True

    def hazard_is_slow(self):
        if self.mode == 'slow':
            return 1
        if self.mode == 'fast':
            return 0
        return int(self.clock > -1.0)

    def cap_limit(self):
        if self.mode == 'h8l4':
            return 4.0 if self.clock <= -1.0 else 8.0
        if self.mode == 'selfnorm':
            return 16.0*max(0.0, self.clock)/(1.0+self.a)
        return None

    def step(self, t, cold, candidate, matched):
        need(math.isfinite(cold) and math.isfinite(candidate), 'finite prices')
        need(isinstance(matched, int) and matched >= 0, 'non-negative match length')
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=mass_log2(self.source/self.cold) if self.active else 0.0,
                      clock_before=self.clock, a_before=self.a,
                      cap_before=self.cap_limit())
        delta = candidate-cold
        slow_used, increment, clipped = -1, 0.0, 0
        if self.active:
            slow_used = self.hazard_is_slow()
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            relative = Decimal.from_float(math.exp2(delta))
            joint = self.source*relative
            total = self.cold+joint
            need(joint > 0 and total > 0, 'positive quote mass')
            increment = 0.0 if candidate == cold else mass_log2(total)
            # Price is fixed before updating either the probability or evidence.
            self.source = (1-hazard)*(joint/total)
            if self.mode in CLOCK_MODES and matched >= 1:
                self.clock = (31.0/32.0)*self.clock+delta
                if self.mode == 'selfnorm':
                    self.a = (31.0/32.0)*self.a+abs(delta)
            limit = self.cap_limit()
            if limit is not None:
                # The cap is computed from this reader's own evidence, then
                # imposed in probability space, independently of C log-odds.
                cap_odds = Decimal(2) ** Decimal.from_float(limit)
                cap_mass = cap_odds/(1+cap_odds)
                if self.source > cap_mass:
                    self.source = cap_mass
                    clipped = 1
                    self.clip_count += 1
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive updated masses')
            near(self.source+self.cold, 1, 'normalized authority', 1e-12)
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
            self.clock = self.a = 0.0
            admitted = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        after_odds = mass_log2(self.source/self.cold) if self.active else 0.0
        self.max_odds_before = max(self.max_odds_before, before['odds_before'])
        self.max_odds_after = max(self.max_odds_after, after_odds)
        cap_after = self.cap_limit()
        if before['cap_before'] is not None:
            self.cap_bound_exact = self.cap_bound_exact and (
                before['odds_before'] <= before['cap_before']+1e-10 and
                after_odds <= cap_after+1e-10)
            if before['active_before']:
                self.cap_min = (before['cap_before'] if self.cap_min is None else
                                min(self.cap_min, before['cap_before']))
                self.cap_max = (before['cap_before'] if self.cap_max is None else
                                max(self.cap_max, before['cap_before']))
        if t == MOVE:
            self.odds_at_move = before['odds_before']
            self.cap_at_move = before['cap_before']
        if t+1 in (1024, 4096, MOVE, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=cold if candidate == cold else cold+increment,
                    admitted_after=int(admitted), slow_used=slow_used,
                    clock_after=self.clock, a_after=self.a,
                    odds_after=after_odds, cap_after=cap_after, clipped=clipped)

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
                    max_odds_after=self.max_odds_after, odds_at_move=self.odds_at_move,
                    cap_min=self.cap_min, cap_max=self.cap_max,
                    cap_at_move=self.cap_at_move, cap_bound_exact=self.cap_bound_exact)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, row, raw, delta):
    record = dict(world=world, regime=regime, t=int(row['t']),
                  truth=int(raw['truth']), rank=int(raw['rank']),
                  matchedL=int(row['matched']), cold=float(row['cold']),
                  candidate=float(row['candidate']), delta=delta)
    for mode in MODES:
        for field in ('live', 'odds_before', 'odds_after'):
            record[mode+'_'+field] = float(row[mode+'_'+field])
    for field in ('w_before', 'w_after', 'a_before', 'a_after'):
        record['selfnorm_'+field] = float(row['selfnorm_'+field])
    for mode in ('h8l4', 'selfnorm'):
        for field in ('cap_before', 'cap_after'):
            record[mode+'_'+field] = float(row[mode+'_'+field])
        record[mode+'_clipped'] = int(row[mode+'_clipped'])
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
    """Recount suffixes and replay every field of all five authority streams."""
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    replay = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    states = {m: Authority(m) for m in MODES}
    clocks = {m: empty_clock() for m in CLOCK_MODES}
    support = dict(maximum_a=0.0, a_at_move=None, final_a=None)
    exact = dict(equal_price_bytes=0, new_bytes=0, equal_price_exact=True,
                 new_exact=True, inactive_exact=True, clock_identical=True,
                 support_recurrence=True, support_domain=True, cap_formula=True)
    column_new_exact = True
    history, previous = [], None
    saving = cost = None
    max_error = norm_error = 0.0
    with gzip.open(source, 'rt') as f, gzip.open(replay, 'rt') as g:
        candidates = csv.DictReader(f, delimiter='\t')
        authority = csv.DictReader(g, delimiter='\t')
        need(authority.fieldnames == COLUMNS, 'exact authority TSV schema')
        for t in range(N):
            raw, row = next(candidates, None), next(authority, None)
            need(raw is not None and row is not None and
                 int(raw['t']) == int(row['t']) == t, 'complete chronological trace')
            cold, candidate = float(raw['logcold']), float(raw['episode_candidate'])
            rank = int(raw['rank'])
            need(float(row['cold']) == cold and float(row['candidate']) == candidate,
                 'identical C/S price inputs')
            length, _, _ = v13.suffix_match(books[0]['episode'], history)
            need(int(raw['episode_matchedL']) == int(row['matched']) == length,
                 'own matchedL recount reaching the clock')
            column_new_exact = column_new_exact and raw['new_exact'] == '1'
            norm_error = max(norm_error, float(raw['max_norm_error']))
            if candidate == cold:
                exact['equal_price_bytes'] += 1
            if rank == 0:
                exact['new_bytes'] += 1
                exact['new_exact'] = exact['new_exact'] and candidate == cold
            expected = {m: states[m].step(t, cold, candidate, length) for m in MODES}
            common = expected['slow']
            need(int(row['admitted_after']) == int(raw['episode_activated_after']) and
                 int(row['active_before']) == int(raw['episode_active_before']),
                 'inherited first admission')
            fields = {k: common[k] for k in
                      ('shadow_before', 'active_before', 'admitted_after')}
            for mode in MODES:
                for key in ('shadow_before', 'active_before', 'admitted_after'):
                    need(expected[mode][key] == common[key], 'paired admission')
                for field in ('live', 'odds_before', 'slow_used', 'odds_after'):
                    fields[mode+'_'+field] = expected[mode][field]
                stored = float(row[mode+'_live'])
                if candidate == cold and stored != cold:
                    exact['equal_price_exact'] = False
                if rank == 0 and stored != cold:
                    exact['new_exact'] = False
                if not common['active_before'] and stored != cold:
                    exact['inactive_exact'] = False
                if previous is not None:
                    need(float(row[mode+'_odds_before']) ==
                         float(previous[mode+'_odds_after']), 'carried odds '+mode)
            for mode in CLOCK_MODES:
                for end in ('before', 'after'):
                    fields[mode+'_w_'+end] = expected[mode]['clock_'+end]
                    exact['clock_identical'] = exact['clock_identical'] and (
                        float(row[mode+'_w_'+end]) == float(row['witness_w_'+end]) and
                        expected[mode]['clock_'+end] == expected['witness']['clock_'+end])
                exact['clock_identical'] = exact['clock_identical'] and (
                    int(row[mode+'_slow_used']) == int(row['witness_slow_used']))
                if previous is not None:
                    need(float(row[mode+'_w_before']) == float(previous[mode+'_w_after']),
                         'carried witness '+mode)
            for end in ('before', 'after'):
                fields['selfnorm_a_'+end] = expected['selfnorm']['a_'+end]
            if previous is not None:
                need(float(row['selfnorm_a_before']) ==
                     float(previous['selfnorm_a_after']), 'carried absolute evidence')
            for mode in ('h8l4', 'selfnorm'):
                for field in ('cap_before', 'cap_after', 'clipped'):
                    fields[mode+'_'+field] = expected[mode][field]
                for end in ('before', 'after'):
                    cap = float(row[mode+'_cap_'+end])
                    states[mode].cap_bound_exact = states[mode].cap_bound_exact and (
                        float(row[mode+'_odds_'+end]) <= cap+1e-10)
                    w = float(row[mode+'_w_'+end])
                    if mode == 'h8l4':
                        wanted = 4.0 if w <= -1 else 8.0
                    else:
                        a = float(row['selfnorm_a_'+end])
                        wanted = 16.0*max(0.0, w)/(1.0+a) if a > -1 else math.nan
                        exact['support_domain'] = exact['support_domain'] and (
                            math.isfinite(a) and a >= 0 and abs(w) <= a+1e-10 and
                            math.isfinite(cap) and 0 <= cap <= 16)
                    exact['cap_formula'] = exact['cap_formula'] and abs(cap-wanted) <= 1e-10
            a_before, a_after = float(row['selfnorm_a_before']), float(row['selfnorm_a_after'])
            wanted_a = a_before
            if common['active_before'] and length >= 1:
                wanted_a = (31.0/32.0)*a_before+abs(candidate-cold)
            if common['admitted_after']:
                wanted_a = 0.0
            exact['support_recurrence'] = exact['support_recurrence'] and abs(a_after-wanted_a) <= 1e-10
            for key, wanted in fields.items():
                if isinstance(wanted, int):
                    need(int(row[key]) == wanted, 'authority flag '+key)
                else:
                    max_error = max(max_error, near(row[key], wanted, 'authority '+key))
            for mode in CLOCK_MODES:
                value, clock = expected[mode]['clock_before'], clocks[mode]
                if clock['minimum'] is None or value < clock['minimum']:
                    clock['minimum'], clock['minimum_t'] = value, t
                if clock['maximum'] is None or value > clock['maximum']:
                    clock['maximum'], clock['maximum_t'] = value, t
                if t == MOVE:
                    clock['at_move'] = value
                if t == N-1:
                    clock['final'] = expected[mode]['clock_after']
            support['maximum_a'] = max(support['maximum_a'],
                                        expected['selfnorm']['a_before'],
                                        expected['selfnorm']['a_after'])
            if t == MOVE:
                support['a_at_move'] = expected['selfnorm']['a_before']
            if t == N-1:
                support['final_a'] = expected['selfnorm']['a_after']
            if regime in TAIL_REGIMES and t >= MOVE and common['active_before']:
                delta = float(row['selfnorm_live'])-float(row['h8l4_live'])
                if saving is None or delta > saving['delta']+TOL:
                    saving = sample_row(world, regime, row, raw, delta)
                if cost is None or delta < cost['delta']-TOL:
                    cost = sample_row(world, regime, row, raw, delta)
            history.append(rank)
            if len(history) > HORIZON:
                del history[0]
            previous = row
        need(next(candidates, None) is None and next(authority, None) is None,
             'no extra observations')
    inherited_keys = ('gain', 'tail', 'horizons', 'minimum', 'peak', 'max_drawdown', 'activation')
    v13.compare_tree(inherited['arms']['episode'],
                     {k: states['slow'].result()[k] for k in inherited_keys},
                     'inherited slow authority')
    for mode in MODES:
        need(states[mode].activation == inherited['arms']['episode']['activation'],
             'shared inherited admission '+mode)
    life = dict(world=world, regime=regime,
                modes={m: states[m].result() for m in MODES}, clocks=clocks,
                support=support, exactness=exact, column_new_exact=column_new_exact,
                raw_help=saving, raw_harm=cost,
                episode_matched_events=inherited['arms']['episode']['matched_events'],
                episode_max_matched_length=inherited['arms']['episode']['max_matched_length'],
                source_trace_sha256=digest(source), authority_trace_sha256=digest(replay))
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


def independent_gate(lives):
    lookup = {(life['world'], life['regime']): life for life in lives}
    need(len(lookup) == len(lives) == len(WORLDS)*len(REGIMES), 'complete paired worlds')
    need(set(lookup) == {(w, r) for w in WORLDS for r in REGIMES}, 'exact life identities')
    mean = lambda values: math.fsum(values)/len(values)

    def read(regime, mode, field):
        return [lookup[w, regime]['modes'][mode][field] for w in WORLDS]

    def gap(regime, other, field='tail', chosen='selfnorm'):
        return [x-y for x, y in zip(read(regime, chosen, field), read(regime, other, field))]

    law_fast, law_static = gap('switched', 'fast'), gap('switched', 'h8l4')
    whole_fast, whole_static = gap('switched', 'fast', 'gain'), gap('switched', 'h8l4', 'gain')
    moved_fast, moved_slow = gap('moved_mid', 'fast'), gap('moved_mid', 'slow')
    static_fast = gap('switched', 'fast', chosen='h8l4')
    early_ref, full_ref = mean(read('recombined', 'slow', 'early')), mean(read('recombined', 'slow', 'gain'))
    early_ratio = mean(read('recombined', 'selfnorm', 'early'))/early_ref if early_ref > 0 else None
    full_ratio = mean(read('recombined', 'selfnorm', 'gain'))/full_ref if full_ref > 0 else None
    positive = read('recombined', 'selfnorm', 'gain')
    quantities = dict(law_vs_fast=law_fast, law_vs_static=law_static,
        law_whole_vs_fast=whole_fast, law_whole_vs_static=whole_static,
        moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
        early_retention=early_ratio, full_retention=full_ratio,
        recombined_full=positive, law_robust_worlds=sum(x >= -1 for x in law_fast),
        static_law_vs_fast=static_fast,
        static_law_robust_worlds=sum(x >= -1 for x in static_fast),
        static_law_whole_vs_fast=gap('switched', 'fast', 'gain', chosen='h8l4'))
    tables = {r: {m: {f: mean(read(r, m, f)) for f in ('gain', 'tail', 'early')}
                  for m in MODES} for r in REGIMES}
    tests = dict(
        u01_law_tail=mean(law_fast) >= -1,
        u02_law_robust=quantities['law_robust_worlds'] >= 5,
        u03_law_whole=mean(whole_fast) >= 0,
        u04_moved_mean=mean(moved_fast) >= 1,
        u05_moved_wins=sum(x > 0 for x in moved_fast) >= 5,
        u06_moved_slow=mean(moved_slow) >= -3,
        u07_early_retention=early_ratio is not None and early_ratio >= .95,
        u08_full_retention=full_ratio is not None and full_ratio >= .95,
        u09_intact_positive=all(x > 0 for x in positive),
        u10_law_vs_static=mean(law_static) > 1,
        u11_whole_vs_static=mean(whole_static) >= 0,
        s12_admission=all(len({life['modes'][m]['activation'] for m in MODES}) == 1 for life in lives),
        s13_unrelated=all(lookup[w, 'unrelated']['modes'][m]['activation'] is None
                          for w in WORLDS for m in MODES),
        s14_archive=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size <= 528 for w in WORLDS),
        s15_bounds=all(s['minimum'] >= -1-TOL and
                       s['max_drawdown'] <= (10 if mode == 'fast' else 16)+TOL
                       for life in lives for mode, s in life['modes'].items()),
        s16_normalized=all(0 <= life['max_norm_error'] <= 1e-8 for life in lives),
        s17_exact_quotes=all(life['column_new_exact'] and all(life['exactness'][k]
                             for k in ('equal_price_exact', 'new_exact', 'inactive_exact'))
                             for life in lives),
        s18_support_cap=all(all(life['exactness'][k] for k in
                            ('clock_identical', 'support_recurrence', 'support_domain', 'cap_formula')) and
                           all(life['modes'][m]['cap_bound_exact'] for m in ('h8l4', 'selfnorm'))
                           for life in lives) and all(
                           lookup[w, 'recombined']['modes']['selfnorm']['clip_count'] > 0 and
                           lookup[w, 'recombined']['modes']['selfnorm']['cap_min'] is not None and
                           lookup[w, 'recombined']['modes']['selfnorm']['cap_max']-
                           lookup[w, 'recombined']['modes']['selfnorm']['cap_min'] > 1
                           for w in WORLDS))
    need(len(tests) == 18, 'eighteen preregistered conditions')
    return quantities, tables, tests


def check_schema(claim):
    need(isinstance(claim, dict), 'RESULT.json object')
    for key in sorted(REQUIRED_RESULT_KEYS):
        need(key in claim, 'RESULT.json schema is missing required key: '+key)
    for key in ('worlds', 'regimes', 'modes', 'lives', 'manipulation'):
        need(isinstance(claim[key], list), 'RESULT.json list field: '+key)
    for key in ('quantities', 'tables', 'tests'):
        need(isinstance(claim[key], dict), 'RESULT.json object field: '+key)
    for key in ('gate_pass', 'independent_reader_pending'):
        need(isinstance(claim[key], bool), 'RESULT.json boolean field: '+key)
    for key in ('namespace', 'protocol_sha256'):
        need(isinstance(claim[key], str), 'RESULT.json string field: '+key)
    return sorted(REQUIRED_RESULT_KEYS)


def identity():
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA, 'frozen protocol identity')
    need(digest(HERE/'INTERFACE.md') == INTERFACE_SHA, 'frozen interface identity')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS) and
         frozen['regimes'] == list(REGIMES) and frozen['modes'] == list(MODES), 'freeze scope')
    for name, wanted in frozen['files'].items():
        need(digest(REPO/name) == wanted, 'frozen file '+name)
    pins = dict(protocol_sha256=PROTOCOL_SHA, interface_sha256=INTERFACE_SHA,
                reader_sha256=digest(Path(__file__)), freeze_sha256=digest(HERE/'FREEZE.json'),
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
         claim['modes'] == list(MODES) and claim['namespace'] == NAMESPACE and
         claim['protocol_sha256'] == PROTOCOL_SHA, 'claimed scope')
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
                life, count, error, own_norm = verify_authority(world, regime, inherited, books)
                authority_count += count
                max_error = max(max_error, error)
                max_norm = max(max_norm, own_norm)
                life['max_norm_error'] = norm
                lives.append(life)
                print('verified', world, regime, flush=True)
    need(candidate_count == len(WORLDS)*len(REGIMES)*N*7, 'all inherited candidate forecasts')
    need(authority_count == len(WORLDS)*len(REGIMES)*N*len(MODES), 'all authority forecasts')
    compare(claim['lives'], lives, 'all life statistics')
    compare(claim['manipulation'], surface, 'surface manipulation')
    quantities, tables, tests = independent_gate(lives)
    compare(claim['quantities'], quantities, 'gate quantities')
    compare(claim['tables'], tables, 'regime tables')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'material gate reconstruction')
    reader_tests = dict(schema_accepted=schema == sorted(REQUIRED_RESULT_KEYS),
                        all_candidate_arms_rebuilt=candidate_count == 3670016,
                        all_five_modes_replayed=authority_count == 2621440,
                        agreement_within_tolerance=max_error <= TOL,
                        retained_digests_intact=True)
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  complete_gate_pass=all(tests.values()) and all(reader_tests.values()),
                  candidate_forecasts=candidate_count, authority_forecasts=authority_count,
                  maximum_numeric_error=max_error, maximum_normalization_error=max_norm,
                  worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
                  identities=pins, quantities=quantities, tables=tables, tests=tests,
                  reader_tests=reader_tests, required_result_keys=schema,
                  scope='Frozen independent turn13 source-book reconstruction and seven candidates; '
                        'own suffix recount; independent Decimal mass replay of all five modes, '
                        'own signed/absolute evidence and relative-support cap, every quote and state. '
                        'HEAD256 bindings and P0 are supplied trace inputs; full-vector receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: result[k] for k in ('verification_pass', 'gate_pass', 'complete_gate_pass',
                      'candidate_forecasts', 'authority_forecasts', 'maximum_numeric_error',
                      'maximum_normalization_error', 'tests', 'reader_tests')}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
