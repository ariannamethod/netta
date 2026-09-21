#!/usr/bin/env python3
"""Frozen local-cold experiment. Run stages in order; never overwrite evidence."""

import argparse
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(REPO / 'turn3'))
import experiment as t3  # noqa: E402: frozen generator, collector, and trace writer

WORLDS = tuple(range(40, 48))
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
ARMS = ('row3', 'global3')
N = 16384
HAZARD = 2.0 ** -16
LOG_PRIOR = (math.log2(1/8), math.log2(7/16), math.log2(7/16))
FROZEN_NAMES = ('turn4/PROTOCOL.md', 'turn4/experiment.py', 'turn4/verify.py',
                'turn3/experiment.py', 'turn3/verify.py', 'turn3/bank.c',
                'turn3/bank', 'turn3/PROTOCOL.md',
                'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
                'byte_recurrence/trace.c', 'byte_recurrence/collect.c',
                'byte_recurrence/byte_trace', 'byte_recurrence/byte_collect',
                'portable_recurrence/recurrence.c',
                'portable_recurrence/recurrence.h')


def seed(world, component):
    raw = f'netta-cold-v1|{world}|{component}'.encode()
    return int.from_bytes(hashlib.sha256(raw).digest()[:8], 'big')


# The previous generator and collector are reused unchanged, with fresh world
# IDs and seed namespace. Their source hashes are pinned in CODE_FREEZE.json.
t3.HERE = HERE
t3.REPO = REPO
t3.WORLDS = WORLDS
t3.seed = seed


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')


def check_frozen():
    name = ('CODE_FREEZE_VERIFY.json' if (HERE/'CODE_FREEZE_VERIFY.json').exists()
            else 'CODE_FREEZE_REPAIR.json' if (HERE/'CODE_FREEZE_REPAIR.json').exists()
            else 'CODE_FREEZE.json')
    frozen = json.loads((HERE / name).read_text())
    if name == 'CODE_FREEZE_REPAIR.json':
        assert frozen['repaired_from_sha256'] == t3.digest(HERE/'CODE_FREEZE.json')
    if name == 'CODE_FREEZE_VERIFY.json':
        assert frozen['repaired_from_sha256'] == t3.digest(HERE/'CODE_FREEZE_REPAIR.json')
    assert frozen['protocol_sha256'] == t3.digest(HERE / 'PROTOCOL.md')
    for name, expected in frozen['files'].items():
        assert t3.digest(REPO / name) == expected, name


t3.check_freeze = check_frozen


def freeze():
    save(HERE / 'CODE_FREEZE.json', {
        'created_utc': time.time(), 'worlds': WORLDS,
        'namespace': 'netta-cold-v1',
        'protocol_sha256': t3.digest(HERE / 'PROTOCOL.md'),
        'files': {name: t3.digest(REPO / name) for name in FROZEN_NAMES},
    })


def repair_freeze():
    original = HERE/'CODE_FREEZE.json'
    assert original.exists() and not (HERE/'CODE_FREEZE_REPAIR.json').exists()
    save(HERE/'CODE_FREEZE_REPAIR.json', {
        'created_utc': time.time(), 'worlds': WORLDS,
        'namespace': 'netta-cold-v1',
        'repaired_from_sha256': t3.digest(original),
        'protocol_sha256': t3.digest(HERE/'PROTOCOL.md'),
        'files': {name: t3.digest(REPO/name) for name in FROZEN_NAMES},
    })


def verify_freeze():
    previous = HERE/'CODE_FREEZE_REPAIR.json'
    assert previous.exists() and not (HERE/'CODE_FREEZE_VERIFY.json').exists()
    save(HERE/'CODE_FREEZE_VERIFY.json', {
        'created_utc': time.time(), 'worlds': WORLDS,
        'namespace': 'netta-cold-v1',
        'repaired_from_sha256': t3.digest(previous),
        'protocol_sha256': t3.digest(HERE/'PROTOCOL.md'),
        'files': {name: t3.digest(REPO/name) for name in FROZEN_NAMES},
    })


def logadd2(*values):
    top = max(values)
    if top == -math.inf:
        return top
    return top + math.log2(math.fsum(2.0 ** (v-top) for v in values))


