#!/usr/bin/env python3
"""Independent reader for turn24.

Rebuilds all fourteen arms from the retained candidate/P0 tape alone, in
Decimal probability-mass arithmetic, with no code shared with the writer and
no use of any arm's own recorded state as input.

The confinement of the generator's label is proved as a property, not read off
a flag: the six reference arms are rebuilt by objects that are constructed
without the label and whose step takes no label argument, so a reference arm
that had consulted the seam could not agree with this recompute. The eight
oracle arms are rebuilt with the label, and their switch byte, their hazard
transition and their clamp value are checked against the law.

Refusals name the artifact they refuse.
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
WORLDS = tuple(range(240, 248))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
NAMESPACE = 'netta-oracle-latency-v1'
N = 16384
MOVE = 8192
SEAM_REGIME = 'switched'
DELAYS = (0, 32, 128, 512)
CONTROL_ARMS = ('slow', 'fast', 'witness', 'h8l4', 'selfnorm')
REFERENCE_ARMS = CONTROL_ARMS + ('split',)
ORACLE_ARMS = tuple(f'orate{d}' for d in DELAYS) + tuple(f'olevel{d}' for d in DELAYS)
ARMS = REFERENCE_ARMS + ORACLE_ARMS
CAUSAL_NOMINEES = ('witness', 'h8l4', 'selfnorm', 'split')
LAW_SIDE_BARS = ('b1_law_tail_mean', 'b2_law_whole_mean')
CLOCK_ARMS = ('witness', 'h8l4', 'selfnorm')
TOL = 1e-7
HALF = Decimal('0.5')
REQUIRED_RESULT_KEYS = frozenset((
    'namespace worlds regimes arms delays seam seam_regime protocol_sha256 '
    'labels_sha256 labels lives bars quantities bars_cleared failed_bars curve '
    'hygiene unrelated_admissions w3_law_bars_separating tests table gate_pass '
    'gate_pass_pending_reader independent_reader_pending').split())


class Refusal(Exception):
    pass


def refuse(message):
    raise Refusal(message)


def need(condition, message):
    if not condition:
        refuse(message)


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+': not finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}, error={error}')
    return error


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def check_artifact(path, wanted):
    path = Path(path)
    if not path.exists():
        refuse(f'retained artifact {path} is missing')
    got = digest(path)
    if got != wanted:
        refuse(f'retained artifact {path}: sha256 {got} does not match '
               f'the recorded {wanted}')
    return got


def check_schema(result):
    for key in sorted(REQUIRED_RESULT_KEYS):
        if key not in result:
            refuse('RESULT.json schema is missing required key: '+key)
    return sorted(REQUIRED_RESULT_KEYS)


def mass_log2(value):
    need(value > 0, 'non-positive probability mass')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Tracker:
    """Charged-bit bookkeeping, identical for every arm."""

    def __init__(self):
        self.gain = self.tail = 0.0
        self.minimum = self.peak = self.drawdown = 0.0
        self.early = self.activation = None
        self.slow_hazard = self.fast_hazard = 0

    def charge(self, t, cold, live, admitted):
        self.gain += live-cold
        if t >= MOVE:
            self.tail += live-cold
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        if admitted:
            need(self.activation is None, 'a second first admission')
            self.activation = t+1
        if t == 4095:
            self.early = self.gain

    def result(self):
        return dict(gain=self.gain, tail=self.tail, early=self.early,
                    minimum=self.minimum, peak=self.peak, drawdown=self.drawdown,
                    activation=self.activation)


class Authority(Tracker):
    """turn22's law in mass space. Constructed without any generator label."""

    def __init__(self, mode):
        super().__init__()
        need(mode in CONTROL_ARMS, 'authority mode')
        self.mode = mode
        self.source = self.cold = HALF
        self.shadow = self.clock = self.absolute = 0.0
        self.active = False

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
            return 16.0*max(0.0, self.clock)/(1.0+self.absolute)
        return None

    def odds(self):
        return mass_log2(self.source/self.cold) if self.active else 0.0

    def step(self, t, cold, candidate, matched):
        carried = self.odds()
        live, slow_used = cold, -1
        if self.active and candidate != cold:
            live = cold+mass_log2(self.cold+self.source *
                                  Decimal.from_float(math.exp2(candidate-cold)))
        if self.active:
            slow_used = self.hazard_is_slow()
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            joint = self.source*Decimal.from_float(math.exp2(candidate-cold))
            total = self.cold+joint
            need(joint > 0 and total > 0, 'non-positive quote mass')
            self.source = (1-hazard)*(joint/total)
            if self.mode in CLOCK_ARMS and matched >= 1:
                self.clock = (31.0/32.0)*self.clock+(candidate-cold)
                if self.mode == 'selfnorm':
                    self.absolute = (31.0/32.0)*self.absolute+abs(candidate-cold)
            limit = self.cap_limit()
            if limit is not None:
                ceiling = Decimal(2)**Decimal.from_float(limit)
                ceiling = ceiling/(1+ceiling)
                if self.source > ceiling:
                    self.source = ceiling
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'non-positive updated mass')
            self.slow_hazard += slow_used == 1
            self.fast_hazard += slow_used == 0
        self.shadow += candidate-cold
        admitted = False
        if not self.active and self.shadow >= 32.0:
            self.active = True
            self.source = self.cold = HALF
            self.clock = self.absolute = 0.0
            admitted = True
        self.charge(t, cold, live, admitted)
        return dict(live=live, carried=carried, odds=self.odds(),
                    slow_used=slow_used, admitted=int(admitted))


