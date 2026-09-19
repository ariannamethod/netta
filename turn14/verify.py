#!/usr/bin/env python3
"""Independent source/candidate reconstruction and two-mass outer replay."""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(144, 152))
REGIMES = ('recombined', 'unrelated', 'switched')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
N = 16384
NAMESPACE = 'netta-joint-fast-withdraw-v1'
HAZARDS = {'slow': Decimal(1)/Decimal(65536),
           'fast': Decimal(1)/Decimal(1024)}
TOL = 1e-7

spec = importlib.util.spec_from_file_location('independent_turn13_reader', REPO/'turn13'/'verify.py')
v13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = v13
spec.loader.exec_module(v13)
v13.HERE = HERE
v13.REPO = REPO
v13.WORLDS = WORLDS


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def near(actual, expected, label, tolerance=TOL):
    error = abs(float(actual)-float(expected))
    need(math.isfinite(float(actual)) and math.isfinite(float(expected)) and
         error <= tolerance, f'{label}: {actual} != {expected}, error={error}')
    return error


def mass_log2(value):
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class MassOuter:
    """Probability-mass reconstruction, separate from writer log-odds code."""
    def __init__(self, hazard):
        self.hazard = hazard
        self.source = self.cold = Decimal('0.5')
        self.active = False
        self.activation = None
        self.shadow = self.gain = 0.0
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}

    def step(self, t, cold_price, candidate_price):
        active = self.active
        shadow = self.shadow
        odds = mass_log2(self.source/self.cold) if active else 0.0
        increment = 0.0
        if active:
            relative = Decimal.from_float(math.exp2(candidate_price-cold_price))
            joint = self.source*relative
            total = self.cold+joint
            increment = mass_log2(total)
            posterior = joint/total
            self.source = (Decimal(1)-self.hazard)*posterior
            self.cold = Decimal(1)-self.source
            need(self.source > 0 and self.cold > 0, 'positive authority masses')
        self.gain += increment
        self.shadow += candidate_price-cold_price
        activated = False
        if not active and self.shadow >= 32.0:
            self.active = True
            self.activation = t+1
            self.source = self.cold = Decimal('0.5')
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        if t+1 in (1024,4096,8192,N):
            self.horizons[str(t+1)] = self.gain
        return dict(candidate=candidate_price, live=cold_price+increment,
                    shadow_before=shadow, odds_before=odds,
                    active_before=int(active), activated_after=int(activated),
                    gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, candidate_gain=self.shadow,
                    tail=self.gain-self.horizons['8192'],
                    horizons=self.horizons, minimum=self.minimum,
                    peak=self.peak, max_drawdown=self.drawdown,
                    activation=self.activation)


def verify_fast(world, regime, slow_life):
    source = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    outer = HERE/'results'/'outer'/f'world{world}'/(regime+'.tsv.gz')
    states = {(arm, mode):MassOuter(hazard)
              for arm in ARMS for mode,hazard in HAZARDS.items()}
    max_error = 0.0
    predictions = 0
    with gzip.open(source, 'rt') as incoming, gzip.open(outer, 'rt') as saved:
        rows = csv.DictReader(incoming, delimiter='\t')
        saved_rows = csv.DictReader(saved, delimiter='\t')
        for t, row in enumerate(rows):
            need(int(row['t']) == t, 'source chronology')
            cold = float(row['logcold'])
            for arm in ARMS:
                candidate = float(row[arm+'_candidate'])
                slow = states[arm,'slow'].step(t,cold,candidate)
                for field,wanted in slow.items():
                    actual = row[arm+'_'+field]
                    if field in ('active_before','activated_after'):
                        need(int(actual) == wanted, 'slow state '+field)
                    else:
                        max_error=max(max_error,near(actual,wanted,'slow '+field))
                fast = states[arm,'fast'].step(t,cold,candidate)
                saved_row = next(saved_rows, None)
                need(saved_row is not None and int(saved_row['t']) == t and
                     saved_row['arm'] == arm, 'fast trace chronology')
                fields = {'logcold':cold, 'logcandidate':candidate,
                          'fast_live':fast['live'],
                          'fast_shadow_before':fast['shadow_before'],
                          'fast_odds_before':fast['odds_before'],
                          'fast_active_before':fast['active_before'],
                          'fast_activated_after':fast['activated_after'],
                          'fast_gain_after':fast['gain_after']}
                for field,wanted in fields.items():
                    if field in ('fast_active_before','fast_activated_after'):
                        need(int(saved_row[field]) == wanted, 'fast trace '+field)
                    else:
                        max_error=max(max_error,near(saved_row[field],wanted,'fast '+field))
                need(states[arm,'slow'].activation == states[arm,'fast'].activation,
                     'shared admission')
                need(states[arm,'slow'].shadow == states[arm,'fast'].shadow,
                     'shared shadow')
                predictions += 2
        need(t+1 == N and next(saved_rows,None) is None, 'complete paired traces')
    slow = {arm:states[arm,'slow'].result() for arm in ARMS}
    fast = {arm:states[arm,'fast'].result() for arm in ARMS}
    v13.compare_tree(slow_life['arms'], slow, 'inherited slow life')
    for arm in ARMS:
        need(slow[arm]['minimum'] >= -1-TOL and
             slow[arm]['max_drawdown'] <= 16+TOL, 'slow loss bounds')
        need(fast[arm]['minimum'] >= -1-TOL and
             fast[arm]['max_drawdown'] <= 10+TOL, 'fast loss bounds')
    life = dict(world=world, regime=regime, slow=slow, fast=fast,
                source_trace_sha256=digest(source), outer_trace_sha256=digest(outer))
    return life,predictions,max_error