def posterior(pair):
    scores = (LOG_PRIOR[0], LOG_PRIOR[1] + pair[0], LOG_PRIOR[2] + pair[1])
    denominator = logadd2(*scores)
    logweights = tuple(score-denominator for score in scores)
    weights = tuple(2.0 ** value for value in logweights)
    # An implausible arm may round to zero in the diagnostic probability,
    # while its finite log-weight remains available for the actual quote.
    assert all(math.isfinite(x) and x >= 0 for x in weights)
    assert abs(math.fsum(weights)-1.0) < 1e-14
    return weights, logweights


class Outer:
    def __init__(self):
        self.shadow = self.odds = self.gain = 0.0
        self.active = False
        self.activation = None
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}

    def step(self, t, logcold, logbank):
        active_before, shadow_before, odds_before = self.active, self.shadow, self.odds
        loglive = (logadd2(logcold, self.odds + logbank) - logadd2(0.0, self.odds)
                   if self.active else logcold)
        self.gain += loglive-logcold
        delta = logbank-logcold
        self.shadow += delta
        activated = False
        if active_before:
            z = odds_before+delta
            self.odds = math.log2(1-HAZARD)-logadd2(-z, math.log2(HAZARD))
        elif self.shadow >= 32.0:
            self.active, self.odds, self.activation = True, 0.0, t+1
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        assert self.minimum >= -1-1e-7 and self.drawdown <= 16+1e-7
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
        return loglive, active_before, activated, shadow_before, odds_before

    def result(self):
        return {'gain': self.gain, 'at8192': self.horizons['8192'],
                'tail': self.gain-self.horizons['8192'],
                'activation': self.activation, 'horizons': self.horizons,
                'minimum': self.minimum, 'drawdown': self.drawdown,
                'shadow': self.shadow}


FIELDS = ('t', 'pattern', 'truth', 'group', 'arm', 'logcold', 'logA', 'logB',
          'weight_cold', 'weight_A', 'weight_B', 'capital_A_before',
          'capital_B_before', 'logbank', 'loglive', 'shadow_before',
          'odds_before', 'active_before', 'activated_after', 'gain_after')


def c_replay(world, regime):
    folder = HERE / 'results' / 'c' / f'world{world:02d}'
    t3.run_gzip([str(REPO/'turn3/bank'),
                 str(HERE/'memory'/f'world{world:02d}'/'bank.bin'), 'row'],
                HERE/'data'/f'world{world:02d}'/f'{regime}.bin',
                folder/f'{regime}-row.tsv.gz', folder/f'{regime}-row.stderr')


def evaluate_one(world, regime):
    cpath = HERE/'results'/'c'/f'world{world:02d}'/f'{regime}-row.tsv.gz'
    raw = (HERE/'data'/f'world{world:02d}'/f'{regime}.bin').read_bytes()
    assert len(raw) == N
    states = {name: Outer() for name in ARMS}
    capitals = {'row3': {}, 'global3': {'*': [0.0, 0.0]}}
    maxima = {'component_price_error': 0.0, 'base_bound_error': 0.0}
    bad_examples = []
    output = HERE/'results'/'events'/f'world{world:02d}'/f'{regime}.tsv.gz'
    output.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(cpath, 'rt', newline='') as source, gzip.open(output, 'xt', newline='') as target:
        reader = csv.DictReader(source, delimiter='\t')
        writer = csv.writer(target, delimiter='\t')
        writer.writerow(FIELDS)
        for t, event in enumerate(reader):
            assert t < N and int(event['t']) == t and int(event['truth']) == raw[t]
            pattern, group = event['pattern'], int(event['group'])
            cold, a, b = (float(event[key]) for key in ('logcold', 'logA', 'logB'))
            assert all(math.isfinite(x) for x in (cold, a, b))
            if pattern == '-':
                assert group == -1 and a == b == cold
            else:
                assert pattern in t3.K and 0 <= group <= t3.K[pattern]
                if group == t3.K[pattern]:
                    assert abs(a-cold) < 1e-10 and abs(b-cold) < 1e-10
            if t == 8191 and regime == 'mosaic_then_unrelated':
                assert raw[:8192] == (HERE/'data'/f'world{world:02d}'/'mosaic.bin').read_bytes()[:8192]
            for arm in ARMS:
                key = pattern if arm == 'row3' else '*'
                pair = capitals[arm].setdefault(key, [0.0, 0.0])
                before = pair[:]
                weights, logweights = posterior(pair)
                logbank = logadd2(logweights[0]+cold,
                                  logweights[1]+a,
                                  logweights[2]+b)
                state = states[arm]
                loglive, active, activated, shadow, odds = state.step(t, cold, logbank)
                assert math.isfinite(logbank) and math.isfinite(loglive)
                writer.writerow((t, pattern, raw[t], group, arm, cold, a, b,
                                 *weights, *before, logbank, loglive, shadow,
                                 odds, int(active), int(activated), state.gain))
                if pattern != '-':
                    pair[0] += a-cold
                    pair[1] += b-cold
                    assert all(math.isfinite(v) for v in pair)
                if arm == 'row3' and t >= 8192 and a < cold and b < cold and len(bad_examples) < 3:
                    bad_examples.append({'t': t, 'pattern': pattern,
                                         'logcold': cold, 'logA': a, 'logB': b,
                                         'weight_cold_before': weights[0],
                                         'bank2_logprice': float(event['logbank']),
                                         'bank3_logprice': logbank})
            maxima['base_bound_error'] = max(maxima['base_bound_error'],
                                              max(0.0, -1-float(event['gain_after'])))
        assert t+1 == N
    row2 = {'gain': float(event['gain_after'])}
    with gzip.open(cpath, 'rt', newline='') as source:
        reader = csv.DictReader(source, delimiter='\t')
        for t, event in enumerate(reader):
            if t == 8191:
                row2['at8192'] = float(event['gain_after'])
        assert t+1 == N
    row2['tail'] = row2['gain']-row2['at8192']
    return {'world': world, 'regime': regime,
            'row2': row2, 'row3': states['row3'].result(),
            'global3': states['global3'].result(),
            'bad_examples': bad_examples, 'checks': maxima,
            'c_trace_sha256': t3.digest(cpath),
            'event_sha256': t3.digest(output)}


