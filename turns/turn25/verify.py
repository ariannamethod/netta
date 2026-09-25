#!/usr/bin/env python3
"""Independent turn25 probability reader; no writer or predictor imports.

The input P0/candidate/match tape is pinned evidence. Four authority laws,
all prices/states, latch chronology, life metrics and eight bars are rebuilt.
HEAD256/source books are not independently rebuilt by this reader.
"""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(248, 256))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'witness', 'latch')
NAMESPACE = 'netta-witness-latch-v1'
N, MOVE, EARLY = 16384, 8192, 4096
TOL = 1e-7
PROTOCOL_SHA = 'db4d93eb8835973330cbe6e4a356b9228d230e9251b7fbc7bb7f0bb074cdf730'
INTERFACE_SHA = '3013801e74b4537ad177fcfdc817d9923101b352a0ae1aa44fd08e308a405ceb'
REQUIRED_RESULT_KEYS = frozenset(('namespace worlds regimes modes protocol_sha256 '
    'lives bars quantities tables validity material_pass independent_reader_pending').split())
COLUMNS = ('t cold candidate matched shadow_before active_before admitted_after '
           'w_before w_after latch_before latch_after').split() + [
    m+'_'+f for m in MODES for f in ('live', 'odds_before', 'slow_used', 'odds_after')]


class Refusal(Exception):
    pass


def need(condition, message):
    if not condition:
        raise Refusal(message)


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+': nonfinite value')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}; error={error}')
    return error


def digest(path):
    path = Path(path)
    need(path.is_file(), f'retained artifact {path} is missing')
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def check_artifact(path, wanted):
    actual = digest(path)
    need(actual == wanted, f'retained artifact {path}: sha256 {actual} != {wanted}')
    return actual


def check_schema(result):
    need(isinstance(result, dict), 'RESULT.json: expected object')
    for key in sorted(REQUIRED_RESULT_KEYS):
        need(key in result, f'RESULT.json: missing required field {key}')
    for key in ('worlds', 'regimes', 'modes', 'lives'):
        need(isinstance(result[key], list), f'RESULT.json: {key} must be a list')
    for key in ('bars', 'quantities', 'tables', 'validity'):
        need(isinstance(result[key], dict), f'RESULT.json: {key} must be an object')
    for key in ('material_pass', 'independent_reader_pending'):
        need(isinstance(result[key], bool), f'RESULT.json: {key} must be boolean')
    for key in ('namespace', 'protocol_sha256'):
        need(isinstance(result[key], str), f'RESULT.json: {key} must be a string')
    return sorted(REQUIRED_RESULT_KEYS)


def mass_log2(value):
    need(value > 0, 'authority: nonpositive probability mass')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Authority:
    """Four independent mass trajectories, without time/regime input to policy."""

    def __init__(self, mode):
        need(mode in MODES, f'authority: unknown mode {mode}')
        self.mode = mode
        self.source = self.cold = Decimal('0.5')
        self.shadow = self.w = 0.0
        self.active = False
        self.latched = 0
        self.gain = self.tail = self.minimum = self.peak = self.drawdown = 0.0
        self.activation = None
        self.horizons = {}
        self.slow_count = self.fast_count = self.tail_fast_count = 0

    def odds(self):
        return mass_log2(self.source/self.cold) if self.active else 0.0

    def step(self, t, cold, candidate, matched):
        need(math.isfinite(cold) and math.isfinite(candidate), f'{self.mode}: nonfinite price')
        need(isinstance(matched, int) and matched >= 0, f'{self.mode}: invalid matched length')
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=self.odds(), w_before=self.w, latch_before=self.latched)
        delta = candidate-cold
        live, slow_used = cold, -1
        if self.active:
            joint = self.source*Decimal.from_float(math.exp2(delta))
            total = self.cold+joint
            need(joint > 0 and total > 0, f'{self.mode}: nonpositive quote mass')
            if candidate != cold:
                live = cold+mass_log2(total)
            if self.mode == 'slow':
                slow_used = 1
            elif self.mode == 'fast':
                slow_used = 0
            elif self.mode == 'witness':
                slow_used = int(self.w > -1.0)
            else:
                slow_used = 1-self.latched
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            self.source = (1-hazard)*joint/total
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, f'{self.mode}: nonpositive updated mass')
            near(self.source+self.cold, 1, f'{self.mode}: mass normalization', 1e-12)
            if self.mode in ('witness', 'latch') and matched >= 1:
                self.w = (31.0/32.0)*self.w+delta
            if self.mode == 'latch':
                if self.w <= -1.0:
                    self.latched = 1
                elif self.w >= 1.0:
                    self.latched = 0
            if slow_used:
                self.slow_count += 1
            else:
                self.fast_count += 1
                if t >= MOVE:
                    self.tail_fast_count += 1
        # Bookkeeping observes a price that was fixed before the transition.
        change = live-cold
        self.gain += change
        if t >= MOVE:
            self.tail += change
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        self.shadow += delta
        admitted = False
        if not self.active and self.shadow >= 32:
            self.active = True
            self.source = self.cold = Decimal('0.5')
            self.w = 0.0
            self.latched = 0
            self.activation = t+1
            admitted = True
        if t+1 in (1024, EARLY, MOVE, N):
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=live, slow_used=slow_used, odds_after=self.odds(),
                    w_after=self.w, latch_after=self.latched,
                    admitted_after=int(admitted))

    def result(self):
        return dict(gain=self.gain, tail=self.tail, early=self.horizons[str(EARLY)],
                    minimum=self.minimum, peak=self.peak, drawdown=self.drawdown,
                    activation=self.activation, horizons=self.horizons,
                    slow_count=self.slow_count, fast_count=self.fast_count,
                    tail_fast_count=self.tail_fast_count)


