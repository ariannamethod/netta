"""Post-result reader repair in the turn20 tradition: a SEPARATE file.
The frozen verify.py ran to completion, agreed with the writer on all seven
gated verdicts and the c1..c6 detail blocks, then refused with KeyError 'c7':
its details dict (line ~547) builds c1..c6 while the comparison loop iterates
every gated test's prefix, c7 included. This copy adds exactly one entry —
c7=dict(hygiene) — the same hygiene dict the reader had already compared and
gated. The frozen verify.py is untouched and its refusal (VERIFY.rc,
VERIFY.stderr) remains part of the record. The turn24 amendment walls are not
invoked: no frozen byte changes. Don, 2026-09-25."""

#!/usr/bin/env python3
"""Independent turn27 probability reader; no writer or predictor imports.

The P0/candidate/match tape is pinned evidence. Six authority laws, all
prices and states, the CUSUM statistic and its one-way latch, the hysteresis
clock, life metrics, the eight-tooth gate and the raw windows are rebuilt from
scratch in probability space. The generator's label is confined by construction:
Authority refuses a seam for any arm but the yardstick, and refuses to build the
yardstick without one, so a gated arm that had consulted the seam could not
agree with this recompute. HEAD256/source books are not rebuilt here.
"""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import math
from pathlib import Path
import random
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(256, 264))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
ARMS = ('slow', 'fast', 'witness', 'hysteresis', 'cusum', 'orate0')
CANDIDATE = 'cusum'
INCUMBENT = 'hysteresis'
YARDSTICK = 'orate0'
NAMESPACE = 'netta-cusum-latch-v1'
N, MOVE, EARLY = 16384, 8192, 4096
SEAM_REGIME = 'switched'
DRIFT, THRESHOLD = 0.5, 8.0
LATCH_DEADLINE = MOVE+256
HORIZONS = (1024, EARLY, MOVE, N)
TOL = 1e-7
HALF = Decimal('0.5')
PROTOCOL_SHA = '9264908b35d0ce89025d0bce6d9cecc6e25e8cf28c5d761072154ce6e3ca6df1'
REQUIRED_RESULT_KEYS = frozenset((
    'namespace worlds regimes arms candidate incumbent yardstick seam seam_regime '
    'drift threshold latch_deadline protocol_sha256 labels_sha256 labels lives '
    'quantities bars tests test_details hygiene false_latch_census table life_table '
    'cusum_trajectories unrelated_admissions fast_own_drawdown_bound gate_pass '
    'gate_pass_pending_reader independent_reader_pending').split())
COLUMNS = ('t cold candidate matched shadow_before active_before admitted_after '
           'w_before w_after hysteresis_latch_before hysteresis_latch_after '
           's_before s_after cusum_latch_before cusum_latch_after orate0_armed').split() + [
    a+'_'+f for a in ARMS for f in ('live', 'odds_before', 'slow_used', 'odds_after')]
GATED_TESTS = ('c1_law_tail', 'c2_law_whole', 'c3_moved', 'c4_recombined',
               'c5_incumbent', 'c6_mechanism', 'c7_hygiene')


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
    for key in ('worlds', 'regimes', 'arms', 'lives', 'false_latch_census',
                'unrelated_admissions'):
        need(isinstance(result[key], list), f'RESULT.json: {key} must be a list')
    for key in ('quantities', 'bars', 'tests', 'test_details', 'hygiene', 'table',
                'life_table', 'cusum_trajectories', 'labels'):
        need(isinstance(result[key], dict), f'RESULT.json: {key} must be an object')
    for key in ('gate_pass', 'gate_pass_pending_reader', 'independent_reader_pending',
                'fast_own_drawdown_bound'):
        need(isinstance(result[key], bool), f'RESULT.json: {key} must be boolean')
    for key in ('namespace', 'candidate', 'incumbent', 'yardstick', 'seam_regime',
                'protocol_sha256', 'labels_sha256'):
        need(isinstance(result[key], str), f'RESULT.json: {key} must be a string')
    for key in ('seam', 'latch_deadline'):
        need(isinstance(result[key], int), f'RESULT.json: {key} must be an integer')
    for key in ('drift', 'threshold'):
        need(isinstance(result[key], (int, float)) and not isinstance(result[key], bool),
             f'RESULT.json: {key} must be a number')
    return sorted(REQUIRED_RESULT_KEYS)


