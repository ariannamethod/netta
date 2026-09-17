#!/usr/bin/env python3
"""Independent probability-mass replay of the two outer laws on C quotes."""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = range(56, 64)
REGIMES = ('mosaic','unrelated','mosaic_then_unrelated')
ARMS = ('revise3','null3')
MODES = ('old','fast')
N = 16384
TOL = 1e-7


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def near(actual, wanted, label, tolerance=TOL):
    error = abs(float(actual)-float(wanted))
    need(error <= tolerance, f'{label}: {error} > {tolerance}')
    return error


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1<<20), b''):
            digest.update(block)
    return digest.hexdigest()


class MassOuter:
    def __init__(self, mode):
        self.h = Decimal(1)/(Decimal(65536) if mode=='old' else Decimal(1024))
        self.mode = mode
        self.cold = self.source = Decimal('0.5')
        self.active = False
        self.activation = None
        self.shadow = self.gain = self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}
        self.tail_weight = 0.0

    def step(self, t, cold_log, candidate_log):
        active_before = self.active
        weight = float(self.source) if active_before else 0.0
        if t>=8192:
            self.tail_weight += weight
        delta = candidate_log-cold_log
        increment = 0.0
        if active_before:
            ratio = Decimal.from_float(math.exp2(delta))
            joint_source = self.source*ratio
            total = self.cold+joint_source
            increment = math.log2(float(total))
            posterior_source = joint_source/total
            posterior_cold = self.cold/total
            self.cold = posterior_cold+self.h*posterior_source
            self.source = (1-self.h)*posterior_source
            need(self.cold>0 and self.source>0, 'positive state masses')
            near(self.cold+self.source,1,'mass normalization',1e-12)
        self.gain += increment
        self.shadow += delta
        if not self.active and self.shadow>=32:
            self.active = True
            self.activation = t+1
            self.cold = self.source = Decimal('0.5')
        self.minimum = min(self.minimum,self.gain)
        self.peak = max(self.peak,self.gain)
        self.drawdown = max(self.drawdown,self.peak-self.gain)
        need(self.minimum>=-1-TOL,'prefix loss')
        if self.mode=='fast':
            need(self.drawdown<=10+TOL,'fast interval loss')
        if t+1 in (1024,4096,8192,N):
            self.horizons[str(t+1)] = self.gain
        return cold_log+increment, weight, active_before

    def result(self):
        return dict(gain=self.gain, tail=self.gain-self.horizons['8192'],
                    activation=self.activation, minimum=self.minimum,
                    drawdown=self.drawdown, horizons=self.horizons,
                    mean_tail_source_weight=self.tail_weight/8192,
                    shadow=self.shadow)


def identity():
    frozen = json.loads((HERE/'CODE_FREEZE.json').read_text())
    need(frozen['worlds']==list(WORLDS) and frozen['namespace']=='netta-outer-fast-v1',
         'frozen batch')
    for name,wanted in frozen['files'].items():
        need(sha(REPO/name)==wanted,'frozen file '+name)
    manifests = json.loads((HERE/'ARTIFACT_MANIFESTS.json').read_text())
    for section, items in manifests.items():
        for name,wanted in items.items():
            need(sha(HERE/name)==wanted,'artifact '+name)
    return dict(protocol_sha256=sha(HERE/'PROTOCOL.md'),
                freeze_sha256=sha(HERE/'CODE_FREEZE.json'),
                result_sha256=sha(HERE/'RESULT.json'),
                artifact_manifest_sha256=sha(HERE/'ARTIFACT_MANIFESTS.json'))


def compare_tree(actual, expected, label):
    if isinstance(expected,dict):
        need(isinstance(actual,dict),label+' object')
        for key,value in expected.items():
            need(key in actual,label+' missing '+key)
            compare_tree(actual[key],value,label+'/'+key)
    elif isinstance(expected,list):
        need(isinstance(actual,list) and len(actual)==len(expected),label+' list')
        for i,(a,e) in enumerate(zip(actual,expected)):
            compare_tree(a,e,label+f'/{i}')
    elif isinstance(expected,bool) or expected is None or isinstance(expected,str) or isinstance(expected,int):
        need(actual==expected,label+' exact')
    else:
        near(actual,expected,label)