def evaluate():
    check_frozen()
    t3.check_manifest(HERE/'data/MANIFEST.json')
    t3.check_manifest(HERE/'traces/MANIFEST.json')
    t3.check_manifest(HERE/'memory/MANIFEST.json')
    if (HERE/'results').exists():
        raise RuntimeError('results already exist; do not overwrite evidence')
    for world in WORLDS:
        for regime in REGIMES:
            c_replay(world, regime)
    lives = [evaluate_one(world, regime) for world in WORLDS for regime in REGIMES]
    mosaic = [x for x in lives if x['regime'] == 'mosaic']
    switched = [x for x in lives if x['regime'] == 'mosaic_then_unrelated']
    mean = lambda values: math.fsum(values)/len(values)
    gain3 = mean([x['row3']['gain'] for x in mosaic])
    gain2 = mean([x['row2']['gain'] for x in mosaic])
    tail_differences = [x['row3']['tail']-x['row2']['tail'] for x in switched]
    summary = {
        'mosaic_mean_row3': gain3, 'mosaic_mean_row2': gain2,
        'mosaic_gain_bpb': gain3/N, 'retained_fraction': gain3/gain2,
        'mosaic_positive_worlds': sum(x['row3']['gain'] > 0 for x in mosaic),
        'changed_tail_mean_improvement': mean(tail_differences),
        'changed_tail_improved_worlds': sum(v > 0 for v in tail_differences),
        'changed_tail_worst_improvement': min(x['row3']['tail'] for x in switched)
                                         - min(x['row2']['tail'] for x in switched),
        'changed_tail_paired_differences': tail_differences,
    }
    save(HERE/'RESULT.json', {'worlds': WORLDS, 'namespace': 'netta-cold-v1',
                              'summary': summary, 'lives': lives,
                              'independent_gate_pending': True})
    save(HERE/'ARTIFACT_MANIFESTS.json', {
        name: t3.manifest(HERE/name) for name in ('data', 'traces', 'memory', 'results')})
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze', 'repair_freeze', 'verify_freeze',
                                          'generate', 'extract', 'evaluate'))
    args = parser.parse_args()
    if args.stage == 'freeze':
        freeze()
    elif args.stage == 'repair_freeze':
        repair_freeze()
    elif args.stage == 'verify_freeze':
        verify_freeze()
    elif args.stage == 'generate':
        t3.generate()
    elif args.stage == 'extract':
        t3.extract()
    else:
        evaluate()