def mass_log2(value):
    need(value > 0, 'authority: nonpositive probability mass')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Authority:
    """One mass trajectory. The seam reaches exactly the yardstick arm."""

    def __init__(self, mode, seam=None):
        need(mode in ARMS, f'authority: unknown arm {mode}')
        need((seam is None) != (mode == YARDSTICK),
             f'authority: the generator label reaches {YARDSTICK} and nothing else')
        self.mode = mode
        self.seam = -1 if seam is None else seam
        self.source = self.cold = HALF
        self.shadow = self.w = self.sum = 0.0
        self.active = False
        self.latched = 0
        self.armed = 0
        self.gain = self.tail = self.minimum = self.peak = self.drawdown = 0.0
        self.activation = None
        self.horizons = {}
        self.slow_hazard_bytes = self.fast_hazard_bytes = self.tail_fast_hazard_bytes = 0

    def odds(self):
        return mass_log2(self.source/self.cold) if self.active else 0.0

    def step(self, t, cold, candidate, matched):
        need(math.isfinite(cold) and math.isfinite(candidate), f'{self.mode}: nonfinite price')
        need(isinstance(matched, int) and matched >= 0, f'{self.mode}: invalid match length')
        if self.mode == YARDSTICK and self.seam >= 0 and t == self.seam:
            need(not self.armed, f'{self.mode}: a second switch on one life')
            self.armed = 1
        before = dict(shadow_before=self.shadow, active_before=int(self.active),
                      odds_before=self.odds(), w_before=self.w, s_before=self.sum,
                      hysteresis_latch_before=self.latched,
                      cusum_latch_before=self.latched, orate0_armed=self.armed)
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
            elif self.mode == YARDSTICK:
                slow_used = 0 if self.armed else 1
            else:
                slow_used = 1-self.latched
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            self.source = (1-hazard)*joint/total
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, f'{self.mode}: nonpositive updated mass')
            near(self.source+self.cold, 1, f'{self.mode}: mass normalization', 1e-12)
            if self.mode in ('witness', INCUMBENT) and matched >= 1:
                self.w = (31.0/32.0)*self.w+delta
            if self.mode == INCUMBENT:
                if self.w <= -1.0:
                    self.latched = 1
                elif self.w >= 1.0:
                    self.latched = 0
            if self.mode == CANDIDATE:
                if matched >= 1:
                    raised = self.sum+(-delta-DRIFT)
                    self.sum = raised if raised > 0.0 else 0.0
                need(self.sum >= 0.0, f'{self.mode}: statistic left its floor')
                if self.sum >= THRESHOLD:
                    self.latched = 1
            if slow_used:
                self.slow_hazard_bytes += 1
            else:
                self.fast_hazard_bytes += 1
                if t >= MOVE:
                    self.tail_fast_hazard_bytes += 1
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
            self.source = self.cold = HALF
            self.w = self.sum = 0.0
            self.latched = 0
            self.activation = t+1
            admitted = True
        if t+1 in HORIZONS:
            self.horizons[str(t+1)] = self.gain
        return dict(**before, live=live, slow_used=slow_used, odds_after=self.odds(),
                    w_after=self.w, s_after=self.sum,
                    hysteresis_latch_after=self.latched, cusum_latch_after=self.latched,
                    admitted_after=int(admitted))

    def result(self):
        return dict(gain=self.gain, tail=self.tail, early=self.horizons[str(EARLY)],
                    minimum=self.minimum, peak=self.peak, drawdown=self.drawdown,
                    activation=self.activation, horizons=self.horizons,
                    slow_hazard_bytes=self.slow_hazard_bytes,
                    fast_hazard_bytes=self.fast_hazard_bytes,
                    tail_fast_hazard_bytes=self.tail_fast_hazard_bytes)


