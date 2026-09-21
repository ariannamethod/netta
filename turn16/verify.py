#!/usr/bin/env python3
"""Independent source/candidate and Decimal earned-return authority reader.

Loads only the frozen turn13 independent reader. Never imports a writer or
new C authority code. Full HEAD256 bindings/P0 remain supplied trace inputs.
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
WORLDS = tuple(range(176, 184))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
MODES = ('slow', 'fast', 'budget', 'return')
N = 16384
MOVE = 8192
NAMESPACE = 'netta-earned-return-v1'
TOL = 1e-7
PROTOCOL_SHA = 'b14c5422427feaa7d2f11675667b53d354dd102c177e62b8f946b4a0e3929d44'

spec = importlib.util.spec_from_file_location('turn16_independent_turn13', REPO/'turn13'/'verify.py')
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
    need(value > 0, 'positive logarithm mass')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Authority:
    """Normalized Decimal masses; support alone follows binary64 arithmetic."""
    def __init__(self, mode):
        need(mode in MODES, 'known authority mode')
        self.mode = mode
        self.hazard = Decimal(1)/Decimal(65536 if mode == 'slow' else 1024)
        self.initial_cold = Decimal('0.5') if mode in ('slow', 'fast') else Decimal('0.5').sqrt()
        self.z0 = mass_log2((1-self.initial_cold)/self.initial_cold)
        self.source, self.cold = 1-self.initial_cold, self.initial_cold
        self.active = self.armed = self.used = False
        self.support = self.shadow = self.gain = 0.0
        self.minimum = self.peak = self.drawdown = 0.0
        self.activation = None
        self.horizons = {}
        self.returns = []

    def reset_mass(self):
        self.cold = self.initial_cold
        self.source = 1-self.cold

    def step(self, t, cold, candidate):
        need(math.isfinite(cold) and math.isfinite(candidate), 'finite prices')
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=mass_log2(self.source/self.cold) if self.active else 0.0,
                      armed_before=int(self.armed), support_before=self.support,
                      used_before=int(self.used))
        delta = candidate-cold
        increment = 0.0
        event = admitted = False
        if self.active:
            relative = Decimal.from_float(math.exp2(delta))
            joint = self.source*relative
            total = self.cold+joint
            need(total > 0 and joint > 0, 'positive quote mass')
            # Neutral evidence must quote the exact original P0 float.
            increment = 0.0 if candidate == cold else mass_log2(total)
            self.source = (1-self.hazard)*(joint/total)
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive updated masses')
            near(self.source+self.cold, 1, 'normalized authority', 1e-12)
            updated_odds = mass_log2(self.source/self.cold)
            if self.mode == 'return' and not self.used:
                if self.armed:
                    self.support = max(0.0, self.support+delta)
                    if updated_odds >= self.z0:
                        self.armed = False
                        self.support = 0.0
                    elif self.support >= 32.0:
                        self.returns.append(dict(t=t, next_byte=t+1,
                            old_odds=updated_odds, support=self.support,
                            cold=cold, candidate=candidate,
                            odds_before=before['odds_before']))
                        self.reset_mass()
                        self.used = True
                        self.armed = False
                        self.support = 0.0
                        event = True
                if not self.armed and not self.used and updated_odds <= -32.0:
                    self.armed = True
                    self.support = 0.0
        self.gain += increment
        self.shadow += delta
        if not before['active_before'] and self.shadow >= 32.0:
            self.active = True
            self.activation = t+1
            self.reset_mass()
            admitted = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        bound = 16 if self.mode == 'slow' else 10.5 if self.mode == 'return' else 10
        need(self.minimum >= -1-TOL and self.drawdown <= bound+TOL,
             self.mode+' prefix/interval bounds')
        need(len(self.returns) <= 1 and self.used == bool(self.returns), 'single spent return')
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=cold if candidate == cold else cold+increment,
                    admitted_after=int(admitted), event=int(event),
                    odds_after=mass_log2(self.source/self.cold) if self.active else 0.0,
                    support_after=self.support, armed_after=int(self.armed),
                    used_after=int(self.used), gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, candidate_gain=self.shadow,
                    tail=self.gain-self.horizons['8192'], horizons=self.horizons,
                    minimum=self.minimum, peak=self.peak, max_drawdown=self.drawdown,
                    activation=self.activation, returns=[event['next_byte'] for event in self.returns])


def manipulation():
    records = []
    for world in WORLDS:
        folder = HERE/'data'/f'world{world}'
        raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
        need(all(len(value) == N for value in raw.values()), 'target manipulation horizons')
        seed_bytes = hashlib.sha256(f'{NAMESPACE}|{world}|surface'.encode()).digest()[:8]
        order = list(range(256))
        random.Random(int.from_bytes(seed_bytes, 'big')).shuffle(order)
        need(sorted(order) == list(range(256)), 'surface bijection')
        need(raw['switched'][:MOVE] == raw['recombined'][:MOVE], 'switched common prefix')
        need(raw['moved_mid'] == raw['recombined'][:MOVE]+bytes(order[b] for b in raw['recombined'][MOVE:]),
             'mid surface map from independent seed')
        hidden = json.loads((folder/'SURFACE.json').read_text())
        need(hidden['world'] == world and hidden['seed'] == int.from_bytes(seed_bytes, 'big') and
             hidden['move'] == MOVE and hidden['surface_map'] == order,
             'declared independently rebuilt surface transformation')
        records.append(dict(world=world, prefix_identical=True, bijection=True,
                            differing_tail=sum(a != b for a, b in zip(
                                raw['moved_mid'][MOVE:], raw['recombined'][MOVE:]))))
    return records


def verify_authority(world, regime, inherited):
    folder = f'world{world}'
    source = HERE/'results'/folder/(regime+'.tsv.gz')
    replay = HERE/'results'/'authority'/folder/(regime+'.tsv.gz')
    states = {(mode, arm): Authority(mode) for mode in MODES for arm in ARMS}
    max_error = 0.0
    count = 0
    events = []
    prior_cost = math.log2(.5/(1-math.sqrt(.5)))
    with gzip.open(source, 'rt') as f, gzip.open(replay, 'rt') as g:
        candidates = csv.DictReader(f, delimiter='\t')
        authority = csv.DictReader(g, delimiter='\t')
        for t in range(N):
            raw = next(candidates, None)
            need(raw is not None and int(raw['t']) == t, 'complete candidate chronology')
            cold = float(raw['logcold'])
            for arm in ARMS:
                candidate = float(raw[arm+'_candidate'])
                row = next(authority, None)
                need(row is not None and int(row['t']) == t and row['arm'] == arm,
                     'complete authority chronology and arm order')
                need(float(row['cold']) == cold and float(row['candidate']) == candidate,
                     'identical observed C/S across candidate and authority files')
                expected = {m: states[m, arm].step(t, cold, candidate) for m in MODES}
                common = expected['slow']
                fields = {k: common[k] for k in ('shadow_before','active_before','admitted_after')}
                for mode in MODES:
                    fields[mode+'_live'] = expected[mode]['live']
                    fields[mode+'_odds_before'] = expected[mode]['odds_before']
                    need(expected[mode]['shadow_before'] == common['shadow_before'] and
                         expected[mode]['active_before'] == common['active_before'] and
                         expected[mode]['admitted_after'] == common['admitted_after'],
                         'shared initial admission and lifetime shadow')
                    if candidate == cold:
                        need(float(row[mode+'_live']) == cold, 'exact unchanged quote '+mode)
                    if not common['active_before']:
                        need(float(row[mode+'_live']) == cold, 'inactive/crossing quote cold '+mode)
                for key in ('armed_before','support_before','used_before','event','odds_after',
                            'support_after','armed_after','used_after'):
                    fields['return_'+key] = expected['return'][key]
                for name, wanted in fields.items():
                    if isinstance(wanted, int):
                        need(int(row[name]) == wanted, 'authority flag '+name)
                    else:
                        max_error = max(max_error, near(row[name], wanted, 'authority '+name))
                if expected['return']['event']:
                    ret = expected['return']
                    events.append(dict(arm=arm, t=t, next_byte=t+1, truth=int(raw['truth']),
                        rank=int(raw['rank']), cold=cold, candidate=candidate,
                        matched_length=None if arm == 'row' else int(raw[arm+'_matchedL']),
                        old_odds=ret['odds_before'], support_before=ret['support_before'],
                        support_at_return=max(0.0, ret['support_before']+candidate-cold),
                        live=ret['live'], fast_live=expected['fast']['live'],
                        budget_live=expected['budget']['live'], slow_live=expected['slow']['live'],
                        odds_after=ret['odds_after']))
                need(states['fast', arm].gain-states['budget', arm].gain <= prior_cost+TOL,
                     'budget prior price against fast')
                if not states['return', arm].used:
                    max_error = max(max_error, near(states['return', arm].gain,
                        states['budget', arm].gain, 'budget/return identity before return'))
                count += len(MODES)
        need(next(candidates, None) is None and next(authority, None) is None,
             'no excess candidate or authority observations')
    modes = {mode: {arm: states[mode, arm].result() for arm in ARMS} for mode in MODES}
    for arm in ARMS:
        original_keys = ('gain','candidate_gain','tail','horizons','minimum','peak','max_drawdown','activation')
        v13.compare_tree(inherited['arms'][arm],
                         {k: modes['slow'][arm][k] for k in original_keys},
                         'inherited slow authority '+arm)
    return dict(world=world, regime=regime, modes=modes, return_events=events,
                source_trace_sha256=digest(source), authority_trace_sha256=digest(replay)), count, max_error


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


def identity():
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA, 'frozen protocol identity')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS) and
         frozen['regimes'] == list(REGIMES), 'freeze world/namespace/regime scope')
    for path, wanted in frozen['files'].items():
        need(digest(REPO/path) == wanted, 'frozen file '+path)
    pins = dict(protocol_sha256=PROTOCOL_SHA, reader_sha256=digest(Path(__file__)),
                freeze_sha256=digest(HERE/'FREEZE.json'), result_sha256=digest(HERE/'RESULT.json'))
    for name in ('DATA_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for path, wanted in manifest.items():
            need(digest(HERE/path) == wanted, 'retained artifact '+path)
        pins[name] = digest(HERE/name)
    return pins



def quantities_and_tests(lives):
    """Paired per-world observations; no regime is supplied to Authority."""
    lookup = {(life['world'], life['regime']): life for life in lives}
    need(len(lives) == len(WORLDS)*len(REGIMES) == len(lookup), 'unique complete target lives')
    mean = lambda values: math.fsum(values)/len(values)
    pick = lambda regime, mode, field: [lookup[w, regime]['modes'][mode]['episode'][field]
                                        for w in WORLDS]
    pair = lambda regime, other, field: [a-b for a,b in zip(
        pick(regime, 'return', field), pick(regime, other, field))]
    surface = {mode: pair('moved_mid', mode, 'tail') for mode in ('budget','fast','slow')}
    law = {mode: pair('switched', mode, 'tail') for mode in ('budget','fast','slow')}
    early = {mode: [lookup[w,'recombined']['modes'][mode]['episode']['horizons']['4096']
                     for w in WORLDS] for mode in MODES}
    full = {mode: pick('recombined', mode, 'gain') for mode in MODES}
    switched = {mode: pick('switched', mode, 'gain') for mode in MODES}
    refs = {'early':mean(early['slow']), 'full':mean(full['slow']),
            'switched':mean(switched['fast'])}
    ratios = {name: (mean(values)/refs[name] if refs[name] > 0 else None)
              for name,values in (('early',early['return']),('full',full['return']),
                                  ('switched',switched['return']))}
    surface_returns = [dict(world=w, next_byte=byte)
                       for w in WORLDS
                       for byte in lookup[w,'moved_mid']['modes']['return']['episode']['returns']
                       if byte > MOVE]
    bounds = all(stats['minimum'] >= -1-TOL and stats['max_drawdown'] <=
                 (16 if mode == 'slow' else 10.5 if mode == 'return' else 10)+TOL
                 for life in lives for mode in MODES for stats in life['modes'][mode].values())
    shared = all(len({life['modes'][mode][arm]['activation'] for mode in MODES}) == 1
                 for life in lives for arm in ARMS)
    null = all(life['modes'][mode]['permuted']['activation'] is None
               for life in lives for mode in MODES) and all(
               stats['activation'] is None for life in lives if life['regime']=='unrelated'
               for mode in MODES for stats in life['modes'][mode].values())
    tests = dict(
        surface_vs_budget_mean=mean(surface['budget']) > 1,
        surface_vs_budget_wins=sum(x > 0 for x in surface['budget']) >= 5,
        surface_vs_fast_mean=mean(surface['fast']) > 1,
        surface_vs_slow_retention=mean(surface['slow']) >= -1,
        law_vs_fast_retention=mean(law['fast']) >= -1,
        law_vs_slow_mean=mean(law['slow']) > 1,
        law_vs_slow_wins=sum(x > 0 for x in law['slow']) >= 5,
        early_retention=ratios['early'] is not None and ratios['early'] >= .9,
        full_retention=ratios['full'] is not None and ratios['full'] >= .9,
        switched_retention=ratios['switched'] is not None and ratios['switched'] >= .9,
        full_positive=all(x > 0 for x in full['return']),
        early_gain=mean(early['return'])/4096 >= .005,
        early_positive=sum(x > 0 for x in early['return']) >= 6,
        surface_return_observed=bool(surface_returns),
        archive_shared=True, first_admission_shared=shared,
        prefix_and_interval_bounds=bounds, null_admissions=null)
    need(len(tests) == 18, 'eighteen preregistered conditions')
    quantities = dict(surface_vs_budget=surface['budget'], surface_vs_fast=surface['fast'],
                      surface_vs_slow=surface['slow'], law_vs_fast=law['fast'],
                      law_vs_slow=law['slow'], early_retention=ratios['early'],
                      full_retention=ratios['full'], switched_retention=ratios['switched'],
                      full_positive=sum(x > 0 for x in full['return']),
                      early_positive=sum(x > 0 for x in early['return']),
                      early_gain_bpb=mean(early['return'])/4096,
                      surface_returns=len(surface_returns))
    return quantities, tests



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'verification output already exists')
    pins = identity()
    claim = json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds'] == list(WORLDS) and claim['regimes'] == list(REGIMES) and
         claim['modes'] == list(MODES) and claim['namespace'] == NAMESPACE and
         claim['protocol_sha256'] == PROTOCOL_SHA, 'declared exact scope')
    manip = manipulation()
    books, lives = [], []
    candidate_count = authority_count = 0
    max_error = max_norm = 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            rebuilt = v13.verify_books(world)
            books.append(dict(world=world, **rebuilt[-1]))
            for regime in REGIMES:
                inherited, count, error, norm, _ = v13.verify_life(world, regime, rebuilt)
                candidate_count += count
                max_error, max_norm = max(max_error, error), max(max_norm, norm)
                life, count, error = verify_authority(world, regime, inherited)
                authority_count += count
                max_error = max(max_error, error)
                life['max_norm_error'] = norm
                lives.append(life)
    need(candidate_count == 8*4*N*7, 'all 3670016 inherited candidate forecasts')
    need(authority_count == 8*4*N*7*4, 'all 14680064 new authority forecasts')
    compare(claim['lives'], lives, 'life forecasts/statistics/return events')
    for life in claim['lives']:
        need(0 <= life['max_norm_error'] <= 1e-8, 'claimed normalization bound')
    compare(claim['manipulation'], manip, 'independent byte manipulation')
    quantities, tests = quantities_and_tests(lives)
    compare(claim['quantities'], quantities, 'paired gate quantities')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'all eighteen gate fields and verdict')
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
        namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
        source_observations=8*4*N, target_observations=8*4*N,
        inherited_candidate_forecasts=candidate_count, authority_forecasts=authority_count,
        maximum_numeric_error=max_error, maximum_normalization_error=max_norm,
        identities=pins, source_books=books, manipulation=manip, lives=lives,
        quantities=quantities, tests=tests,
        scope='Independent frozen turn13 source BPE/joint selection, seven exact archives, '
              'stored matches and candidate reconstruction; independent Decimal mass replay '
              'of slow/fast/budget/one-return authority, every prospective quote/state/event, '
              'all material quantities, first admissions, loss bounds, null behavior and '
              'source-prefix/surface manipulation. HEAD256 bindings and sparse P0 remain '
              'supplied trace inputs; full 256-byte normalization receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