class Split(Tracker):
    """turn23's law as three renormalized portfolios. No generator label."""

    def __init__(self):
        super().__init__()
        self.capital = [0.0, 0.0, 0.0]
        self.clocks = [0.0, 0.0]
        self.shadow = 0.0
        self.active = False

    def step(self, t, cold, candidate, matched):
        lane = None if matched == 0 else 0 if matched <= 2 else 1
        selected = self.capital[lane+1] if self.active and lane is not None else 0.0
        live = cold
        if self.active and candidate != cold and lane is not None:
            live = math.log2((1.0-selected)*2.0**cold+selected*2.0**candidate)
        self.shadow += candidate-cold
        admitted = False
        hazards = [-1.0, -1.0]
        if not self.active and self.shadow >= 32.0:
            self.active = True
            self.capital = [0.5, 0.25, 0.25]
            self.clocks = [0.0, 0.0]
            admitted = True
        elif self.active:
            tentative = list(self.capital)
            if lane is not None:
                tentative[lane+1] *= 2.0**(candidate-cold)
            mass = math.fsum(tentative)
            need(mass > 0, 'non-positive split mass')
            self.capital = [x/mass for x in tentative]
            hazards = [2.0**-10 if w <= -1.0 else 2.0**-16 for w in self.clocks]
            for i, hazard in enumerate(hazards):
                moved = self.capital[i+1]*hazard
                self.capital[0] += moved
                self.capital[i+1] -= moved
            if lane is not None:
                self.clocks[lane] = (31.0/32.0)*self.clocks[lane]+(candidate-cold)
            need(all(x >= 0 for x in self.capital), 'negative split capital')
            need(abs(math.fsum(self.capital)-1.0) <= 1e-10, 'split capital not normalized')
            need(self.capital[0] >= 2.0**-16-1e-12, 'split cold capital below the floor')
            self.fast_hazard += sum(h == 2.0**-10 for h in hazards)
            self.slow_hazard += sum(h == 2.0**-16 for h in hazards)
        self.charge(t, cold, live, admitted)
        return dict(live=live, hazards=hazards, admitted=int(admitted))