def derive_labels(world):
    """Re-derive the hidden label from the generator's own data, independently."""
    folder = HERE/'data'/f'world{world}'
    declared = json.loads((folder/'GENERATOR.json').read_text())
    need(declared['world'] == world, f'world {world}: GENERATOR.json names another world')
    seeds = declared['emission_seeds']
    need(seeds[SEAM_REGIME] == seeds['recombined'],
         f'world {world}: the switched life does not share the recombined emission seed')
    raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
    cmd = {r: (folder/(r+'.commands.bin')).read_bytes()
           for r in ('recombined', SEAM_REGIME, 'unrelated')}
    need(not (folder/'moved_mid.commands.bin').exists(),
         f'world {world}: moved_mid acquired a command tape')
    need(all(len(b) == N for b in raw.values()), f'world {world}: target length')
    need(raw[SEAM_REGIME][:MOVE] == raw['recombined'][:MOVE],
         f'world {world}: the switched prefix is not the recombined prefix')
    need(cmd[SEAM_REGIME] == cmd['recombined'][:MOVE]+cmd['unrelated'][MOVE:],
         f'world {world}: the switched command tape is not the declared construction')
    order = list(range(256))
    seed = hashlib.sha256(f'{NAMESPACE}|{world}|surface'.encode()).digest()[:8]
    random.Random(int.from_bytes(seed, 'big')).shuffle(order)
    need(raw['moved_mid'] == raw['recombined'][:MOVE] +
         bytes(order[b] for b in raw['recombined'][MOVE:]),
         f'world {world}: the moved surface is not the declared bijection')
    differing = sum(a != b for a, b in zip(raw[SEAM_REGIME][MOVE:], raw['recombined'][MOVE:]))
    return dict(world=world, seam=MOVE, law_changed_regime=SEAM_REGIME,
                generator_sha256=digest(folder/'GENERATOR.json'),
                shared_emission_seed=seeds[SEAM_REGIME],
                raw_prefix_identical_to_recombined=True,
                command_tape_is_declared_construction=True,
                raw_tail_differing_bytes=differing,
                raw_tail_divergence=differing/(N-MOVE),
                raw_tail_differing_bytes_vs_unrelated=sum(
                    a != b for a, b in zip(raw[SEAM_REGIME][MOVE:], raw['unrelated'][MOVE:])))