def summarize(lives):
    by={(x['world'],x['regime']):x for x in lives}
    mean=lambda values:math.fsum(values)/len(values)
    modes={}; mode_tests={}
    for mode in HAZARDS:
        early=[by[w,'recombined'][mode]['episode']['horizons']['4096'] for w in WORLDS]
        full=[by[w,'recombined'][mode]['episode']['gain'] for w in WORLDS]
        diffs={arm:[by[w,'recombined'][mode]['episode']['horizons']['4096']-
                         by[w,'recombined'][mode][arm]['horizons']['4096'] for w in WORLDS]
               for arm in ('frequency','row','reverse','permuted','flat')}
        switched=[by[w,'switched'][mode]['episode']['gain']-
                  by[w,'switched'][mode]['row']['gain'] for w in WORLDS]
        tails=[by[w,'switched'][mode]['episode']['tail']-
               by[w,'switched'][mode]['row']['tail'] for w in WORLDS]
        modes[mode]=dict(early=early,full=full,early_gain_bpb=mean(early)/4096,
            full_gain_bpb=mean(full)/N,early_differences=diffs,
            switched_differences=switched,tail_differences=tails,
            means={regime:{arm:{field:mean([by[w,regime][mode][arm][field] for w in WORLDS])
                                    for field in ('gain','tail','candidate_gain')}
                           for arm in ARMS} for regime in REGIMES})
        tests=dict(early_gain=mean(early)/4096>=.005,
                   early_positive=sum(x>0 for x in early)>=6,
                   full_gain=mean(full)/N>=.005,
                   full_positive=all(x>0 for x in full),
                   switched_mean=mean(switched)>=0,
                   switched_count=sum(x>0 for x in switched)>=5,
                   tail_tolerance=mean(tails)>=-1)
        for arm,values in diffs.items():
            tests[arm+'_early_mean']=mean(values)>1
            tests[arm+'_early_count']=sum(x>0 for x in values)>=5
        need(len(tests)==17,'seventeen inherited tests')
        mode_tests[mode]=tests
    slow_early=mean(modes['slow']['early']); fast_early=mean(modes['fast']['early'])
    slow_full=mean(modes['slow']['full']); fast_full=mean(modes['fast']['full'])
    slow_switched=modes['slow']['means']['switched']['episode']['gain']
    fast_switched=modes['fast']['means']['switched']['episode']['gain']
    changes=[by[w,'switched']['fast']['episode']['tail']-
             by[w,'switched']['slow']['episode']['tail'] for w in WORLDS]
    paired=dict(early_retention=fast_early/slow_early,
                full_retention=fast_full/slow_full,
                switched_retention=fast_switched/slow_switched,
                changed_tail_improvements=changes,
                changed_tail_mean=mean(changes),
                changed_tail_improved=sum(x>0 for x in changes),
                changed_tail_worst_lift=(min(by[w,'switched']['fast']['episode']['tail'] for w in WORLDS)-
                                         min(by[w,'switched']['slow']['episode']['tail'] for w in WORLDS)))
    tests={'fast_'+key:value for key,value in mode_tests['fast'].items()}
    tests.update(retain_early=slow_early>0 and paired['early_retention']>=.90,
                 retain_full=slow_full>0 and paired['full_retention']>=.90,
                 retain_switched=slow_switched>0 and paired['switched_retention']>=.90,
                 tail_mean=paired['changed_tail_mean']>1,
                 tail_count=paired['changed_tail_improved']>=5,
                 tail_worst=paired['changed_tail_worst_lift']>1)
    return dict(modes=modes,paired=paired),tests


