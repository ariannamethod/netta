#!/usr/bin/env python3
"""Independent turn13 source reader plus Decimal replay of six authority modes.

Rebuilds the source books, all seven inherited candidate arms, the surface
bijections and this turn's own matchedL sequence with its own code, then steps
slow/fast/adaptive/witness/ceiling/witnessed_ceiling through every byte in Decimal mass arithmetic and
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
WORLDS = tuple(range(208, 216))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'adaptive', 'witness', 'ceiling', 'witnessed_ceiling')
CLOCK_MODES = ('adaptive', 'witness', 'ceiling', 'witnessed_ceiling')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-witnessed-ceiling-v1'
N = 16384
MOVE = 8192
WINDOW = 256
FAST_SHARE = 0.25
TOL = 1e-7
HORIZON = 32
WITNESSED_CEILING_CAPS = [5, 10]
PROTOCOL_SHA = 'cb9ae1dc25751e8618c6827dd66590fb00decd721ffc989421dfb91f8979cc9d'

spec = importlib.util.spec_from_file_location('turn20_independent_turn13_reader',
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


class Authority:
    """One mode's whole life. The clock field belongs to whichever mode owns it."""

    def __init__(self, mode):
        need(mode in MODES, 'authority mode')
        self.mode = mode
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
        if self.mode == 'adaptive':
            return int(self.clock >= 1.0)
        return int(not self.clock <= -1.0)

    def step(self, t, cold, candidate, matched):
        need(math.isfinite(cold) and math.isfinite(candidate), 'finite prices')
        need(matched >= 0, 'non-negative match length')
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=mass_log2(self.source/self.cold) if self.active else 0.0,
                      clock_before=self.clock)
        delta = candidate-cold
        slow_used = -1
        increment = 0.0
        clipped = 0
        if self.active:
            slow_used = self.hazard_is_slow()
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            relative = Decimal.from_float(math.exp2(delta))
            joint = self.source*relative
            total = self.cold+joint
            need(total > 0 and joint > 0, 'positive mixed mass')
            increment = 0.0 if candidate == cold else mass_log2(total)
            self.source = (1-hazard)*(joint/total)
            cap = Decimal(32)/Decimal(33)
            if self.mode == 'ceiling' and self.source > cap:
                self.source = cap
                clipped = 1
                self.clip_count += 1
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive updated masses')
            near(self.source+self.cold, 1, 'normalized authority', 1e-12)
            if self.mode == 'adaptive':
                self.clock = (255.0/256.0)*self.clock+delta
            elif self.mode in ('witness', 'ceiling', 'witnessed_ceiling') and matched >= 1:
                self.clock = (31.0/32.0)*self.clock+delta
            if self.mode == 'witnessed_ceiling':
                limit = 5 if self.clock <= -1 else 10
                cap = Decimal(2**limit)/Decimal(2**limit+1)
                if self.source > cap:
                    self.source = cap
                    self.cold = 1-self.source
                    clipped = 1
                    self.clip_count += 1
                    if limit == 5:
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
        bound = (math.log2(33) if self.mode == 'ceiling' else
                 math.log2(1025) if self.mode == 'witnessed_ceiling' else
                 10 if self.mode == 'fast' else 16)
        need(self.minimum >= -1-TOL and self.drawdown <= bound+TOL,
             self.mode+' prefix/interval bound')
        if t+1 in (1024, 4096, MOVE, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=cold if candidate == cold else cold+increment,
                    admitted_after=int(admitted), slow_used=slow_used,
                    clock_after=self.clock,
                    odds_after=after_odds, clipped=clipped)

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