def check_life(world, regime):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    replay = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    states = {m: Authority(m) for m in MODES}
    diagnostics = dict(first_set=None, first_return=None, set_count=0, return_count=0,
                       sets_before_seam=0, returns_before_seam=0,
                       first_set_after_seam=None, latch_at_seam=None)
    exact = dict(admission=True, equal_price=True, inactive=True, new=True,
                 clock=True, latch_law=True)
    error = maxnorm = 0.0
    previous = None
    with gzip.open(source, 'rt') as sf, gzip.open(replay, 'rt') as af:
        inputs = csv.DictReader(sf, delimiter='\t')
        outputs = csv.DictReader(af, delimiter='\t')
        need(outputs.fieldnames == COLUMNS, f'{replay}: authority schema differs')
        for t in range(N):
            raw, row = next(inputs, None), next(outputs, None)
            need(raw is not None and row is not None, f'{source} / {replay}: missing row {t}')
            at = f'{replay}: t={t}'
            need(int(raw['t']) == int(row['t']) == t, at+': chronology')
            cold, candidate = float(raw['logcold']), float(raw['episode_candidate'])
            matched, rank = int(raw['episode_matchedL']), int(raw['rank'])
            need(float(row['cold']) == cold and float(row['candidate']) == candidate and
                 int(row['matched']) == matched, at+': input tape differs')
            normalization = float(raw['max_norm_error'])
            need(math.isfinite(normalization) and normalization >= 0, f'{source}: normalization at {t}')
            maxnorm = max(maxnorm, normalization)
            expected = {m: states[m].step(t, cold, candidate, matched) for m in MODES}
            common = expected['slow']
            fields = {k: common[k] for k in ('shadow_before', 'active_before', 'admitted_after')}
            fields.update(w_before=expected['latch']['w_before'], w_after=expected['latch']['w_after'],
                          latch_before=expected['latch']['latch_before'],
                          latch_after=expected['latch']['latch_after'])
            for m in MODES:
                for f in ('live', 'odds_before', 'slow_used', 'odds_after'):
                    fields[m+'_'+f] = expected[m][f]
                for f in ('shadow_before', 'active_before', 'admitted_after'):
                    need(expected[m][f] == common[f], at+': independent admission disagreement '+m)
                actual = float(row[m+'_live'])
                if not common['active_before'] and actual != cold:
                    exact['inactive'] = False
                if candidate == cold and actual != cold:
                    exact['equal_price'] = False
                if rank == 0 and (candidate != cold or actual != cold):
                    exact['new'] = False
                if previous is not None:
                    need(float(row[m+'_odds_before']) == float(previous[m+'_odds_after']),
                         at+': carried odds '+m)
            exact['new'] = exact['new'] and raw['new_exact'] == '1'
            exact['admission'] = exact['admission'] and (
                int(raw['episode_active_before']) == common['active_before'] and
                int(raw['episode_activated_after']) == common['admitted_after'] and
                int(row['active_before']) == common['active_before'] and
                int(row['admitted_after']) == common['admitted_after'])
            error = max(error, near(raw['episode_shadow_before'], common['shadow_before'],
                                    f'{source}: inherited shadow t={t}'),
                        near(raw['episode_live'], expected['slow']['live'],
                             f'{source}: inherited slow price t={t}'))
            exact['clock'] = exact['clock'] and all(
                expected['witness'][f] == expected['latch'][f] for f in ('w_before', 'w_after'))
            if previous is not None:
                need(float(row['w_before']) == float(previous['w_after']), at+': carried clock')
                need(int(row['latch_before']) == int(previous['latch_after']), at+': carried latch')
            before, after = int(row['latch_before']), int(row['latch_after'])
            need(before in (0, 1) and after in (0, 1), at+': latch is not a bit')
            wanted_w = float(row['w_before'])
            if common['active_before'] and matched >= 1:
                wanted_w = (31.0/32.0)*wanted_w+(candidate-cold)
            if common['admitted_after']:
                wanted_w = 0.0
            exact['clock'] = exact['clock'] and abs(float(row['w_after'])-wanted_w) <= TOL
            wanted_flag = before
            if common['admitted_after']:
                wanted_flag = 0
            elif common['active_before']:
                if float(row['w_after']) <= -1:
                    wanted_flag = 1
                elif float(row['w_after']) >= 1:
                    wanted_flag = 0
            exact['latch_law'] = exact['latch_law'] and after == wanted_flag
            if candidate == cold and before == 1:
                exact['latch_law'] = exact['latch_law'] and after == 1
            for key, wanted in fields.items():
                if isinstance(wanted, int):
                    need(int(row[key]) == wanted, at+': flag '+key)
                else:
                    error = max(error, near(row[key], wanted, at+': '+key))
            if before == 0 and after == 1:
                diagnostics['set_count'] += 1
                if diagnostics['first_set'] is None:
                    diagnostics['first_set'] = t
                if t < MOVE:
                    diagnostics['sets_before_seam'] += 1
                elif diagnostics['first_set_after_seam'] is None:
                    diagnostics['first_set_after_seam'] = t
            if before == 1 and after == 0:
                diagnostics['return_count'] += 1
                if diagnostics['first_return'] is None:
                    diagnostics['first_return'] = t
                if t < MOVE:
                    diagnostics['returns_before_seam'] += 1
            if t == MOVE:
                diagnostics['latch_at_seam'] = before
            previous = row
        need(next(inputs, None) is None, f'{source}: extra rows after {N}')
        need(next(outputs, None) is None, f'{replay}: extra rows after {N}')
    exact['admission'] = exact['admission'] and len({s.activation for s in states.values()}) == 1
    life = dict(world=world, regime=regime, modes={m: states[m].result() for m in MODES},
                diagnostics=diagnostics, exactness=exact, max_norm_error=maxnorm,
                archive_bytes=(HERE/'memory'/f'world{world}'/'episodes.bin').stat().st_size)
    return life, N*len(MODES), error