def verify_life(world,regime):
    source = HERE/'results'/'events'/f'world{world:02d}'/f'{regime}.tsv.gz'
    outer_path = HERE/'results'/'outer'/f'world{world:02d}'/f'{regime}.tsv.gz'
    c_path = HERE/'results'/'c'/f'world{world:02d}'/f'{regime}-revise3.tsv.gz'
    states = {(arm,mode):MassOuter(mode) for arm in ARMS for mode in MODES}
    maximum_error = 0.0
    forecast_count = 0
    raw = (HERE/'data'/f'world{world:02d}'/f'{regime}.bin').read_bytes()
    need(len(raw)==N,'raw length')
    if regime=='mosaic_then_unrelated':
        mosaic = (HERE/'data'/f'world{world:02d}'/'mosaic.bin').read_bytes()
        need(raw[:8192]==mosaic[:8192],'common prefix')
    with gzip.open(source,'rt') as incoming, gzip.open(outer_path,'rt') as recorded, gzip.open(c_path,'rt') as c_stream:
        base = csv.DictReader(incoming,delimiter='\t')
        outer = iter(csv.DictReader(recorded,delimiter='\t'))
        c = iter(csv.DictReader(c_stream,delimiter='\t'))
        for event in base:
            arm = event['arm']
            if arm not in ARMS:
                continue
            t = int(event['t'])
            need(0<=t<N and int(event['truth'])==raw[t], 'causal event identity')
            cold, candidate = float(event['logcold']),float(event['logbank'])
            need(math.isfinite(cold) and math.isfinite(candidate),'finite quote')
            if arm=='revise3':
                cev = next(c,None)
                need(cev is not None and int(cev['t'])==t and int(cev['truth'])==raw[t]
                     and cev['pattern']==event['pattern'],'C chronology')
                maximum_error = max(maximum_error,near(cev['logcold'],cold,'C cold'),
                                    near(cev['logbank'],candidate,'C candidate'))
            for mode in MODES:
                row = next(outer,None)
                need(row is not None and int(row['t'])==t and row['arm']==arm and
                     row['mode']==mode,'outer record identity')
                maximum_error = max(maximum_error,near(row['logcold'],cold,'outer cold'),
                                    near(row['logcandidate'],candidate,'outer candidate'))
                state = states[arm,mode]
                live,weight,active = state.step(t,cold,candidate)
                maximum_error = max(maximum_error,near(row['loglive'],live,'independent live'),
                                    near(row['source_weight_before'],weight,'source exposure'),
                                    near(row['gain_after'],state.gain,'independent gain'))
                need(int(row['active_before'])==int(active),'prospective admission')
                need((int(row['activation']) if row['activation'] else None)==state.activation,
                     'activation position')
                if mode=='old':
                    maximum_error = max(maximum_error,near(live,event['loglive'],'old turn5 live'))
                    need(int(active)==int(event['active_before']),'old turn5 admission')
                forecast_count += 1
        need(next(outer,None) is None and next(c,None) is None,'no surplus records')
    for arm in ARMS:
        need(states[arm,'old'].activation==states[arm,'fast'].activation,
             'equal admission')
        near(states[arm,'old'].shadow,states[arm,'fast'].shadow,'equal shadow',1e-10)
    return dict(world=world,regime=regime,
                arms={arm:{mode:states[arm,mode].result() for mode in MODES} for arm in ARMS},
                source_sha256=sha(source),outer_sha256=sha(outer_path)),forecast_count,maximum_error


def gate(lives):
    mosaic=[x for x in lives if x['regime']=='mosaic']
    changed=[x for x in lives if x['regime']=='mosaic_then_unrelated']
    mean=lambda xs: math.fsum(xs)/len(xs)
    values=lambda rows,arm,mode,key:[x['arms'][arm][mode][key] for x in rows]
    fast=mean(values(mosaic,'revise3','fast','gain'))
    old=mean(values(mosaic,'revise3','old','gain'))
    null=mean(values(mosaic,'null3','fast','gain'))
    paired=[a-b for a,b in zip(values(changed,'revise3','fast','tail'),
                              values(changed,'revise3','old','tail'))]
    summary=dict(mosaic_fast_mean=fast,mosaic_old_mean=old,
                 mosaic_null_fast_mean=null,mosaic_gain_bpb=fast/N,
                 null_contrast_bpb=(fast-null)/N,retention=fast/old,
                 mosaic_positive=sum(x>0 for x in values(mosaic,'revise3','fast','gain')),
                 changed_paired=paired,changed_mean_improvement=mean(paired),
                 changed_improved=sum(x>0 for x in paired),
                 changed_worst_improvement=min(values(changed,'revise3','fast','tail'))-
                                           min(values(changed,'revise3','old','tail')))
    tests=dict(material_gain=summary['mosaic_gain_bpb']>=.0075,
               null_contrast=summary['null_contrast_bpb']>=.0075,
               positive_worlds=summary['mosaic_positive']==8,
               retention=summary['retention']>=.70,
               tail_mean=summary['changed_mean_improvement']>1,
               tail_count=summary['changed_improved']>=5,
               tail_worst=summary['changed_worst_improvement']>1)
    return summary,tests


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json')
    args=parser.parse_args()
    need(not args.output.exists(),'verification output exists')
    ids=identity()
    with localcontext() as context:
        context.prec=50
        lives=[];count=0;maximum_error=0.0
        for world in WORLDS:
            for regime in REGIMES:
                life,n,error=verify_life(world,regime)
                lives.append(life);count+=n;maximum_error=max(maximum_error,error)
    claim=json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds']==list(WORLDS) and claim['namespace']=='netta-outer-fast-v1',
         'claim identity')
    need(claim['protocol_sha256']==ids['protocol_sha256'],'claim protocol')
    compare_tree(claim['lives'],lives,'lives')
    summary,tests=gate(lives)
    compare_tree(claim['summary'],summary,'summary')
    need(claim['tests']==tests and claim['gate_pass']==all(tests.values()),
         'material gate agreement')
    result=dict(verification_pass=True,gate_pass=all(tests.values()),
                identities=ids,predictions=count,maximum_numeric_error=maximum_error,
                summary=summary,tests=tests,scope='Independent probability-mass outer replay on C pretruth quotes; does not independently reconstruct the frontend or source books.')
    with args.output.open('x') as stream:
        json.dump(result,stream,indent=2,sort_keys=True,allow_nan=False)
        stream.write('\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