def compare(actual, expected, label):
    if isinstance(expected,dict):
        need(isinstance(actual,dict) and set(actual)==set(expected),label+' keys')
        for key,value in expected.items(): compare(actual[key],value,label+'/'+key)
    elif isinstance(expected,list):
        need(isinstance(actual,list) and len(actual)==len(expected),label+' list')
        for i,(a,b) in enumerate(zip(actual,expected)): compare(a,b,label+f'/{i}')
    elif isinstance(expected,(bool,str,int)) or expected is None:
        need(actual==expected,label+' exact')
    else:
        near(actual,expected,label)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json')
    args=parser.parse_args()
    need(not args.output.exists(),'verification output already exists')
    frozen=json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['worlds']==list(WORLDS) and frozen['namespace']==NAMESPACE,'freeze scope')
    for name,wanted in frozen['files'].items():
        need(digest(REPO/name)==wanted,'frozen code '+name)
    for manifest_name in ('DATA_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        for path,wanted in json.loads((HERE/manifest_name).read_text()).items():
            need(digest(HERE/path)==wanted,'retained artifact '+path)
    claim=json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds']==list(WORLDS) and claim['namespace']==NAMESPACE,'claim scope')
    need(claim['protocol_sha256']==frozen['files']['turn14/PROTOCOL.md'],'claim protocol')
    lives=[]; books=[]; predictions=0; max_error=0.0; inherited_predictions=0
    with localcontext() as context:
        context.prec=50
        for world in WORLDS:
            rebuilt=v13.verify_books(world)
            books.append(dict(world=world,**rebuilt[-1]))
            for regime in REGIMES:
                slow,count,error,norm,_=v13.verify_life(world,regime,rebuilt)
                need(norm<=1e-8,'candidate normalization')
                inherited_predictions+=count
                max_error=max(max_error,error)
                life,count,error=verify_fast(world,regime,slow)
                lives.append(life); predictions+=count; max_error=max(max_error,error)
    need(inherited_predictions==8*3*N*7,'all inherited forecasts')
    need(predictions==8*3*N*7*2,'all paired authority forecasts')
    summary,tests=summarize(lives)
    compare(claim['lives'],lives,'lives')
    compare(claim['summary'],summary,'summary')
    need(claim['tests']==tests,'material tests')
    identities=dict(candidate_shared=True,shadow_shared=True,admission_shared=True,
                    slow_bound=True,fast_bound=True)
    need(claim['identities']==identities,'identity claims')
    gate=all(tests.values()) and all(identities.values())
    need(claim['gate_pass']==gate,'gate verdict')
    result=dict(verification_pass=True,gate_pass=gate,
        source_observations=8*4*N,target_observations=8*3*N,
        inherited_candidate_forecasts=inherited_predictions,
        paired_authority_forecasts=predictions,maximum_numeric_error=max_error,
        identities=dict(protocol_sha256=frozen['files']['turn14/PROTOCOL.md'],
                        freeze_sha256=digest(HERE/'FREEZE.json'),
                        result_sha256=digest(HERE/'RESULT.json'),
                        reader_sha256=digest(Path(__file__).resolve())),
        source_books=books,summary=summary,tests=tests,
        scope='Independent turn13 source/archive/candidate reconstruction plus '
              'separate Decimal probability-mass replay of both authority laws.')
    with args.output.open('x') as stream:
        json.dump(result,stream,indent=2,sort_keys=True,allow_nan=False); stream.write('\n')
    print(json.dumps(result,indent=2,sort_keys=True))


if __name__=='__main__':
    main()