def compare(actual, expected, label):
    error = 0.0
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+': expected object')
        for key, value in expected.items():
            need(key in actual, label+': missing '+key)
            error = max(error, compare(actual[key], value, label+'/'+key))
    elif isinstance(expected, list):
        need(isinstance(actual, list) and len(actual) == len(expected), label+': list length')
        for i, (a, b) in enumerate(zip(actual, expected)):
            error = max(error, compare(a, b, label+f'/{i}'))
    elif isinstance(expected, (bool, str, int)) or expected is None:
        need(actual == expected, f'{label}: {actual} != {expected}')
    else:
        error = near(actual, expected, label)
    return error


def rebuild_gate(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    wanted = {(w, r) for w in WORLDS for r in REGIMES}
    need(len(lives) == len(by) and set(by) == wanted, 'RESULT.json: incomplete or duplicate life grid')
    mean = lambda values: math.fsum(values)/len(values)
    def values(regime, mode, field):
        return [by[w, regime]['modes'][mode][field] for w in WORLDS]
    def gap(regime, mode, reference, field):
        return [a-b for a, b in zip(values(regime, mode, field), values(regime, reference, field))]
    quantities, bars = {}, {}
    for m in MODES:
        law_tail = gap('switched', m, 'fast', 'tail')
        law_whole = gap('switched', m, 'fast', 'gain')
        moved = gap('moved_mid', m, 'fast', 'tail')
        moved_slow = gap('moved_mid', m, 'slow', 'tail')
        intact = values('recombined', m, 'gain')
        slow_early, slow_full = mean(values('recombined', 'slow', 'early')), mean(values('recombined', 'slow', 'gain'))
        early_ratio = mean(values('recombined', m, 'early'))/slow_early if slow_early > 0 else None
        full_ratio = mean(intact)/slow_full if slow_full > 0 else None
        quantities[m] = dict(law_tail_vs_fast=law_tail, law_tail_mean=mean(law_tail),
            law_whole_vs_fast=law_whole, law_whole_mean=mean(law_whole),
            moved_tail_vs_fast=moved, moved_tail_mean=mean(moved), moved_tail_wins=sum(x > 0 for x in moved),
            moved_tail_vs_slow=moved_slow, moved_tail_vs_slow_mean=mean(moved_slow),
            intact_early_retention=early_ratio, intact_full_retention=full_ratio, intact_full_gains=intact)
        bars[m] = dict(b1_law_tail_mean=mean(law_tail) >= -1,
                      b2_law_whole_mean=mean(law_whole) >= 0,
                      b3_moved_tail_mean=mean(moved) >= 1,
                      b4_moved_tail_wins=sum(x > 0 for x in moved) >= 5,
                      b5_moved_tail_vs_slow=mean(moved_slow) >= -3,
                      b6_intact_early_retention=early_ratio is not None and early_ratio >= .95,
                      b7_intact_full_retention=full_ratio is not None and full_ratio >= .95,
                      b8_intact_full_positive=all(x > 0 for x in intact))
    tables = {r: {m: {f: mean(values(r, m, f)) for f in ('gain', 'tail', 'early')}
                  for m in MODES} for r in REGIMES}
    validity = dict(
        admission=all(x['exactness']['admission'] for x in lives),
        archive=all(x['archive_bytes'] <= 528 for x in lives),
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= (10 if m == 'fast' else 16)+TOL
                   for x in lives for m, s in x['modes'].items()),
        normalization=all(0 <= x['max_norm_error'] <= 1e-8 for x in lives),
        exact_quotes=all(all(x['exactness'][f] for f in ('equal_price', 'inactive', 'new')) for x in lives),
        latch=all(x['exactness']['clock'] and x['exactness']['latch_law'] for x in lives))
    return bars, quantities, tables, validity