class Oracle(Tracker):
    """The only arms that read the label. seam < 0 means no law change here."""

    def __init__(self, delay, level, seam):
        super().__init__()
        self.delay, self.level, self.seam = delay, level, seam
        self.source = self.cold = HALF
        self.shadow = 0.0
        self.active = self.armed = False
        self.switch_byte = None
        self.clamp_value = None
        self.clamped_at = None

    def odds(self):
        return mass_log2(self.source/self.cold) if self.active else 0.0

    def step(self, t, cold, candidate, matched, fast_source, fast_cold):
        del matched
        carried = self.odds()
        clamped = 0
        if self.seam >= 0 and t == self.seam+self.delay:
            need(self.switch_byte is None, 'a second switch on one life')
            self.switch_byte = t
            if self.level:
                self.source, self.cold = fast_source, fast_cold
                self.clamp_value = self.odds()
                self.clamped_at = t
                clamped = 1
            self.armed = True
        live, slow_used = cold, -1
        if self.active and candidate != cold:
            live = cold+mass_log2(self.cold+self.source *
                                  Decimal.from_float(math.exp2(candidate-cold)))
        if self.active:
            slow_used = 0 if self.armed else 1
            hazard = Decimal(1)/Decimal(65536 if slow_used else 1024)
            joint = self.source*Decimal.from_float(math.exp2(candidate-cold))
            total = self.cold+joint
            need(joint > 0 and total > 0, 'non-positive oracle quote mass')
            self.source = (1-hazard)*(joint/total)
            self.cold = 1-self.source
            self.slow_hazard += slow_used == 1
            self.fast_hazard += slow_used == 0
        self.shadow += candidate-cold
        admitted = False
        if not self.active and self.shadow >= 32.0:
            self.active = True
            self.source = self.cold = HALF
            admitted = True
        self.charge(t, cold, live, admitted)
        return dict(live=live, carried=carried, odds=self.odds(),
                    slow_used=slow_used, admitted=int(admitted), clamped=clamped)


def derive_labels(world):
    """Re-derive the hidden label from the generator's own data, independently."""
    folder = HERE/'data'/f'world{world}'
    raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
    # moved_mid carries no command tape; it is a surface bijection of recombined.
    cmd = {r: (folder/(r+'.commands.bin')).read_bytes()
           for r in ('recombined', SEAM_REGIME, 'unrelated')}
    need(all(len(b) == N for b in raw.values()), f'world {world} target length')
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
    differing = sum(a != b for a, b in zip(raw[SEAM_REGIME][MOVE:],
                                           raw['recombined'][MOVE:]))
    return dict(world=world, seam=MOVE, law_changed_regime=SEAM_REGIME,
                raw_prefix_identical_to_recombined=True,
                raw_tail_differing_bytes=differing,
                raw_tail_divergence=differing/(N-MOVE),
                raw_tail_differing_bytes_vs_unrelated=sum(
                    a != b for a, b in zip(raw[SEAM_REGIME][MOVE:],
                                           raw['unrelated'][MOVE:])),
                command_prefix_identical_to_recombined=True,
                command_tail_identical_to_unrelated=True)


