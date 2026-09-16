#!/usr/bin/env python3
"""One frozen change to the outer source-to-cold hazard; no new local model."""
import argparse
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
spec = importlib.util.spec_from_file_location('frozen_turn5_experiment', REPO/'turn5'/'experiment.py')
t5 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = t5
spec.loader.exec_module(t5)
t3 = t5.t3
WORLDS = tuple(range(56, 64))
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
N = 16384
HAZARDS = {'old': 2.0**-16, 'fast': 2.0**-10}
ARMS = ('revise3', 'null3')
FROZEN = ('turn6/PROTOCOL.md', 'turn6/experiment.py', 'turn6/verify.py',
          'turn5/experiment.py', 'turn5/revision.c', 'turn5/revision',
          'turn3/experiment.py', 'turn3/verify.py', 'turn3/bank.c', 'turn3/bank',
          'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
          'byte_recurrence/trace.c', 'byte_recurrence/collect.c',
          'byte_recurrence/byte_trace', 'byte_recurrence/byte_collect',
          'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h')


def seed(world, component):
    payload = f'netta-outer-fast-v1|{world}|{component}'.encode()
    return int.from_bytes(hashlib.sha256(payload).digest()[:8], 'big')


t5.HERE, t5.WORLDS, t5.seed = HERE, WORLDS, seed
t3.HERE, t3.REPO, t3.WORLDS, t3.seed = HERE, REPO, WORLDS, seed


def check_frozen():
    frozen = json.loads((HERE/'CODE_FREEZE.json').read_text())
    assert frozen['worlds'] == list(WORLDS)
    assert frozen['namespace'] == 'netta-outer-fast-v1'
    for path, wanted in frozen['files'].items():
        assert t3.digest(REPO/path) == wanted, path


t3.check_freeze = check_frozen


def c_replay(world, regime, arm):
    folder = HERE/'results'/'c'/f'world{world:02d}'
    archive = HERE/'memory'/f'world{world:02d}'/'bank.bin'
    command = ([str(REPO/'turn3/bank'), str(archive), 'row'] if arm == 'row2'
               else [str(REPO/'turn5/revision'), str(archive),
                     'static' if arm == 'static3' else 'share'])
    path = folder/f'{regime}-{arm}.tsv.gz'
    t3.run_gzip(command, HERE/'data'/f'world{world:02d}'/f'{regime}.bin', path,
                folder/f'{regime}-{arm}.stderr')
    return path


t5.c_replay = c_replay


class Outer:
    def __init__(self, hazard):
        self.hazard = hazard
        self.shadow = self.odds = self.gain = 0.0
        self.active = False
        self.activation = None
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}
        self.weight_sum_tail = 0.0

    def step(self, t, cold, candidate):
        active = self.active
        odds_before = self.odds if active else 0.0
        weight = t3.weight(odds_before) if active else 0.0
        live = t3.la(cold, odds_before + candidate) - t3.la(0.0, odds_before) if active else cold
        self.gain += live-cold
        self.shadow += candidate-cold
        if t >= 8192:
            self.weight_sum_tail += weight
        if active:
            z = odds_before+candidate-cold
            self.odds = math.log1p(-self.hazard)/math.log(2.0) - t3.la(-z, math.log2(self.hazard))
        elif self.shadow >= 32.0:
            self.active, self.odds, self.activation = True, 0.0, t+1
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        assert self.minimum >= -1-1e-7
        if self.hazard == HAZARDS['fast']:
            assert self.drawdown <= 10+1e-7
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
        return live, weight, active

    def result(self):
        return dict(gain=self.gain, tail=self.gain-self.horizons['8192'],
                    activation=self.activation, minimum=self.minimum,
                    drawdown=self.drawdown, horizons=self.horizons,
                    mean_tail_source_weight=self.weight_sum_tail/8192,
                    shadow=self.shadow)


def freeze():
    t3.save(HERE/'CODE_FREEZE.json', dict(created_utc=time.time(), worlds=WORLDS,
            namespace='netta-outer-fast-v1', files={p:t3.digest(REPO/p) for p in FROZEN}))