def check_life(world, regime, saved, label):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    replay = HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    check_artifact(source, saved['source_sha256'])
    check_artifact(replay, saved['replay_sha256'])
    seam = label['seam'] if regime == label['law_changed_regime'] else -1
    need(saved['seam'] == seam, f'world {world} {regime}: recorded seam {saved["seam"]}')
    arms = {a: Authority(a, seam if a == YARDSTICK else None) for a in ARMS}
    cusum = dict(latched=False, latch_byte=None, latch_before_seam=None, s_at_seam=None,
                 s_final=0., s_max=0., s_max_byte=None, s_max_before_seam=0.,
                 s_max_after_seam=None, s_after_at_latch=None, voting_bytes=0,
                 voting_bytes_after_seam=0)
    hysteresis = dict(first_set=None, first_return=None, set_count=0, return_count=0,
                      sets_before_seam=0, returns_before_seam=0,
                      first_set_after_seam=None, latch_at_seam=None)
    exact = dict(admission=True, inactive=True, equal_price=True, protected_new=True,
                 new_column=True, normalized=True, witness_clock=True,
                 hysteresis_law=True, cusum_statistic=True, cusum_one_way=True,
                 cusum_chronology=True, orate0_off_law_is_slow=True,
                 orate0_pre_seam_is_slow=True, orate0_armed_is_fast=True)
    error = maxnorm = 0.0
    previous = None
    rows = []
    with gzip.open(source, 'rt') as sf, gzip.open(replay, 'rt') as af:
        inputs, outputs = csv.DictReader(sf, delimiter='\t'), csv.DictReader(af, delimiter='\t')
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
            need(math.isfinite(normalization) and normalization >= 0,
                 f'{source}: normalization at {t}')
            maxnorm = max(maxnorm, normalization)
            step = {a: arms[a].step(t, cold, candidate, matched) for a in ARMS}
            common = step['slow']
            fields = {k: common[k] for k in ('shadow_before', 'active_before', 'admitted_after')}
            fields.update(w_before=step[INCUMBENT]['w_before'],
                          w_after=step[INCUMBENT]['w_after'],
                          hysteresis_latch_before=step[INCUMBENT]['hysteresis_latch_before'],
                          hysteresis_latch_after=step[INCUMBENT]['hysteresis_latch_after'],
                          s_before=step[CANDIDATE]['s_before'],
                          s_after=step[CANDIDATE]['s_after'],
                          cusum_latch_before=step[CANDIDATE]['cusum_latch_before'],
                          cusum_latch_after=step[CANDIDATE]['cusum_latch_after'],
                          orate0_armed=step[YARDSTICK]['orate0_armed'])
            for a in ARMS:
                for f in ('live', 'odds_before', 'slow_used', 'odds_after'):
                    fields[a+'_'+f] = step[a][f]
                for f in ('shadow_before', 'active_before', 'admitted_after'):
                    need(step[a][f] == common[f], at+': independent admission disagreement '+a)
                actual = float(row[a+'_live'])
                if not common['active_before'] and actual != cold:
                    exact['inactive'] = False
                if candidate == cold and actual != cold:
                    exact['equal_price'] = False
                if rank == 0 and (candidate != cold or actual != cold):
                    exact['protected_new'] = False
                if previous is not None:
                    need(float(row[a+'_odds_before']) == float(previous[a+'_odds_after']),
                         at+': carried odds '+a)
            exact['new_column'] = exact['new_column'] and raw['new_exact'] == '1'
            exact['admission'] = exact['admission'] and (
                int(raw['episode_active_before']) == common['active_before'] and
                int(raw['episode_activated_after']) == common['admitted_after'] and
                int(row['active_before']) == common['active_before'] and
                int(row['admitted_after']) == common['admitted_after'])
            error = max(error, near(raw['episode_shadow_before'], common['shadow_before'],
                                    f'{source}: inherited shadow t={t}'),
                        near(raw['episode_live'], step['slow']['live'],
                             f'{source}: inherited slow price t={t}'))
            exact['witness_clock'] = exact['witness_clock'] and all(
                step['witness'][f] == step[INCUMBENT][f] for f in ('w_before', 'w_after'))
            if previous is not None:
                need(float(row['w_before']) == float(previous['w_after']), at+': carried clock')
                need(float(row['s_before']) == float(previous['s_after']), at+': carried sum')
                for flag in ('hysteresis_latch', 'cusum_latch'):
                    need(int(row[flag+'_before']) == int(previous[flag+'_after']),
                         at+': carried '+flag)
            for key, wanted in fields.items():
                if isinstance(wanted, int):
                    need(int(row[key]) == wanted, at+': flag '+key)
                else:
                    error = max(error, near(row[key], wanted, at+': '+key))

            # The three laws, re-derived from the recorded state a second time.
            oldw, neww = float(row['w_before']), float(row['w_after'])
            oldl, newl = int(row['hysteresis_latch_before']), int(row['hysteresis_latch_after'])
            wantw = (31./32.)*oldw+(candidate-cold) if common['active_before'] and matched >= 1 else oldw
            if common['admitted_after']:
                wantw = 0.
            exact['witness_clock'] = exact['witness_clock'] and abs(wantw-neww) <= 1e-10
            wantl = (1 if neww <= -1 else 0 if neww >= 1 else oldl) if common['active_before'] else 0
            exact['hysteresis_law'] = exact['hysteresis_law'] and newl == wantl
            if common['active_before']:
                exact['hysteresis_law'] = (exact['hysteresis_law'] and
                                           int(row['hysteresis_slow_used']) == 1-oldl)
            olds, news = float(row['s_before']), float(row['s_after'])
            oldc, newc = int(row['cusum_latch_before']), int(row['cusum_latch_after'])
            wants, wantc = olds, oldc
            if common['active_before']:
                if matched >= 1:
                    raised = olds+(-(candidate-cold)-DRIFT)
                    wants = raised if raised > 0. else 0.
                if wants >= THRESHOLD:
                    wantc = 1
            elif common['admitted_after']:
                wants, wantc = 0., 0
            exact['cusum_statistic'] = exact['cusum_statistic'] and news == wants
            exact['cusum_one_way'] = (exact['cusum_one_way'] and newc == wantc and
                                      newc >= oldc and news >= 0.)
            if common['active_before']:
                exact['cusum_chronology'] = (exact['cusum_chronology'] and
                                             int(row['cusum_slow_used']) == 1-oldc)
            armed = int(row['orate0_armed'])
            need(armed == int(seam >= 0 and t >= seam), at+': oracle arming')
            same = (row[YARDSTICK+'_live'] == row['slow_live'] and
                    row[YARDSTICK+'_odds_after'] == row['slow_odds_after'] and
                    row[YARDSTICK+'_slow_used'] == row['slow_slow_used'])
            if seam < 0:
                exact['orate0_off_law_is_slow'] = exact['orate0_off_law_is_slow'] and same
            elif not armed:
                exact['orate0_pre_seam_is_slow'] = exact['orate0_pre_seam_is_slow'] and same
            else:
                exact['orate0_armed_is_fast'] = (exact['orate0_armed_is_fast'] and
                                                 int(row[YARDSTICK+'_slow_used']) != 1)

            if t == MOVE:
                hysteresis['latch_at_seam'] = oldl
                cusum['s_at_seam'] = olds
            if newl != oldl:
                kind = 'set' if newl else 'return'
                hysteresis[kind+'_count'] += 1
                if hysteresis['first_'+kind] is None:
                    hysteresis['first_'+kind] = t
                if t < MOVE:
                    hysteresis[kind+'s_before_seam'] += 1
                elif newl and hysteresis['first_set_after_seam'] is None:
                    hysteresis['first_set_after_seam'] = t
            if common['active_before'] and matched >= 1:
                cusum['voting_bytes'] += 1
                if t >= MOVE:
                    cusum['voting_bytes_after_seam'] += 1
            if news > cusum['s_max']:
                cusum['s_max'], cusum['s_max_byte'] = news, t
            if t < MOVE:
                cusum['s_max_before_seam'] = max(cusum['s_max_before_seam'], news)
            else:
                cusum['s_max_after_seam'] = max(cusum['s_max_after_seam'] or 0., news)
            if newc and not oldc:
                need(cusum['latch_byte'] is None, at+': a second cusum latch on one life')
                cusum.update(latched=True, latch_byte=t, s_after_at_latch=news,
                             latch_before_seam=t < MOVE)
            cusum['s_final'] = news
            rows.append((t, int(raw['truth']), rank, matched, cold, candidate,
                         {a: float(row[a+'_live']) for a in ARMS},
                         int(row['cusum_slow_used']), olds, news, oldc, newc))
            previous = row
        need(next(inputs, None) is None, f'{source}: extra rows after {N}')
        need(next(outputs, None) is None, f'{replay}: extra rows after {N}')
    exact['normalized'] = 0 <= maxnorm <= 1e-8
    metrics = {a: arms[a].result() for a in ARMS}
    for a, s in metrics.items():
        need(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL,
             f'world {world} {regime} {a}: prefix/interval loss bound')
    need(len({s['activation'] for s in metrics.values()}) == 1,
         f'world {world} {regime}: admission is not shared')
    centre = cusum['latch_byte'] if cusum['latched'] else (MOVE if seam >= 0 else None)
    radius = 32 if cusum['latched'] and regime != SEAM_REGIME else 8
    window = []
    if centre is not None:
        for row in rows[max(0, centre-radius):min(N, centre+radius+1)]:
            t, truth, rank, matched, cold, candidate, live, used, olds, news, oldc, newc = row
            window.append(dict(t=t, truth=truth, rank=rank, matchedL=matched, cold=cold,
                               candidate=candidate, delta=candidate-cold,
                               cusum_live=live[CANDIDATE], slow_live=live['slow'],
                               fast_live=live['fast'], cusum_slow_used=used,
                               s_before=olds, s_after=news,
                               latch_before=oldc, latch_after=newc))
    life = dict(world=world, regime=regime, seam=seam, arms=metrics, cusum=cusum,
                hysteresis=hysteresis, exact=exact, admission=metrics['slow']['activation'],
                max_norm_error=maxnorm,
                archive_bytes=(HERE/'memory'/f'world{world}'/'episodes.bin').stat().st_size,
                source_sha256=saved['source_sha256'], replay_sha256=saved['replay_sha256'],
                raw_window_centre=centre, raw_window=window)
    return life, N*len(ARMS), error