def sample_row(world, regime, row, raw, delta):
    t = int(row['t'])
    record = dict(world=world, regime=regime, t=t, truth=int(raw['truth']),
                  rank=int(raw['rank']), matchedL=int(row['matched']),
                  cold=float(row['cold']), candidate=float(row['candidate']),
                  hazard='2^-16' if int(row['ceiling_slow_used']) else '2^-10',
                  witness_w_before=float(row['witness_w_before']),
                  witness_w_after=float(row['witness_w_after']),
                  adaptive_e_before=float(row['adaptive_e_before']),
                  ceiling_clipped=int(row['ceiling_clipped']),
                  witnessed_ceiling_clipped=int(row['witnessed_ceiling_clipped']),
                  witnessed_ceiling_cap_after=float(row['witnessed_ceiling_cap_after']),
                  ceiling_w_before=float(row['ceiling_w_before']),
                  ceiling_w_after=float(row['ceiling_w_after']), delta=delta)
    for mode in MODES:
        for field in ('live','odds_before','odds_after'):
            record[mode+'_'+field] = float(row[mode+'_'+field])
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
                 new_exact=True, inactive_exact=True, witness_clock_identical=True,
                 witnessed_clock_identical=True, first_admission_shared=True)
    column_new_exact = True
    history = []
    saving = cost = witness_saving = witness_cost = None
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
            expected = {m: states[m].step(t, cold, candidate, length) for m in MODES}
            common = expected['slow']
            need(int(row['admitted_after']) == int(raw['episode_activated_after']) and
                 int(row['active_before']) == int(raw['episode_active_before']),
                 'inherited first admission')
            fields = {k: common[k] for k in
                      ('shadow_before', 'active_before', 'admitted_after')}
            for mode in MODES:
                need(expected[mode]['shadow_before'] == common['shadow_before'] and
                     expected[mode]['active_before'] == common['active_before'] and
                     expected[mode]['admitted_after'] == common['admitted_after'],
                     'paired first admission')
                fields[mode+'_live'] = expected[mode]['live']
                fields[mode+'_odds_before'] = expected[mode]['odds_before']
                fields[mode+'_odds_after'] = expected[mode]['odds_after']
                fields[mode+'_slow_used'] = expected[mode]['slow_used']
                stored = float(row[mode+'_live'])
                # Condition 8 is a gate condition, so a violation is recorded and
                # preserved here, never raised: hard equality, no tolerance.
                if candidate == cold and stored != cold:
                    exact['equal_price_exact'] = False
                if rank == 0 and stored != cold:
                    exact['new_exact'] = False
                if not common['active_before'] and stored != cold:
                    exact['inactive_exact'] = False
            fields['adaptive_e_before'] = expected['adaptive']['clock_before']
            fields['adaptive_e_after'] = expected['adaptive']['clock_after']
            fields['witness_w_before'] = expected['witness']['clock_before']
            fields['witness_w_after'] = expected['witness']['clock_after']
            fields['ceiling_w_before'] = expected['ceiling']['clock_before']
            fields['ceiling_w_after'] = expected['ceiling']['clock_after']
            fields['ceiling_clipped'] = expected['ceiling']['clipped']
            fields['witnessed_ceiling_w_before'] = expected['witnessed_ceiling']['clock_before']
            fields['witnessed_ceiling_w_after'] = expected['witnessed_ceiling']['clock_after']
            fields['witnessed_ceiling_clipped'] = expected['witnessed_ceiling']['clipped']
            limit = 5 if expected['witnessed_ceiling']['clock_after'] <= -1 else 10
            prior_limit = 5 if expected['witnessed_ceiling']['clock_before'] <= -1 else 10
            fields['witnessed_ceiling_cap_after'] = float(limit)
            state = states['witnessed_ceiling']
            state.cap_bound_exact = state.cap_bound_exact and (
                float(row['witnessed_ceiling_cap_after']) == limit and
                float(row['witnessed_ceiling_odds_before']) <= prior_limit and
                float(row['witnessed_ceiling_odds_after']) <= limit)
            parity = all(expected['ceiling'][field] == expected['witness'][field]
                         for field in ('clock_before','clock_after','active_before','admitted_after'))
            parity = parity and float(row['ceiling_w_before']) == float(row['witness_w_before']) and float(row['ceiling_w_after']) == float(row['witness_w_after'])
            exact['witness_clock_identical'] = exact['witness_clock_identical'] and parity
            parity = all(expected['witnessed_ceiling'][field] == expected['witness'][field]
                         for field in ('clock_before','clock_after','active_before','admitted_after','slow_used'))
            parity = parity and float(row['witnessed_ceiling_w_before']) == float(row['witness_w_before']) and float(row['witnessed_ceiling_w_after']) == float(row['witness_w_after'])
            exact['witnessed_clock_identical'] = exact['witnessed_clock_identical'] and parity
            for key, wanted in fields.items():
                if isinstance(wanted, int):
                    need(int(row[key]) == wanted, 'authority flag '+key)
                else:
                    max_error = max(max_error, near(row[key], wanted, 'authority '+key))
            for mode in CLOCK_MODES:
                value = expected[mode]['clock_before']
                clock = clocks[mode]
                if clock['minimum'] is None or value < clock['minimum']:
                    clock['minimum'], clock['minimum_t'] = value, t
                if clock['maximum'] is None or value > clock['maximum']:
                    clock['maximum'], clock['maximum_t'] = value, t
                if t == MOVE:
                    clock['at_move'] = value
                if t == N-1:
                    clock['final'] = expected[mode]['clock_after']
            if regime in TAIL_REGIMES and t >= MOVE and common['active_before']:
                delta = float(row['witnessed_ceiling_live'])-float(row['ceiling_live'])
                if saving is None or delta > saving['delta']:
                    saving = sample_row(world, regime, row, raw, delta)
                if cost is None or delta < cost['delta']:
                    cost = sample_row(world, regime, row, raw, delta)
                delta = float(row['witnessed_ceiling_live'])-float(row['witness_live'])
                if witness_saving is None or delta > witness_saving['delta']:
                    witness_saving = sample_row(world, regime, row, raw, delta)
                if witness_cost is None or delta < witness_cost['delta']:
                    witness_cost = sample_row(world, regime, row, raw, delta)
            history.append(rank)
            if len(history) > HORIZON:
                del history[0]
        need(next(candidates, None) is None and next(authority, None) is None,
             'no extra observations')
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
                raw_cap_help=saving, raw_cap_harm=cost,
                raw_witness_help=witness_saving, raw_witness_harm=witness_cost,
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


