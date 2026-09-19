#!/usr/bin/env python3
"""Compose turn13 joint memory with turn6's already fixed withdrawal law."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(144, 152))
REGIMES = ('recombined', 'unrelated', 'switched')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
N = 16384
NAMESPACE = 'netta-joint-fast-withdraw-v1'
HAZARDS = {'slow': 2.0**-16, 'fast': 2.0**-10}

spec = importlib.util.spec_from_file_location('frozen_turn13_experiment', REPO/'turn13'/'experiment.py')
t13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t13
spec.loader.exec_module(t13)
t13.HERE = HERE
t13.REPO = REPO
t13.WORLDS = WORLDS
t13.NAMESPACE = NAMESPACE

FROZEN = (
    'turn14/PROTOCOL.md', 'turn14/experiment.py', 'turn14/verify.py',
    'turn14/Makefile', 'turn14/episode', 'turn13/episode.c',
    'turn13/verify.py', 'turn13/experiment.py',
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
        json.dump(value, stream, ensure_ascii=False, indent=2,
                  sort_keys=True, allow_nan=False)
        stream.write('\n')


def logadd(a, b):
    high, low = (a, b) if a >= b else (b, a)
    return high + math.log1p(math.exp2(low-high))/math.log(2.0)


class Outer:
    """Prospective two-state authority; candidate and admission are external."""
    def __init__(self, hazard):
        self.hazard = hazard
        self.shadow = self.odds = self.gain = 0.0
        self.active = False
        self.activation = None
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}

    def step(self, t, cold, candidate):
        before_active = self.active
        before_shadow = self.shadow
        before_odds = self.odds if before_active else 0.0
        live = (logadd(cold, before_odds+candidate)-logadd(0.0, before_odds)
                if before_active else cold)
        self.gain += live-cold
        self.shadow += candidate-cold
        activated = False
        if before_active:
            posterior = before_odds+candidate-cold
            self.odds = (math.log1p(-self.hazard)/math.log(2.0)
                         - logadd(-posterior, math.log2(self.hazard)))
        elif self.shadow >= 32.0:
            self.active = True
            self.activation = t+1
            self.odds = 0.0
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
        return dict(candidate=candidate, live=live, shadow_before=before_shadow,
                    odds_before=before_odds, active_before=int(before_active),
                    activated_after=int(activated), gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, candidate_gain=self.shadow,
                    tail=self.gain-self.horizons['8192'],
                    horizons=self.horizons, minimum=self.minimum,
                    peak=self.peak, max_drawdown=self.drawdown,
                    activation=self.activation)


def freeze():
    save(HERE/'FREEZE.json', dict(created_utc=time.time(), namespace=NAMESPACE,
         worlds=WORLDS, files={name:digest(REPO/name) for name in FROZEN}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    for name, wanted in frozen['files'].items():
        assert digest(REPO/name) == wanted, name


def replay_fast(world, regime, slow):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    target = HERE/'results'/'outer'/f'world{world}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    states = {(arm, mode): Outer(hazard)
              for arm in ARMS for mode, hazard in HAZARDS.items()}
    examples = {'help': None, 'harm': None}
    with gzip.open(source, 'rt') as incoming, gzip.open(target, 'xt', newline='') as outgoing:
        rows = csv.DictReader(incoming, delimiter='\t')
        writer = csv.writer(outgoing, delimiter='\t')
        writer.writerow(('t', 'arm', 'logcold', 'logcandidate', 'fast_live',
                         'fast_shadow_before', 'fast_odds_before',
                         'fast_active_before', 'fast_activated_after', 'fast_gain_after'))
        for t, row in enumerate(rows):
            assert int(row['t']) == t
            cold = float(row['logcold'])
            for arm in ARMS:
                candidate = float(row[arm+'_candidate'])
                slow_wanted = states[arm, 'slow'].step(t, cold, candidate)
                for field, wanted in slow_wanted.items():
                    actual = row[arm+'_'+field]
                    if field in ('active_before', 'activated_after'):
                        assert int(actual) == wanted
                    else:
                        assert abs(float(actual)-wanted) <= 1e-7
                fast_wanted = states[arm, 'fast'].step(t, cold, candidate)
                writer.writerow((t, arm, cold, candidate, fast_wanted['live'],
                    fast_wanted['shadow_before'], fast_wanted['odds_before'],
                    fast_wanted['active_before'], fast_wanted['activated_after'],
                    fast_wanted['gain_after']))
                assert states[arm, 'slow'].activation == states[arm, 'fast'].activation
                assert states[arm, 'slow'].shadow == states[arm, 'fast'].shadow
                if arm == 'episode' and regime == 'switched' and t >= 8192:
                    delta = fast_wanted['live']-slow_wanted['live']
                    sample = dict(world=world, regime=regime, t=t,
                                  truth=int(row['truth']), cold=cold,
                                  candidate=candidate, slow_live=slow_wanted['live'],
                                  fast_live=fast_wanted['live'], difference=delta,
                                  matched_length=int(row['episode_matchedL']))
                    if examples['help'] is None or delta > examples['help']['difference']:
                        examples['help'] = sample
                    if examples['harm'] is None or delta < examples['harm']['difference']:
                        examples['harm'] = sample
    assert t+1 == N
    slow_arms = {arm: states[arm, 'slow'].result() for arm in ARMS}
    fast_arms = {arm: states[arm, 'fast'].result() for arm in ARMS}
    for arm in ARMS:
        assert slow_arms[arm]['activation'] == slow['arms'][arm]['activation']
        for field in ('gain', 'tail', 'candidate_gain', 'minimum', 'peak', 'max_drawdown'):
            assert abs(slow_arms[arm][field]-slow['arms'][arm][field]) <= 1e-7
        for horizon, value in slow_arms[arm]['horizons'].items():
            assert abs(value-slow['arms'][arm]['horizons'][horizon]) <= 1e-7
        assert slow_arms[arm]['minimum'] >= -1-1e-7
        assert fast_arms[arm]['minimum'] >= -1-1e-7
        assert slow_arms[arm]['max_drawdown'] <= 16+1e-7
        assert fast_arms[arm]['max_drawdown'] <= 10+1e-7
    return dict(world=world, regime=regime, slow=slow_arms, fast=fast_arms,
                source_trace_sha256=digest(source), outer_trace_sha256=digest(target)), examples


def summarize(lives):
    by = {(x['world'], x['regime']):x for x in lives}
    mean = lambda values: math.fsum(values)/len(values)
    mode_summary = {}
    mode_tests = {}
    for mode in HAZARDS:
        early = [by[w, 'recombined'][mode]['episode']['horizons']['4096'] for w in WORLDS]
        full = [by[w, 'recombined'][mode]['episode']['gain'] for w in WORLDS]
        diffs = {arm:[by[w, 'recombined'][mode]['episode']['horizons']['4096']-
                           by[w, 'recombined'][mode][arm]['horizons']['4096'] for w in WORLDS]
                 for arm in ('frequency', 'row', 'reverse', 'permuted', 'flat')}
        switched = [by[w, 'switched'][mode]['episode']['gain']-
                    by[w, 'switched'][mode]['row']['gain'] for w in WORLDS]
        tails = [by[w, 'switched'][mode]['episode']['tail']-
                 by[w, 'switched'][mode]['row']['tail'] for w in WORLDS]
        mode_summary[mode] = dict(early=early, full=full,
            early_gain_bpb=mean(early)/4096, full_gain_bpb=mean(full)/N,
            early_differences=diffs, switched_differences=switched,
            tail_differences=tails,
            means={regime:{arm:{field:mean([by[w,regime][mode][arm][field] for w in WORLDS])
                                      for field in ('gain','tail','candidate_gain')}
                           for arm in ARMS} for regime in REGIMES})
        tests = dict(early_gain=mean(early)/4096 >= .005,
                     early_positive=sum(x > 0 for x in early) >= 6,
                     full_gain=mean(full)/N >= .005,
                     full_positive=all(x > 0 for x in full),
                     switched_mean=mean(switched) >= 0,
                     switched_count=sum(x > 0 for x in switched) >= 5,
                     tail_tolerance=mean(tails) >= -1)
        for arm, values in diffs.items():
            tests[arm+'_early_mean'] = mean(values) > 1
            tests[arm+'_early_count'] = sum(x > 0 for x in values) >= 5
        assert len(tests) == 17
        mode_tests[mode] = tests

    slow_early = mean(mode_summary['slow']['early'])
    fast_early = mean(mode_summary['fast']['early'])
    slow_full = mean(mode_summary['slow']['full'])
    fast_full = mean(mode_summary['fast']['full'])
    slow_switched = mode_summary['slow']['means']['switched']['episode']['gain']
    fast_switched = mode_summary['fast']['means']['switched']['episode']['gain']
    tail_changes = [by[w,'switched']['fast']['episode']['tail']-
                    by[w,'switched']['slow']['episode']['tail'] for w in WORLDS]
    paired = dict(early_retention=fast_early/slow_early,
                  full_retention=fast_full/slow_full,
                  switched_retention=fast_switched/slow_switched,
                  changed_tail_improvements=tail_changes,
                  changed_tail_mean=mean(tail_changes),
                  changed_tail_improved=sum(x > 0 for x in tail_changes),
                  changed_tail_worst_lift=(min(by[w,'switched']['fast']['episode']['tail'] for w in WORLDS)-
                                           min(by[w,'switched']['slow']['episode']['tail'] for w in WORLDS)))
    tests = {'fast_'+key:value for key,value in mode_tests['fast'].items()}
    tests.update(retain_early=slow_early > 0 and paired['early_retention'] >= .90,
                 retain_full=slow_full > 0 and paired['full_retention'] >= .90,
                 retain_switched=slow_switched > 0 and paired['switched_retention'] >= .90,
                 tail_mean=paired['changed_tail_mean'] > 1,
                 tail_count=paired['changed_tail_improved'] >= 5,
                 tail_worst=paired['changed_tail_worst_lift'] > 1)
    return dict(modes=mode_summary, paired=paired), tests


def evaluate():
    check_freeze()
    for name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json'):
        for path, wanted in json.loads((HERE/name).read_text()).items():
            assert digest(HERE/path) == wanted, path
    if (HERE/'results').exists():
        raise RuntimeError('results already exist; refusing to replace evidence')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(t13.run_target, w, regime)
                   for w in WORLDS for regime in REGIMES]
        slow = [future.result() for future in futures]
    slow_by = {(x['world'],x['regime']):x for x in slow}
    lives, samples = [], []
    for w in WORLDS:
        for regime in REGIMES:
            life, example = replay_fast(w, regime, slow_by[w,regime])
            lives.append(life)
            if regime == 'switched':
                samples.append(example)
    summary, tests = summarize(lives)
    identities = dict(candidate_shared=True, shadow_shared=True,
                      admission_shared=True, slow_bound=True,
                      fast_bound=True)
    result = dict(worlds=WORLDS, namespace=NAMESPACE,
        protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
        summary=summary, tests=tests, identities=identities,
        raw_extrema=dict(help=max((x['help'] for x in samples), key=lambda x:x['difference']),
                         harm=min((x['harm'] for x in samples), key=lambda x:x['difference'])),
        gate_pass=all(tests.values()) and all(identities.values()),
        independent_reader_pending=True)
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', t13.manifest(HERE/'results'))
    print(json.dumps(dict(summary=summary['paired'], tests=tests,
                          gate_pass=result['gate_pass']), indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    if stage in ('generate','extract'):
        function = t13.generate_world if stage == 'generate' else t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(function, WORLDS)))
    elif stage == 'learn':
        for world in WORLDS:
            print('learned', t13.learn_world(world), flush=True)
    else:
        evaluate()
    if stage == 'extract':
        save(HERE/'DATA_MANIFEST.json', t13.manifest(HERE/'data'))
    if stage == 'learn':
        save(HERE/'MEMORY_MANIFEST.json', t13.manifest(HERE/'memory'))


if __name__ == '__main__':
    main()