def compare(actual, expected, label):
    error = 0.0
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+': expected object')
        need(set(actual) == set(expected),
             label+': keys differ; only in RESULT '+str(sorted(set(actual)-set(expected)))+
             ', only in reader '+str(sorted(set(expected)-set(actual))))
        for key, value in expected.items():
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


def mean(values):
    return math.fsum(values)/len(values)


def rebuild(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    need(len(lives) == len(by) and set(by) == {(w, r) for w in WORLDS for r in REGIMES},
         'incomplete or duplicate life grid')

    def gap(regime, arm, reference, field):
        return [by[w, regime]['arms'][arm][field]-by[w, regime]['arms'][reference][field]
                for w in WORLDS]
    quantities, bars = {}, {}
    slow_early = mean([by[w, 'recombined']['arms']['slow']['early'] for w in WORLDS])
    slow_full = mean([by[w, 'recombined']['arms']['slow']['gain'] for w in WORLDS])
    for a in ARMS:
        law_tail, law_whole = gap(SEAM_REGIME, a, 'fast', 'tail'), gap(SEAM_REGIME, a, 'fast', 'gain')
        moved_fast, moved_slow = gap('moved_mid', a, 'fast', 'tail'), gap('moved_mid', a, 'slow', 'tail')
        intact = [by[w, 'recombined']['arms'][a]['gain'] for w in WORLDS]
        early = mean([by[w, 'recombined']['arms'][a]['early'] for w in WORLDS])
        q = dict(law_tail_vs_fast=law_tail, law_tail_mean=mean(law_tail),
                 law_tail_worlds_at_least_minus_one=sum(x >= -1 for x in law_tail),
                 law_whole_vs_fast=law_whole, law_whole_mean=mean(law_whole),
                 law_whole_worlds_at_least_zero=sum(x >= 0 for x in law_whole),
                 moved_tail_vs_fast=moved_fast, moved_tail_mean=mean(moved_fast),
                 moved_tail_wins=sum(x > 0 for x in moved_fast),
                 moved_tail_vs_slow=moved_slow, moved_tail_vs_slow_mean=mean(moved_slow),
                 intact_full_gains=intact, intact_early_mean=early,
                 intact_full_mean=mean(intact),
                 intact_full_positive=sum(x > 0 for x in intact),
                 intact_early_retention=early/slow_early if slow_early > 0 else None,
                 intact_full_retention=mean(intact)/slow_full if slow_full > 0 else None)
        quantities[a] = q
        bars[a] = dict(b1_law_tail_mean=q['law_tail_mean'] >= -1,
                       b2_law_whole_mean=q['law_whole_mean'] >= 0,
                       b3_moved_tail_mean=q['moved_tail_mean'] >= 1,
                       b4_moved_tail_wins=q['moved_tail_wins'] >= 5,
                       b5_moved_tail_vs_slow=q['moved_tail_vs_slow_mean'] >= -3,
                       b6_intact_early_retention=q['intact_early_retention'] is not None and
                       q['intact_early_retention'] >= .95,
                       b7_intact_full_retention=q['intact_full_retention'] is not None and
                       q['intact_full_retention'] >= .95,
                       b8_intact_full_positive=q['intact_full_positive'] == len(WORLDS))
    q, inc = quantities[CANDIDATE], quantities[INCUMBENT]
    census = [dict(world=x['world'], regime=x['regime'], latch_byte=x['cusum']['latch_byte'],
                   s_max=x['cusum']['s_max'], s_max_byte=x['cusum']['s_max_byte'])
              for x in lives if x['regime'] != SEAM_REGIME and x['cusum']['latched']]
    switched = [by[w, SEAM_REGIME] for w in WORLDS]
    in_time = [x['world'] for x in switched
               if x['cusum']['latched'] and x['cusum']['latch_byte'] <= LATCH_DEADLINE]
    hygiene = dict(
        admission_identity=all(len({s['activation'] for s in x['arms'].values()}) == 1
                               for x in lives),
        admission_shared_with_episode=all(x['exact']['admission'] for x in lives),
        unrelated_admissions_reported=(
            len([x for x in lives if x['regime'] == 'unrelated']) == len(WORLDS) and
            all(set(x['arms']) == set(ARMS) for x in lives if x['regime'] == 'unrelated')),
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                   for x in lives for s in x['arms'].values()),
        distributions_positive_and_normalized=all(
            x['exact']['normalized'] and x['exact']['new_column'] for x in lives),
        exact_quotes=all(x['exact'][k] for x in lives
                         for k in ('inactive', 'equal_price', 'protected_new')),
        label_confined=all(x['exact'][k] for x in lives
                           for k in ('orate0_off_law_is_slow', 'orate0_pre_seam_is_slow',
                                     'orate0_armed_is_fast')),
        laws_recomputed=all(x['exact'][k] for x in lives
                            for k in ('witness_clock', 'hysteresis_law', 'cusum_statistic',
                                      'cusum_one_way', 'cusum_chronology')),
        archive=all(x['archive_bytes'] <= 528 for x in lives))
    details = dict(
        c1=dict(law_tail_mean=q['law_tail_mean'], law_tail_vs_fast=q['law_tail_vs_fast'],
                worlds_at_least_minus_one=q['law_tail_worlds_at_least_minus_one']),
        c2=dict(law_whole_mean=q['law_whole_mean'], law_whole_vs_fast=q['law_whole_vs_fast']),
        c3=dict(moved_tail_mean=q['moved_tail_mean'], moved_tail_wins=q['moved_tail_wins'],
                moved_tail_vs_slow_mean=q['moved_tail_vs_slow_mean'],
                moved_tail_vs_fast=q['moved_tail_vs_fast']),
        c4=dict(intact_early_retention=q['intact_early_retention'],
                intact_full_retention=q['intact_full_retention'],
                intact_full_positive=q['intact_full_positive'],
                intact_full_gains=q['intact_full_gains']),
        c5=dict(cusum_law_tail_mean=q['law_tail_mean'],
                hysteresis_law_tail_mean=inc['law_tail_mean'],
                margin=q['law_tail_mean']-inc['law_tail_mean']),
        c6=dict(false_latch_count=len(census), false_latches=census,
                switched_latch_bytes={str(x['world']): x['cusum']['latch_byte']
                                      for x in switched},
                switched_latched_by_deadline=len(in_time), latch_deadline=LATCH_DEADLINE,
                switched_latched_before_seam=[x['world'] for x in switched
                                              if x['cusum']['latch_before_seam']],
                intact_s_max={f"{x['world']}/{x['regime']}": x['cusum']['s_max']
                              for x in lives if x['regime'] != SEAM_REGIME}),
        c7=dict(hygiene))
    tests = dict(
        c1_law_tail=q['law_tail_mean'] >= -1 and q['law_tail_worlds_at_least_minus_one'] >= 5,
        c2_law_whole=q['law_whole_mean'] >= 0,
        c3_moved=(q['moved_tail_mean'] >= 1 and q['moved_tail_wins'] >= 5 and
                  q['moved_tail_vs_slow_mean'] >= -3),
        c4_recombined=(q['intact_early_retention'] is not None and
                       q['intact_early_retention'] >= .95 and
                       q['intact_full_retention'] is not None and
                       q['intact_full_retention'] >= .95 and
                       q['intact_full_positive'] == len(WORLDS)),
        c5_incumbent=q['law_tail_mean']-inc['law_tail_mean'] > 1,
        c6_mechanism=not census and len(in_time) >= 6,
        c7_hygiene=all(hygiene.values()),
        c8_reader=True)
    table = {r: {a: {f: mean([by[w, r]['arms'][a][f] for w in WORLDS])
                     for f in ('early', 'tail', 'gain')} for a in ARMS} for r in REGIMES}
    life_table = {r: {a: {str(w): by[w, r]['arms'][a]['gain'] for w in WORLDS}
                      for a in ARMS} for r in REGIMES}
    return dict(quantities=quantities, bars=bars, tests=tests, test_details=details,
                hygiene=hygiene, false_latch_census=census, table=table,
                life_table=life_table,
                cusum_trajectories={f"{x['world']}/{x['regime']}": x['cusum'] for x in lives},
                unrelated_admissions=[
                    dict(world=x['world'], regime=x['regime'], admission=x['admission'],
                         gains={a: x['arms'][a]['gain'] for a in ARMS})
                    for x in lives if x['regime'] == 'unrelated'],
                fast_own_drawdown_bound=all(x['arms']['fast']['drawdown'] <= 10+TOL
                                            for x in lives))