def independent_gate(lives):
    lookup = {(life['world'], life['regime']): life for life in lives}
    need(len(lookup) == len(lives) == 32, 'complete paired worlds')
    mean = lambda values: math.fsum(values)/len(values)

    def read(regime, mode, field):
        return [lookup[w, regime]['modes'][mode][field] for w in WORLDS]

    def gap(regime, other):
        return [x-y for x, y in zip(read(regime, 'witnessed_ceiling', 'tail'),
                                    read(regime, other, 'tail'))]

    moved_fast, moved_slow = gap('moved_mid', 'fast'), gap('moved_mid', 'slow')
    moved_witness = gap('moved_mid', 'witness')
    law_fast, law_slow = gap('switched', 'fast'), gap('switched', 'slow')
    law_witness = gap('switched', 'witness')
    law_ceiling = gap('switched', 'ceiling')
    early_ref = mean(read('recombined', 'slow', 'early'))
    full_ref = mean(read('recombined', 'slow', 'gain'))
    early_ratio = (mean(read('recombined', 'witnessed_ceiling', 'early'))/early_ref
                   if early_ref > 0 else None)
    full_ratio = (mean(read('recombined', 'witnessed_ceiling', 'gain'))/full_ref
                  if full_ref > 0 else None)
    positive = read('recombined', 'witnessed_ceiling', 'gain')
    quantities = dict(
        moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
        moved_vs_witness=moved_witness, law_vs_fast=law_fast,
        law_vs_slow=law_slow, law_vs_witness=law_witness,
        law_vs_ceiling=law_ceiling,
        early_retention=early_ratio, full_retention=full_ratio,
        recombined_full=positive,
        switched_first_fast_after_move=read('switched', 'witnessed_ceiling',
                                            'first_fast_after_move'),
        moved_fast_share=read('moved_mid', 'witnessed_ceiling', 'tail_fast_share'),
        related_low_clips=sum(life['modes']['witnessed_ceiling']['low_clip_count']
                              for life in lives if life['regime'] != 'unrelated'),
        related_high_clips=sum(life['modes']['witnessed_ceiling']['high_clip_count']
                               for life in lives if life['regime'] != 'unrelated'))
    tables = {regime: {mode: {field: mean(read(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
    tests = dict(
        surface_fast_mean=mean(moved_fast) > 1,
        surface_fast_wins=sum(x > 0 for x in moved_fast) >= 5,
        surface_slow_retention=mean(moved_slow) >= -3,
        surface_witness_recovery=mean(moved_witness) >= -3,
        law_fast_retention=mean(law_fast) >= -1,
        law_slow_mean=mean(law_slow) > 1,
        law_slow_wins=sum(x > 0 for x in law_slow) >= 5,
        law_witness_mean=mean(law_witness) > 1,
        law_fixed_ceiling_protection=mean(law_ceiling) >= -2,
        recombined_early_retention=early_ratio is not None and early_ratio >= .95,
        recombined_full_retention=full_ratio is not None and full_ratio >= .95,
        recombined_full_positive=all(x > 0 for x in positive),
        mechanism_clock_admission=all(life['exactness']['first_admission_shared'] and
            life['exactness']['witnessed_clock_identical'] for life in lives),
        mechanism_caps=all(life['modes']['witnessed_ceiling']['cap_bound_exact'] and
            life['modes']['witnessed_ceiling']['max_odds_before'] <= 10 and
            life['modes']['witnessed_ceiling']['max_odds_after'] <= 10 for life in lives),
        mechanism_both_levels_clip=quantities['related_low_clips'] > 0 and
            quantities['related_high_clips'] > 0,
        structural_unrelated_never_admits=all(lookup[w, 'unrelated']['modes'][m]['activation']
                                      is None for w in WORLDS for m in MODES),
        structural_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        structural_distributions_normalized=all(0 <= life['max_norm_error'] <= 1e-8 and
                                        life['column_new_exact'] for life in lives),
        structural_exact_quotes=all(life['exactness']['equal_price_exact'] and
            life['exactness']['inactive_exact'] and life['exactness']['new_exact']
            for life in lives),
        structural_prefix_drawdown=all(
            life['modes']['witnessed_ceiling']['minimum'] >= -1-TOL and
            life['modes']['witnessed_ceiling']['max_drawdown'] <= math.log2(1025)+TOL
            for life in lives))
    need(len(tests) == 20, 'twenty preregistered conditions')
    return quantities, tables, tests


def identity():
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA, 'frozen protocol identity')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS) and
         frozen['regimes'] == list(REGIMES) and frozen['modes'] == list(MODES),
         'freeze scope')
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
    pins = identity()
    claim = json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds'] == list(WORLDS) and claim['regimes'] == list(REGIMES) and
         claim['modes'] == list(MODES) and claim['namespace'] == NAMESPACE and
         claim['protocol_sha256'] == PROTOCOL_SHA, 'claimed scope')
    # Post-freeze reader-only repair: turn19's fast_share_bound receipt was
    # removed in turn20; WINDOW and the frozen two-level cap remain checked.
    need(claim['window'] == WINDOW and
         claim['witnessed_ceiling_caps'] == WITNESSED_CEILING_CAPS,
         'claimed mechanism thresholds')
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
    need(candidate_count == 8*4*N*7, 'all inherited candidate forecasts')
    need(authority_count == 8*4*N*6, 'all authority forecasts')
    compare(claim['lives'], lives, 'all life statistics')
    compare(claim['manipulation'], surface, 'surface manipulation')
    quantities, tables, tests = independent_gate(lives)
    compare(claim['quantities'], quantities, 'gate quantities')
    compare(claim['tables'], tables, 'regime tables')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'material gate reconstruction')
    reader_tests = dict(
        c9_all_candidate_arms_rebuilt=candidate_count == 8*4*N*7,
        c9_all_six_modes_replayed=authority_count == 8*4*N*6,
        c9_agreement_within_tolerance=max_error <= TOL,
        c9_retained_digests_intact=True)
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  complete_gate_pass=all(tests.values()) and all(reader_tests.values()),
                  witnessed_ceiling_caps=WITNESSED_CEILING_CAPS,
                  candidate_forecasts=candidate_count,
                  authority_forecasts=authority_count,
                  maximum_numeric_error=max_error, maximum_normalization_error=max_norm,
                  worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
                  identities=pins, quantities=quantities, tables=tables,
                  tests=tests, reader_tests=reader_tests,
                  scope='Independent frozen turn13 source books and seven candidates; '
                        'own suffix recount of the episode matchedL sequence; '
                        'independent Decimal mass replay of slow/fast/adaptive/witness, '
                        'fixed ceiling and witnessed ceiling authority, every quote, '
                        'hazard choice, clip, odds and clock value. '
                        'HEAD256 bindings and P0 are supplied trace inputs; full-vector '
                        'receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: result[k] for k in
                      ('verification_pass', 'gate_pass', 'complete_gate_pass',
                       'candidate_forecasts', 'authority_forecasts',
                       'maximum_numeric_error', 'maximum_normalization_error',
                       'quantities', 'tests', 'reader_tests')},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
