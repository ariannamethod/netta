#!/usr/bin/env python3
"""Independent turn13 source reader plus Decimal replay of four authority clocks.

Rebuilds the source books, all seven inherited candidate arms, the surface
bijections and this turn's own matchedL sequence with its own code, then steps
slow/fast/adaptive/witness through every byte in Decimal mass arithmetic and
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
REPO = HERE.parent
WORLDS = tuple(range(192, 200))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'adaptive', 'witness')
CLOCK_MODES = ('adaptive', 'witness')
TAIL_REGIMES = ('switched', 'moved_mid')
NAMESPACE = 'netta-witness-clock-v1'
N = 16384
MOVE = 8192
WINDOW = 256
FAST_SHARE = 0.25
TOL = 1e-7
HORIZON = 32
PROTOCOL_SHA = 'f6eb3c3bb156ba0a181dc66d811c4f99518b41f0453bad740fc12052188be6b6'

spec = importlib.util.spec_from_file_location('turn18_independent_turn13_reader',
                                              REPO/'turn13/verify.py')
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
        if self.active:
            slow_used = self.hazard_is_slow()
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            relative = Decimal.from_float(math.exp2(delta))
            joint = self.source*relative
            total = self.cold+joint
            need(total > 0 and joint > 0, 'positive mixed mass')
            increment = 0.0 if candidate == cold else mass_log2(total)
            self.source = (1-hazard)*(joint/total)
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive updated masses')
            near(self.source+self.cold, 1, 'normalized authority', 1e-12)
            if self.mode == 'adaptive':
                self.clock = (255.0/256.0)*self.clock+delta
            elif self.mode == 'witness' and matched >= 1:
                self.clock = (31.0/32.0)*self.clock+delta
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
        bound = 10 if self.mode == 'fast' else 16
        need(self.minimum >= -1-TOL and self.drawdown <= bound+TOL,
             self.mode+' prefix/interval bound')
        if t+1 in (1024, 4096, MOVE, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=cold if candidate == cold else cold+increment,
                    admitted_after=int(admitted), slow_used=slow_used,
                    clock_after=self.clock,
                    odds_after=mass_log2(self.source/self.cold) if self.active else 0.0)

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
                    first_fast_after_move=self.first_fast_after_move)


def empty_clock():
    return dict(minimum=None, minimum_t=None, maximum=None, maximum_t=None,
                at_move=None, final=None)


def sample_row(world, regime, row, raw, delta):
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
                 new_exact=True, inactive_exact=True)
    column_new_exact = True
    history = []
    saving = cost = None
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
                if expected['witness']['slow_used'] == 0:
                    delta = float(row['witness_live'])-float(row['slow_live'])
                    if saving is None or delta > saving['delta']:
                        saving = sample_row(world, regime, row, raw, delta)
                else:
                    delta = float(row['witness_live'])-float(row['fast_live'])
                    if cost is None or delta < cost['delta']:
                        cost = sample_row(world, regime, row, raw, delta)
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
    life = dict(world=world, regime=regime,
                modes={m: states[m].result() for m in MODES},
                clocks=clocks, exactness=exact, column_new_exact=column_new_exact,
                raw_guilt_saving=saving, raw_hedge_cost=cost,
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
        return [x-y for x, y in zip(read(regime, 'witness', 'tail'),
                                    read(regime, other, 'tail'))]

    moved_fast, moved_slow = gap('moved_mid', 'fast'), gap('moved_mid', 'slow')
    law_fast, law_slow = gap('switched', 'fast'), gap('switched', 'slow')
    law_adaptive = gap('switched', 'adaptive')
    early_ref = mean(read('recombined', 'slow', 'early'))
    full_ref = mean(read('recombined', 'slow', 'gain'))
    early_ratio = (mean(read('recombined', 'witness', 'early'))/early_ref
                   if early_ref > 0 else None)
    full_ratio = (mean(read('recombined', 'witness', 'gain'))/full_ref
                  if full_ref > 0 else None)
    positive = read('recombined', 'witness', 'gain')
    first_fast = read('switched', 'witness', 'first_fast_after_move')
    offsets = [None if x is None else x-MOVE for x in first_fast]
    prompt = [x is not None and x < WINDOW for x in offsets]
    shares = read('moved_mid', 'witness', 'tail_fast_share')
    quiet = [x <= FAST_SHARE for x in shares]
    quantities = dict(
        moved_vs_fast=moved_fast, moved_vs_slow=moved_slow,
        law_vs_fast=law_fast, law_vs_slow=law_slow, law_vs_adaptive=law_adaptive,
        early_retention=early_ratio, full_retention=full_ratio,
        recombined_full=positive,
        switched_first_fast_after_move=first_fast,
        switched_first_fast_offsets=offsets,
        switched_prompt_worlds=sum(prompt),
        moved_fast_share=shares, moved_quiet_worlds=sum(quiet))
    tables = {regime: {mode: {field: mean(read(regime, mode, field))
                              for field in ('gain', 'tail', 'early')}
                       for mode in MODES} for regime in REGIMES}
    tests = dict(
        c1_moved_vs_fast_mean=mean(moved_fast) > 1,
        c1_moved_vs_fast_wins=sum(x > 0 for x in moved_fast) >= 5,
        c1_moved_vs_slow_retention=mean(moved_slow) >= -3,
        c2_law_vs_fast_retention=mean(law_fast) >= -1,
        c2_law_vs_slow_mean=mean(law_slow) > 1,
        c2_law_vs_slow_wins=sum(x > 0 for x in law_slow) >= 5,
        c3_law_vs_adaptive_mean=mean(law_adaptive) > 1,
        c4_recombined_early_retention=early_ratio is not None and early_ratio >= .95,
        c4_recombined_full_retention=full_ratio is not None and full_ratio >= .95,
        c4_recombined_full_positive=all(x > 0 for x in positive),
        c5_switched_first_fast_within_256=sum(prompt) >= 6,
        c6_moved_fast_share_at_most_quarter=sum(quiet) >= 6,
        c7_first_admission_shared=all(len({life['modes'][m]['activation']
                                           for m in MODES}) == 1 for life in lives),
        c7_unrelated_never_admits=all(lookup[w, 'unrelated']['modes'][m]['activation']
                                      is None for w in WORLDS for m in MODES),
        c7_archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                           <= 528 for w in WORLDS),
        c7_prefix_bound=all(s['minimum'] >= -1-TOL for life in lives
                            for s in life['modes'].values()),
        c7_drawdown_bound=all(s['max_drawdown'] <= 16+TOL for life in lives
                              for s in life['modes'].values()),
        c7_distributions_normalized=all(0 <= life['max_norm_error'] <= 1e-8 and
                                        life['column_new_exact'] for life in lives),
        c8_equal_price_exact=all(life['exactness']['equal_price_exact'] and
                                 life['exactness']['inactive_exact'] for life in lives),
        c8_protected_new_exact=all(life['exactness']['new_exact'] for life in lives))
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
    need(claim['window'] == WINDOW and claim['fast_share_bound'] == FAST_SHARE,
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
    need(authority_count == 8*4*N*4, 'all authority forecasts')
    compare(claim['lives'], lives, 'all life statistics')
    compare(claim['manipulation'], surface, 'surface manipulation')
    quantities, tables, tests = independent_gate(lives)
    compare(claim['quantities'], quantities, 'gate quantities')
    compare(claim['tables'], tables, 'regime tables')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'material gate reconstruction')
    reader_tests = dict(
        c9_all_candidate_arms_rebuilt=candidate_count == 8*4*N*7,
        c9_all_four_modes_replayed=authority_count == 8*4*N*4,
        c9_agreement_within_tolerance=max_error <= TOL,
        c9_retained_digests_intact=True)
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  nine_family_gate_pass=all(tests.values()) and all(reader_tests.values()),
                  candidate_forecasts=candidate_count,
                  authority_forecasts=authority_count,
                  maximum_numeric_error=max_error, maximum_normalization_error=max_norm,
                  worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
                  identities=pins, quantities=quantities, tables=tables,
                  tests=tests, reader_tests=reader_tests,
                  scope='Independent frozen turn13 source books and seven candidates; '
                        'own suffix recount of the episode matchedL sequence; '
                        'independent Decimal mass replay of slow/fast/adaptive/witness '
                        'authority, every quote, hazard choice, odds and clock value. '
                        'HEAD256 bindings and P0 are supplied trace inputs; full-vector '
                        'receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: result[k] for k in
                      ('verification_pass', 'gate_pass', 'nine_family_gate_pass',
                       'candidate_forecasts', 'authority_forecasts',
                       'maximum_numeric_error', 'maximum_normalization_error',
                       'quantities', 'tests', 'reader_tests')},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