def identity(result):
    check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, value in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                           arms=list(ARMS), seam=MOVE, seam_regime=SEAM_REGIME,
                           drift=DRIFT, threshold=THRESHOLD).items():
        need(frozen[key] == value, f'{HERE / "FREEZE.json"}: wrong {key}')
    amendment = HERE/'FREEZE_AMENDMENT.json'
    amended = {}
    if amendment.exists():
        note = json.loads(amendment.read_text())
        for name, pair in note['files'].items():
            need(pair['frozen_sha256'] == frozen['files'][name],
                 f'FREEZE_AMENDMENT.json: {name} does not carry its frozen sha')
            amended[name] = pair['amended_sha256']
    for rel, wanted in frozen['files'].items():
        got = digest(ROOT/rel)
        need(got == wanted or got == amended.get(rel),
             f'retained artifact {ROOT / rel}: sha256 {got} != {wanted}')
    check_artifact(HERE/'labels'/'GENERATOR_LABELS.json', result['labels_sha256'])
    pins = dict(protocol_sha256=PROTOCOL_SHA, reader_sha256=digest(Path(__file__)),
                freeze_sha256=digest(HERE/'FREEZE.json'), result_sha256=digest(HERE/'RESULT.json'),
                labels_sha256=result['labels_sha256'], amended_files=sorted(amended))
    for name in ('DATA_MANIFEST.json', 'EXTRACT_MANIFEST.json', 'MEMORY_MANIFEST.json',
                 'LABELS_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        for rel, wanted in json.loads((HERE/name).read_text()).items():
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
    pins = identity(result)
    for key, value in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                           arms=list(ARMS), candidate=CANDIDATE, incumbent=INCUMBENT,
                           yardstick=YARDSTICK, seam=MOVE, seam_regime=SEAM_REGIME,
                           drift=DRIFT, threshold=THRESHOLD, latch_deadline=LATCH_DEADLINE,
                           protocol_sha256=PROTOCOL_SHA).items():
        need(result[key] == value, f'RESULT.json: wrong {key}')
    need(result['tests']['c8_reader'] is False and result['independent_reader_pending'] is True,
         'RESULT.json: the writer must not claim the independent reader')

    declared = json.loads((HERE/'labels'/'GENERATOR_LABELS.json').read_text())
    need(declared['seam'] == MOVE and declared['law_changed_regime'] == SEAM_REGIME,
         'the label file does not declare the constructional seam')
    labels = {}
    for w in WORLDS:
        mine, theirs = derive_labels(w), declared['worlds'][str(w)]
        for key, value in mine.items():
            need(theirs[key] == value,
                 f'world {w} label {key}: reader says {value}, the label file says {theirs[key]}')
        need(result['labels'][str(w)] == theirs,
             f'world {w}: RESULT.json label differs from GENERATOR_LABELS.json')
        labels[w] = mine

    recorded = {(x['world'], x['regime']): x for x in result['lives']}
    need(len(recorded) == len(result['lives']) and
         set(recorded) == {(w, r) for w in WORLDS for r in REGIMES},
         'RESULT.json: incomplete or duplicate life grid')
    lives, count, error = [], 0, 0.0
    with localcontext() as ctx:
        ctx.prec = 50
        for w in WORLDS:
            for r in REGIMES:
                saved = recorded[w, r]
                life, n, e = check_life(w, r, saved, labels[w])
                error = max(error, compare(saved, life, f'RESULT.json: world{w}/{r}'), e)
                lives.append(life)
                count += n
                print('verified', w, r, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(ARMS), 'incomplete authority forecast count')
    rebuilt = rebuild(lives)
    for key in ('quantities', 'bars', 'hygiene', 'false_latch_census', 'table',
                'life_table', 'cusum_trajectories', 'unrelated_admissions'):
        error = max(error, compare(result[key], rebuilt[key], 'RESULT.json: '+key))
    need(result['fast_own_drawdown_bound'] == rebuilt['fast_own_drawdown_bound'],
         'RESULT.json: fast drawdown bound mismatch')
    for name in GATED_TESTS:
        need(result['tests'][name] == rebuilt['tests'][name],
             f'RESULT.json: {name} is {result["tests"][name]}, the reader says '
             f'{rebuilt["tests"][name]}')
        error = max(error, compare(result['test_details'][name[:2]],
                                   rebuilt['test_details'][name[:2]],
                                   'RESULT.json: test_details/'+name[:2]))
    need('c8' in result['test_details'], 'RESULT.json: test_details/c8 is missing')
    need(result['gate_pass_pending_reader'] ==
         all(rebuilt['tests'][name] for name in GATED_TESTS),
         'RESULT.json: material verdict mismatch')
    gate_pass = all(rebuilt['tests'].values())
    receipt = dict(verification_pass=True, gate_pass=gate_pass,
                   tests=rebuilt['tests'], test_details=rebuilt['test_details'],
                   hygiene=rebuilt['hygiene'], quantities=rebuilt['quantities'],
                   bars=rebuilt['bars'], table=rebuilt['table'],
                   life_table=rebuilt['life_table'],
                   cusum_trajectories=rebuilt['cusum_trajectories'],
                   false_latch_census=rebuilt['false_latch_census'],
                   unrelated_admissions=rebuilt['unrelated_admissions'],
                   fast_own_drawdown_bound=rebuilt['fast_own_drawdown_bound'],
                   namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                   arms=list(ARMS), forecasts=count, maximum_numeric_error=error,
                   identities=pins, required_result_keys=schema, labels=labels, lives=lives,
                   scope='Independent six-arm Decimal authority replay from pinned '
                         'P0/candidate/match inputs: every quote, state, carried odds, the '
                         'witness clock, turn25\'s hysteresis latch, this turn\'s CUSUM '
                         'statistic and one-way latch, the O-rate-0 confinement, the raw '
                         'windows and the whole C1..C7 gate. The generator label is re-derived '
                         'from data/ and reaches only the yardstick arm. No source-book, '
                         'candidate or HEAD256 rebuild; frontend normalization is a retained '
                         'C receipt.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: receipt[k] for k in ('verification_pass', 'gate_pass', 'tests',
                                              'hygiene', 'forecasts', 'maximum_numeric_error')},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (Refusal, OSError, KeyError, ValueError) as exc:
        print('verify.py refuses: '+str(exc), file=sys.stderr)
        sys.exit(1)