def identity():
    check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    check_artifact(HERE/'INTERFACE.md', INTERFACE_SHA)
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, value in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES)).items():
        need(frozen[key] == value, f'{HERE / "FREEZE.json"}: wrong {key}')
    for rel, wanted in frozen['files'].items():
        check_artifact(ROOT/rel, wanted)
    pins = dict(protocol_sha256=PROTOCOL_SHA, interface_sha256=INTERFACE_SHA,
                reader_sha256=digest(Path(__file__)), freeze_sha256=digest(HERE/'FREEZE.json'),
                result_sha256=digest(HERE/'RESULT.json'))
    for name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for rel, wanted in manifest.items():
            check_artifact(HERE/rel, wanted)
        pins[name] = digest(HERE/name)
    return pins


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), f'output file {args.output} already exists')
    result = json.loads((HERE/'RESULT.json').read_text())
    schema = check_schema(result)
    pins = identity()
    for key, value in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                           modes=list(MODES), protocol_sha256=PROTOCOL_SHA).items():
        need(result[key] == value, f'RESULT.json: wrong {key}')
    wanted = {(w, r) for w in WORLDS for r in REGIMES}
    recorded = {(x['world'], x['regime']): x for x in result['lives']}
    need(len(recorded) == len(result['lives']) and set(recorded) == wanted,
         'RESULT.json: incomplete or duplicate life grid')
    lives, count, error = [], 0, 0.0
    with localcontext() as ctx:
        ctx.prec = 50
        for w in WORLDS:
            for r in REGIMES:
                life, n, e = check_life(w, r)
                error = max(error, compare(recorded[w, r], life, f'RESULT.json: world{w}/{r}'))
                lives.append(life)
                count += n
                error = max(error, e)
                print('verified', w, r, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(MODES), 'incomplete authority forecast count')
    bars, quantities, tables, validity = rebuild_gate(lives)
    error = max(error, compare(result['bars'], bars, 'RESULT.json: bars'))
    error = max(error, compare(result['quantities'], quantities, 'RESULT.json: quantities'))
    error = max(error, compare(result['tables'], tables, 'RESULT.json: tables'))
    error = max(error, compare(result['validity'], validity, 'RESULT.json: validity'))
    material = all(bars['latch'].values())
    need(result['material_pass'] == material, 'RESULT.json: material verdict mismatch')
    receipt = dict(verification_pass=True, material_pass=material, validity_pass=all(validity.values()),
                   acceptance_pass=material and all(validity.values()),
                   namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES), modes=list(MODES),
                   forecasts=count, maximum_numeric_error=error,
                   identities=pins, required_result_keys=schema,
                   bars=bars, quantities=quantities, tables=tables, validity=validity, lives=lives,
                   scope='Independent four-mode Decimal authority replay from pinned P0/candidate/match inputs; '
                         'every quote/state, chronology, latch transitions and reported material metrics. '
                         'No source-book, candidate or HEAD256 rebuild; frontend normalization is a retained C receipt. '
                         'Descriptive raw-help/harm selection is outside this reader.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: receipt[k] for k in ('verification_pass', 'material_pass', 'validity_pass',
          'acceptance_pass', 'forecasts', 'maximum_numeric_error', 'validity')}, indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (Refusal, OSError, KeyError, ValueError) as exc:
        print('verify.py refuses: '+str(exc), file=sys.stderr)
        sys.exit(1)