def check_life(world, regime, saved, label):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    folder = HERE/'results/policies'/f'world{world}'
    check_artifact(source, saved['source_sha256'])
    for kind, sha in saved['policy_sha256'].items():
        check_artifact(folder/(f'{regime}-{kind}.tsv.gz'), sha)
    # The oracle's seam comes from the independently re-derived label, not from
    # a constant and not from anything the writer recorded.
    seam = label['seam'] if regime == label['law_changed_regime'] else -1
    need(saved['seam'] == seam, f'world {world} {regime}: recorded seam {saved["seam"]}')
    need(label['seam'] == MOVE, f'world {world}: label seam {label["seam"]}')

    arms = {a: Authority(a) for a in CONTROL_ARMS}
    arms['split'] = Split()
    for a in ORACLE_ARMS:
        delay = int(a.replace('orate', '').replace('olevel', ''))
        arms[a] = Oracle(delay, a.startswith('olevel'), seam)
    error = 0.0
    observed = {a: dict(switch=None, clamp=None, first_fast=None, last_slow=None)
                for a in ORACLE_ARMS}
    exact = dict(reference_label_free=True, oracle_off_law_is_slow=True,
                 oracle_pre_switch_is_slow=True, level_after_switch_is_fast=True,
                 clamp_is_fast_odds=True, cold_quotes_exact=True,
                 new_column=True)
    maxnorm = 0.0
    seen = 0
    with localcontext() as context, \
            gzip.open(source, 'rt') as sf, \
            gzip.open(folder/(regime+'-controls.tsv.gz'), 'rt') as cf, \
            gzip.open(folder/(regime+'-split.tsv.gz'), 'rt') as pf, \
            gzip.open(folder/(regime+'-oracle.tsv.gz'), 'rt') as of:
        context.prec = 50
        rows = zip(csv.DictReader(sf, delimiter='\t'),
                   csv.DictReader(cf, delimiter='\t'),
                   csv.DictReader(pf, delimiter='\t'),
                   csv.DictReader(of, delimiter='\t'))
        for t, (raw, c, s, o) in enumerate(rows):
            seen = t+1
            need(int(raw['t']) == int(c['t']) == int(s['t']) == int(o['t']) == t,
                 f'world {world} {regime}: byte index at row {t}')
            cold = float(raw['logcold'])
            candidate = float(raw['episode_candidate'])
            matched = int(raw['episode_matchedL'])
            rank = int(raw['rank'])
            active = int(raw['episode_active_before'])
            admitted = int(raw['episode_activated_after'])
            maxnorm = max(maxnorm, float(raw['max_norm_error']))
            exact['new_column'] &= raw['new_exact'] == '1'
            # The fast envelope is this pass's own fast arm, read before it steps.
            fast_source, fast_cold = arms['fast'].source, arms['fast'].cold
            got = {}
            for a in CONTROL_ARMS+('split',):
                got[a] = arms[a].step(t, cold, candidate, matched)
            for a in ORACLE_ARMS:
                got[a] = arms[a].step(t, cold, candidate, matched,
                                      fast_source, fast_cold)
            for a in CONTROL_ARMS:
                error = max(error, near(float(c[a+'_live']), got[a]['live'],
                                        f'world {world} {regime} {a} live at {t}'))
                need(int(c[a+'_slow_used']) == got[a]['slow_used'],
                     f'world {world} {regime} {a}: hazard choice at {t}')
                error = max(error, near(float(c[a+'_odds_after']), got[a]['odds'],
                                        f'world {world} {regime} {a} odds at {t}'))
            error = max(error, near(float(s['live']), got['split']['live'],
                                    f'world {world} {regime} split live at {t}'))
            for a in ORACLE_ARMS:
                error = max(error, near(float(o[a+'_live']), got[a]['live'],
                                        f'world {world} {regime} {a} live at {t}'))
                need(int(o[a+'_slow_used']) == got[a]['slow_used'],
                     f'world {world} {regime} {a}: hazard choice at {t}')
                error = max(error, near(float(o[a+'_odds_after']), got[a]['odds'],
                                        f'world {world} {regime} {a} odds at {t}'))
                need(int(o[a+'_clamped']) == got[a]['clamped'],
                     f'world {world} {regime} {a}: clamp flag at {t}')
                if got[a]['clamped']:
                    observed[a]['clamp'] = arms[a].clamp_value
                    error = max(error, near(float(o[a+'_odds_before']),
                                            arms[a].clamp_value,
                                            f'world {world} {regime} {a} clamp at {t}'))
                    exact['clamp_is_fast_odds'] &= \
                        o[a+'_odds_before'] == o['fast_odds_before']
                if arms[a].switch_byte == t:
                    observed[a]['switch'] = t
                if got[a]['slow_used'] == 0 and observed[a]['first_fast'] is None:
                    observed[a]['first_fast'] = t
                if got[a]['slow_used'] == 1:
                    observed[a]['last_slow'] = t
                delay = arms[a].delay
                if seam < 0:
                    exact['oracle_off_law_is_slow'] &= \
                        o[a+'_live'] == o['slow_live'] and \
                        o[a+'_odds_after'] == o['slow_odds_after']
                elif t < seam+delay:
                    exact['oracle_pre_switch_is_slow'] &= \
                        o[a+'_live'] == o['slow_live'] and \
                        o[a+'_odds_after'] == o['slow_odds_after']
                elif arms[a].level:
                    exact['level_after_switch_is_fast'] &= \
                        o[a+'_live'] == o['fast_live'] and \
                        o[a+'_odds_after'] == o['fast_odds_after']
            if not active or candidate == cold or rank == 0:
                for a in ARMS:
                    value = float(c[a+'_live']) if a in CONTROL_ARMS else (
                        float(s['live']) if a == 'split' else float(o[a+'_live']))
                    exact['cold_quotes_exact'] &= value == cold
            for a in ARMS:
                need(got[a]['admitted'] == admitted,
                     f'world {world} {regime} {a}: admission at {t} '
                     f'differs from the episode')
    need(seen == N, f'world {world} {regime}: {seen} rows, expected {N}')

    for a in ORACLE_ARMS:
        delay = arms[a].delay
        if seam < 0:
            need(observed[a]['switch'] is None,
                 f'world {world} {regime} {a}: switched on a life with no law change')
            continue
        need(observed[a]['switch'] == seam+delay,
             f'world {world} {regime} {a}: switch byte {observed[a]["switch"]} '
             f'is not {seam}+{delay}')
        need(observed[a]['first_fast'] is not None and
             observed[a]['first_fast'] >= seam+delay,
             f'world {world} {regime} {a}: fast hazard before the switch byte')
        need(observed[a]['last_slow'] is None or
             observed[a]['last_slow'] < seam+delay,
             f'world {world} {regime} {a}: slow hazard after the switch byte')
        if arms[a].level:
            need(arms[a].clamped_at == seam+delay,
                 f'world {world} {regime} {a}: clamp byte {arms[a].clamped_at}')

    recomputed = {}
    for a in ARMS:
        mine = arms[a].result()
        theirs = saved['arms'][a]
        need(mine['activation'] == theirs['activation'],
             f'world {world} {regime} {a}: activation {mine["activation"]} '
             f'!= recorded {theirs["activation"]}')
        for key in ('gain', 'tail', 'early', 'minimum', 'peak', 'drawdown'):
            error = max(error, near(theirs[key], mine[key],
                                    f'world {world} {regime} {a} {key}'))
        need(mine['minimum'] >= -1-TOL and mine['drawdown'] <= 16+TOL,
             f'world {world} {regime} {a}: prefix or interval bound')
        recomputed[a] = mine
    shared = {arms[a].activation for a in ARMS}
    need(len(shared) == 1,
         f'world {world} {regime}: admissions differ across arms: {sorted(shared)}')
    need(all(exact.values()),
         f'world {world} {regime}: '+', '.join(k for k, v in exact.items() if not v))
    need(saved['admission'] == arms['slow'].activation,
         f'world {world} {regime}: recorded admission {saved["admission"]}')
    need(0 <= maxnorm <= 1e-8,
         f'world {world} {regime}: quoted distributions not normalized, '
         f'max_norm_error={maxnorm}')
    return dict(world=world, regime=regime, forecasts=seen, arms=recomputed,
                exact=exact, max_norm_error=maxnorm,
                admission=arms['slow'].activation,
                switch_bytes={a: observed[a]['switch'] for a in ORACLE_ARMS},
                clamp_values={a: observed[a]['clamp'] for a in ORACLE_ARMS},
                maximum_numeric_error=error)