def evaluate():
    check_frozen()
    for kind in ('data', 'traces', 'memory'):
        t3.check_manifest(HERE/kind/'MANIFEST.json')
    if (HERE/'results').exists():
        raise RuntimeError('results exist; no overwrite')
    base = []
    for world in WORLDS:
        base.extend(t5.evaluate_world(world))
    t3.save(HERE/'BASE_RESULT.json', dict(lives=base,
            scope='Frozen turn5 candidate and old outer on fresh worlds'))
    lives = []
    for world in WORLDS:
        for regime in REGIMES:
            states = {(arm, mode): Outer(h) for arm in ARMS for mode,h in HAZARDS.items()}
            source = HERE/'results'/'events'/f'world{world:02d}'/f'{regime}.tsv.gz'
            dest = HERE/'results'/'outer'/f'world{world:02d}'/f'{regime}.tsv.gz'
            dest.parent.mkdir(parents=True, exist_ok=True)
            with gzip.open(source, 'rt') as incoming, gzip.open(dest, 'xt', newline='') as outgoing:
                reader = csv.DictReader(incoming, delimiter='\t')
                writer = csv.writer(outgoing, delimiter='\t')
                writer.writerow(('t','arm','mode','logcold','logcandidate','loglive',
                                 'source_weight_before','active_before','activation','gain_after'))
                for record in reader:
                    arm = record['arm']
                    if arm not in ARMS:
                        continue
                    t = int(record['t'])
                    cold, candidate = float(record['logcold']), float(record['logbank'])
                    for mode in HAZARDS:
                        state = states[arm, mode]
                        live, weight, active = state.step(t, cold, candidate)
                        if mode == 'old':
                            assert abs(live-float(record['loglive'])) < 1e-7
                            assert int(active) == int(record['active_before'])
                            assert abs(state.gain-float(record['gain_after'])) < 1e-7
                        writer.writerow((t,arm,mode,cold,candidate,live,weight,
                                         int(active),state.activation or '',state.gain))
            for arm in ARMS:
                assert states[arm,'old'].activation == states[arm,'fast'].activation
                assert abs(states[arm,'old'].shadow-states[arm,'fast'].shadow) < 1e-9
            lives.append(dict(world=world, regime=regime,
                              arms={arm:{mode:states[arm,mode].result() for mode in HAZARDS}
                                    for arm in ARMS}, source_sha256=t3.digest(source),
                              outer_sha256=t3.digest(dest)))
    mosaic = [x for x in lives if x['regime']=='mosaic']
    changed = [x for x in lives if x['regime']=='mosaic_then_unrelated']
    mean = lambda xs: math.fsum(xs)/len(xs)
    gain = lambda rows,arm,mode,key: [x['arms'][arm][mode][key] for x in rows]
    fast = mean(gain(mosaic,'revise3','fast','gain'))
    old = mean(gain(mosaic,'revise3','old','gain'))
    null = mean(gain(mosaic,'null3','fast','gain'))
    paired = [a-b for a,b in zip(gain(changed,'revise3','fast','tail'),
                                gain(changed,'revise3','old','tail'))]
    summary = dict(mosaic_fast_mean=fast, mosaic_old_mean=old,
                   mosaic_null_fast_mean=null, mosaic_gain_bpb=fast/N,
                   null_contrast_bpb=(fast-null)/N, retention=fast/old,
                   mosaic_positive=sum(x>0 for x in gain(mosaic,'revise3','fast','gain')),
                   changed_paired=paired, changed_mean_improvement=mean(paired),
                   changed_improved=sum(x>0 for x in paired),
                   changed_worst_improvement=min(gain(changed,'revise3','fast','tail'))-
                                             min(gain(changed,'revise3','old','tail')))
    tests = dict(material_gain=summary['mosaic_gain_bpb']>=.0075,
                 null_contrast=summary['null_contrast_bpb']>=.0075,
                 positive_worlds=summary['mosaic_positive']==8,
                 retention=summary['retention']>=.70,
                 tail_mean=summary['changed_mean_improvement']>1,
                 tail_count=summary['changed_improved']>=5,
                 tail_worst=summary['changed_worst_improvement']>1)
    t3.save(HERE/'RESULT.json', dict(worlds=WORLDS, namespace='netta-outer-fast-v1',
            protocol_sha256=t3.digest(HERE/'PROTOCOL.md'), lives=lives,
            summary=summary, tests=tests, gate_pass=all(tests.values()),
            independent_reader_pending=True))
    t3.save(HERE/'ARTIFACT_MANIFESTS.json',
            {kind:t3.manifest(HERE/kind) for kind in ('data','traces','memory','results')})
    print(json.dumps(dict(summary=summary,tests=tests,gate_pass=all(tests.values())),indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
    elif stage == 'generate':
        t3.generate()
    elif stage == 'extract':
        t3.extract()
    else:
        evaluate()