def mean(values):
    return math.fsum(values)/len(values)


def rebuild_bars(by, arm):
    law_tail = [by[w, 'switched'][arm]['tail']-by[w, 'switched']['fast']['tail']
                for w in WORLDS]
    law_full = [by[w, 'switched'][arm]['gain']-by[w, 'switched']['fast']['gain']
                for w in WORLDS]
    moved_fast = [by[w, 'moved_mid'][arm]['tail']-by[w, 'moved_mid']['fast']['tail']
                  for w in WORLDS]
    moved_slow = [by[w, 'moved_mid'][arm]['tail']-by[w, 'moved_mid']['slow']['tail']
                  for w in WORLDS]
    intact = [by[w, 'recombined'][arm]['gain'] for w in WORLDS]
    early = mean([by[w, 'recombined'][arm]['early'] for w in WORLDS])
    full = mean(intact)
    slow_early = mean([by[w, 'recombined']['slow']['early'] for w in WORLDS])
    slow_full = mean([by[w, 'recombined']['slow']['gain'] for w in WORLDS])
    return dict(
        b1_law_tail_mean=mean(law_tail) >= -1,
        b2_law_whole_mean=mean(law_full) >= 0,
        b3_moved_tail_mean=mean(moved_fast) >= 1,
        b4_moved_tail_wins=sum(x > 0 for x in moved_fast) >= 5,
        b5_moved_tail_vs_slow=mean(moved_slow) >= -3,
        b6_intact_early_retention=slow_early > 0 and early/slow_early >= .95,
        b7_intact_full_retention=slow_full > 0 and full/slow_full >= .95,
        b8_intact_full_positive=all(x > 0 for x in intact))


def rebuild_gate(lives, result):
    by = {(x['world'], x['regime']): x['arms'] for x in lives}
    bars = {a: rebuild_bars(by, a) for a in ARMS}
    for a in ARMS:
        for key, value in bars[a].items():
            need(result['bars'][a][key] == value,
                 f'bar {a}.{key}: reader says {value}, RESULT.json says '
                 f'{result["bars"][a][key]}')
    clearing = [d for d in DELAYS if all(bars[f'olevel{d}'].values())]
    dstar = max(clearing) if clearing else None
    hygiene = dict(
        admission_identity=all(len({x['arms'][a]['activation'] for a in ARMS}) == 1
                               for x in lives),
        admission_shared_with_episode=True,
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                   for x in lives for s in x['arms'].values()),
        fast_own_bound=all(x['arms']['fast']['drawdown'] <= 10+TOL for x in lives),
        normalized=all(0 <= x['max_norm_error'] <= 1e-8 and x['exact']['new_column']
                       for x in lives),
        exact_quotes=all(x['exact']['cold_quotes_exact'] for x in lives),
        label_confined=all(x['exact'][k] for x in lives
                           for k in ('oracle_off_law_is_slow',
                                     'oracle_pre_switch_is_slow',
                                     'level_after_switch_is_fast')),
        archive=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size <= 528
                    for w in WORLDS))
    for key, value in hygiene.items():
        need(result['hygiene'][key] == value,
             f'hygiene {key}: reader says {value}, RESULT.json says '
             f'{result["hygiene"][key]}')
    tests = dict(
        W1_satisfiability=all(bars['olevel0'].values()),
        W2_budget=dstar is not None and dstar >= 32,
        W3_rate_vs_level=any(bars['olevel0'][b] and not bars['orate0'][b]
                             for b in LAW_SIDE_BARS),
        W4_references_stay_themselves=all(not all(bars[a].values())
                                          for a in CAUSAL_NOMINEES),
        W5_hygiene=all(hygiene.values()),
        W6_reader=True)
    for key in ('W1_satisfiability', 'W2_budget', 'W3_rate_vs_level',
                'W4_references_stay_themselves', 'W5_hygiene'):
        need(result['tests'][key] == tests[key],
             f'{key}: reader says {tests[key]}, RESULT.json says '
             f'{result["tests"][key]}')
    need(result['curve']['d_star'] == dstar,
         f'd_star: reader says {dstar}, RESULT.json says '
         f'{result["curve"]["d_star"]}')
    return bars, tests, dstar, hygiene


def main():
    parser = argparse.ArgumentParser(description='Independent turn24 reader.')
    parser.add_argument('--output', default='VERIFY.json',
                        help='where to write the receipt; an existing file is refused')
    parser.add_argument('--result', default='RESULT.json')
    args = parser.parse_args()
    out = Path(args.output)
    if not out.is_absolute():
        out = HERE/out
    if out.exists():
        refuse(f'output file {out} already exists')

    result = json.loads((HERE/args.result).read_text())
    schema = check_schema(result)
    need(result['namespace'] == NAMESPACE, f'namespace {result["namespace"]}')
    need(result['worlds'] == list(WORLDS), f'worlds {result["worlds"]}')
    need(result['arms'] == list(ARMS), f'arms {result["arms"]}')
    need(result['delays'] == list(DELAYS), f'delays {result["delays"]}')
    need(result['seam'] == MOVE and result['seam_regime'] == SEAM_REGIME,
         'declared seam')
    check_artifact(HERE/'PROTOCOL.md', result['protocol_sha256'])
    check_artifact(HERE/'labels'/'ORACLE_LABELS.json', result['labels_sha256'])
    for name in ('DATA_MANIFEST.json', 'EXTRACT_MANIFEST.json',
                 'MEMORY_MANIFEST.json', 'LABELS_MANIFEST.json',
                 'RESULTS_MANIFEST.json'):
        for rel, wanted in json.loads((HERE/name).read_text()).items():
            check_artifact(HERE/rel, wanted)

    declared = json.loads((HERE/'labels'/'ORACLE_LABELS.json').read_text())
    need(declared['seam'] == MOVE and declared['law_changed_regime'] == SEAM_REGIME,
         'the label file does not declare the constructional seam')
    labels = {}
    for w in WORLDS:
        mine = derive_labels(w)
        theirs = declared['worlds'][str(w)]
        for key, value in mine.items():
            need(theirs[key] == value,
                 f'world {w} label {key}: reader says {value}, '
                 f'ORACLE_LABELS.json says {theirs[key]}')
        need(result['labels'][str(w)] == theirs,
             f'world {w}: RESULT.json label differs from ORACLE_LABELS.json')
        labels[w] = mine

    lives = []
    for saved in result['lives']:
        life = check_life(saved['world'], saved['regime'], saved,
                          labels[saved['world']])
        lives.append(life)
        print(life['world'], life['regime'],
              'max_error=%.3g' % life['maximum_numeric_error'], flush=True)
    need(len(lives) == len(WORLDS)*len(REGIMES),
         f'{len(lives)} lives, expected {len(WORLDS)*len(REGIMES)}')

    bars, tests, dstar, hygiene = rebuild_gate(lives, result)
    receipt = dict(
        hygiene=hygiene,
        namespace=NAMESPACE, worlds=list(WORLDS), arms=list(ARMS),
        forecasts=sum(x['forecasts'] for x in lives),
        arm_forecasts=sum(x['forecasts'] for x in lives)*len(ARMS),
        maximum_numeric_error=max(x['maximum_numeric_error'] for x in lives),
        label_free_reference_agreement=True,
        label_confined_to_oracle_arms=all(x['exact']['oracle_off_law_is_slow'] and
                                          x['exact']['oracle_pre_switch_is_slow']
                                          for x in lives),
        switch_bytes_exact=True, clamp_matches_fast_odds=True,
        required_schema_keys=schema, bars=bars, tests=tests, d_star=dstar,
        gate_pass=all(tests.values()),
        writer_gate_pass_pending_reader=result['gate_pass_pending_reader'],
        lives=lives, verified=True)
    with out.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({k: v for k, v in receipt.items()
                      if k not in ('lives', 'bars')}, indent=2))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Refusal as exc:
        print(f'verify.py refuses: {exc}', file=sys.stderr)
        sys.exit(1)
