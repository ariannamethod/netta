#!/usr/bin/env python3
"""Independent turn13 source reader plus Decimal prospective authority replay."""
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
WORLDS = tuple(range(184,192))
REGIMES = ('recombined','switched','moved_mid','unrelated')
MODES = ('slow','fast','adaptive')
NAMESPACE = 'netta-prospective-hazard-v1'
N = 16384
MOVE = 8192
TOL = 1e-7
PROTOCOL_SHA = '3fbb761054e432725b5b8dc1b16c728f85b892ac08e3b0cda882bb0de4ba2188'

spec = importlib.util.spec_from_file_location('turn17_independent_source',
                                               REPO/'turn13/verify.py')
v13 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = v13
spec.loader.exec_module(v13)
v13.HERE, v13.REPO, v13.WORLDS = HERE, REPO, WORLDS


def need(condition,label):
    if not condition:
        raise AssertionError(label)


def near(actual,expected,label,tolerance=TOL):
    a,b = float(actual),float(expected)
    need(math.isfinite(a) and math.isfinite(b),label+' finite')
    error = abs(a-b)
    need(error <= tolerance,f'{label}: {a} != {b}, error={error}')
    return error


def digest(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for part in iter(lambda: stream.read(1<<20),b''):
            h.update(part)
    return h.hexdigest()


def mass_log2(value):
    need(value>0,'positive probability mass')
    simple=float(value)
    if simple>=2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent=value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


class Authority:
    def __init__(self,mode):
        need(mode in MODES,'authority mode')
        self.mode=mode
        self.source=self.cold=Decimal('0.5')
        self.shadow=self.evidence=self.gain=0.0
        self.active=False
        self.minimum=self.peak=self.drawdown=0.0
        self.activation=None
        self.horizons={}
        self.slow_count=self.fast_count=0

    def step(self,t,cold,candidate):
        need(math.isfinite(cold) and math.isfinite(candidate),'finite prices')
        before=dict(shadow_before=self.shadow,active_before=int(self.active),
                    odds_before=mass_log2(self.source/self.cold) if self.active else 0.0,
                    e_before=self.evidence)
        delta=candidate-cold
        slow_used=-1
        increment=0.0
        if self.active:
            slow_used=int(self.mode=='slow' or
                          (self.mode=='adaptive' and self.evidence>=1.0))
            hazard=Decimal(1)/Decimal(65536 if slow_used else 1024)
            relative=Decimal.from_float(math.exp2(delta))
            joint=self.source*relative
            total=self.cold+joint
            need(total>0 and joint>0,'positive mixed mass')
            increment=0.0 if candidate==cold else mass_log2(total)
            self.source=(1-hazard)*(joint/total)
            self.cold=1-self.source
            need(self.source>0 and self.cold>0,'positive updated masses')
            near(self.source+self.cold,1,'normalized authority',1e-12)
            if self.mode=='adaptive':
                self.evidence=(255.0/256.0)*self.evidence+delta
            if slow_used:
                self.slow_count+=1
            else:
                self.fast_count+=1
        self.gain+=increment
        self.shadow+=delta
        admitted=False
        if not before['active_before'] and self.shadow>=32.0:
            self.active=True
            self.activation=t+1
            self.source=self.cold=Decimal('0.5')
            self.evidence=0.0
            admitted=True
        self.minimum=min(self.minimum,self.gain)
        self.peak=max(self.peak,self.gain)
        self.drawdown=max(self.drawdown,self.peak-self.gain)
        bound=10 if self.mode=='fast' else 16
        need(self.minimum>=-1-TOL and self.drawdown<=bound+TOL,
             self.mode+' prefix/interval bound')
        if t+1 in (1024,4096,MOVE,N):
            self.horizons[str(t+1)]=self.gain
        return dict(**before,live=cold if candidate==cold else cold+increment,
                    admitted_after=int(admitted),slow_used=slow_used,
                    e_after=self.evidence,
                    odds_after=mass_log2(self.source/self.cold) if self.active else 0.0)

    def result(self):
        return dict(gain=self.gain,tail=self.gain-self.horizons[str(MOVE)],
                    minimum=self.minimum,peak=self.peak,max_drawdown=self.drawdown,
                    activation=self.activation,horizons=self.horizons,
                    slow_count=self.slow_count,fast_count=self.fast_count)


def verify_manipulation():
    rows=[]
    for world in WORLDS:
        folder=HERE/'data'/f'world{world}'
        raw={r:(folder/(r+'.bin')).read_bytes() for r in REGIMES}
        need(all(len(body)==N for body in raw.values()),'target lengths')
        seed_bytes=hashlib.sha256(f'{NAMESPACE}|{world}|surface'.encode()).digest()[:8]
        order=list(range(256))
        random.Random(int.from_bytes(seed_bytes,'big')).shuffle(order)
        need(sorted(order)==list(range(256)),'surface bijection')
        need(raw['switched'][:MOVE]==raw['recombined'][:MOVE],
             'switched prefix unchanged')
        need(raw['moved_mid']==raw['recombined'][:MOVE]+
             bytes(order[b] for b in raw['recombined'][MOVE:]),
             'independent surface manipulation')
        hidden=json.loads((folder/'SURFACE.json').read_text())
        need(hidden['seed']==int.from_bytes(seed_bytes,'big') and
             hidden['world']==world and hidden['move']==MOVE and
             hidden['surface_map']==order,'declared surface transformation')
        rows.append(dict(world=world,prefix_identical=True,bijection=True,
                         differing_tail=sum(a!=b for a,b in zip(
                             raw['moved_mid'][MOVE:],raw['recombined'][MOVE:]))))
    return rows


def verify_authority(world,regime,inherited):
    source=HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    replay=HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz')
    states={m:Authority(m) for m in MODES}
    max_error=0.0
    with gzip.open(source,'rt') as f,gzip.open(replay,'rt') as g:
        candidates=csv.DictReader(f,delimiter='\t')
        authority=csv.DictReader(g,delimiter='\t')
        for t in range(N):
            raw=next(candidates,None)
            row=next(authority,None)
            need(raw is not None and row is not None and
                 int(raw['t'])==int(row['t'])==t,'complete chronological trace')
            cold=float(raw['logcold'])
            candidate=float(raw['episode_candidate'])
            need(float(row['cold'])==cold and float(row['candidate'])==candidate,
                 'identical C/S price inputs')
            expected={m:states[m].step(t,cold,candidate) for m in MODES}
            common=expected['slow']
            fields={k:common[k] for k in
                    ('shadow_before','active_before','admitted_after')}
            need(int(row['admitted_after'])==int(raw['episode_activated_after']) and
                 int(row['active_before'])==int(raw['episode_active_before']),
                 'inherited first admission')
            for mode in MODES:
                need(expected[mode]['shadow_before']==common['shadow_before'] and
                     expected[mode]['active_before']==common['active_before'] and
                     expected[mode]['admitted_after']==common['admitted_after'],
                     'paired first admission')
                fields[mode+'_live']=expected[mode]['live']
                fields[mode+'_odds_before']=expected[mode]['odds_before']
                if candidate==cold:
                    need(float(row[mode+'_live'])==cold,'exact unchanged price '+mode)
                if not common['active_before']:
                    need(float(row[mode+'_live'])==cold,'inactive price '+mode)
            fields.update(adaptive_e_before=expected['adaptive']['e_before'],
                          adaptive_slow_used=expected['adaptive']['slow_used'],
                          adaptive_e_after=expected['adaptive']['e_after'],
                          adaptive_odds_after=expected['adaptive']['odds_after'])
            for key,wanted in fields.items():
                if isinstance(wanted,int):
                    need(int(row[key])==wanted,'authority flag '+key)
                else:
                    max_error=max(max_error,near(row[key],wanted,'authority '+key))
        need(next(candidates,None) is None and next(authority,None) is None,
             'no extra observations')
    original_keys=('gain','tail','horizons','minimum','peak','max_drawdown','activation')
    v13.compare_tree(inherited['arms']['episode'],
                     {k:states['slow'].result()[k] for k in original_keys},
                     'inherited slow authority')
    return dict(world=world,regime=regime,
                modes={m:states[m].result() for m in MODES},
                source_trace_sha256=digest(source),
                authority_trace_sha256=digest(replay)),N*len(MODES),max_error


def compare(actual,expected,label):
    if isinstance(expected,dict):
        need(isinstance(actual,dict),label+' object')
        for key,value in expected.items():
            need(key in actual,label+' missing '+key)
            compare(actual[key],value,label+'/'+key)
    elif isinstance(expected,list):
        need(isinstance(actual,list) and len(actual)==len(expected),label+' list')
        for i,(a,b) in enumerate(zip(actual,expected)):
            compare(a,b,label+f'/{i}')
    elif isinstance(expected,(bool,str,int)) or expected is None:
        need(actual==expected,label+' exact')
    else:
        near(actual,expected,label)


def independent_gate(lives):
    lookup={(life['world'],life['regime']):life for life in lives}
    need(len(lookup)==len(lives)==32,'complete paired worlds')
    mean=lambda values:math.fsum(values)/len(values)
    def read(regime,mode,field):
        return [lookup[w,regime]['modes'][mode]['horizons']['4096']
                if field=='early' else lookup[w,regime]['modes'][mode][field]
                for w in WORLDS]
    def gap(regime,other):
        return [x-y for x,y in zip(read(regime,'adaptive','tail'),
                                  read(regime,other,'tail'))]
    sf,ss=gap('moved_mid','fast'),gap('moved_mid','slow')
    lf,ls=gap('switched','fast'),gap('switched','slow')
    early_ref=mean(read('recombined','slow','early'))
    full_ref=mean(read('recombined','slow','gain'))
    early_ratio=mean(read('recombined','adaptive','early'))/early_ref if early_ref>0 else None
    full_ratio=mean(read('recombined','adaptive','gain'))/full_ref if full_ref>0 else None
    positive=read('recombined','adaptive','gain')
    tests=dict(surface_vs_fast_mean=mean(sf)>1,
               surface_vs_fast_wins=sum(x>0 for x in sf)>=5,
               surface_vs_slow_retention=mean(ss)>=-3,
               law_vs_fast_retention=mean(lf)>=-1,
               law_vs_slow_mean=mean(ls)>1,
               law_vs_slow_wins=sum(x>0 for x in ls)>=5,
               early_retention=early_ratio is not None and early_ratio>=.95,
               full_retention=full_ratio is not None and full_ratio>=.95,
               full_positive=all(x>0 for x in positive),
               first_admission_shared=all(len({life['modes'][m]['activation']
                   for m in MODES})==1 for life in lives),
               episode_null=all(lookup[w,'unrelated']['modes']['adaptive']['activation']
                                is None for w in WORLDS),
               archive_cap=all((HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size
                               <=528 for w in WORLDS),
               prefix_and_interval_bounds=all(s['minimum']>=-1-TOL and
                   s['max_drawdown']<=(10 if m=='fast' else 16)+TOL
                   for life in lives for m,s in life['modes'].items()))
    need(len(tests)==13,'thirteen preregistered conditions')
    return dict(surface_vs_fast=sf,surface_vs_slow=ss,law_vs_fast=lf,
                law_vs_slow=ls,early_retention=early_ratio,
                full_retention=full_ratio,recombined_full=positive),tests


def identity():
    need(digest(HERE/'PROTOCOL.md')==PROTOCOL_SHA,'frozen protocol identity')
    frozen=json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace']==NAMESPACE and frozen['worlds']==list(WORLDS) and
         frozen['regimes']==list(REGIMES) and frozen['modes']==list(MODES),
         'freeze scope')
    for name,wanted in frozen['files'].items():
        need(digest(REPO/name)==wanted,'frozen file '+name)
    pins=dict(protocol_sha256=PROTOCOL_SHA,reader_sha256=digest(Path(__file__)),
              freeze_sha256=digest(HERE/'FREEZE.json'),
              result_sha256=digest(HERE/'RESULT.json'))
    for name in ('DATA_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        manifest=json.loads((HERE/name).read_text())
        for path,wanted in manifest.items():
            need(digest(HERE/path)==wanted,'retained artifact '+path)
        pins[name]=digest(HERE/name)
    return pins


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json')
    args=parser.parse_args()
    need(not args.output.exists(),'verification output already exists')
    pins=identity()
    claim=json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds']==list(WORLDS) and claim['regimes']==list(REGIMES) and
         claim['modes']==list(MODES) and claim['namespace']==NAMESPACE and
         claim['protocol_sha256']==PROTOCOL_SHA,'claimed scope')
    surface=verify_manipulation()
    lives=[]
    candidate_count=authority_count=0
    max_error=max_norm=0.0
    with localcontext() as context:
        context.prec=50
        for world in WORLDS:
            books=v13.verify_books(world)
            for regime in REGIMES:
                inherited,count,error,norm,_=v13.verify_life(world,regime,books)
                candidate_count+=count
                max_error=max(max_error,error)
                max_norm=max(max_norm,norm)
                life,count,error=verify_authority(world,regime,inherited)
                authority_count+=count
                max_error=max(max_error,error)
                life['max_norm_error']=norm
                lives.append(life)
    need(candidate_count==8*4*N*7,'all inherited candidate forecasts')
    need(authority_count==8*4*N*3,'all authority forecasts')
    compare(claim['lives'],lives,'all life statistics')
    compare(claim['manipulation'],surface,'surface manipulation')
    quantities,tests=independent_gate(lives)
    compare(claim['quantities'],quantities,'gate quantities')
    need(claim['tests']==tests and claim['gate_pass']==all(tests.values()),
         'material gate reconstruction')
    result=dict(verification_pass=True,gate_pass=all(tests.values()),
                candidate_forecasts=candidate_count,authority_forecasts=authority_count,
                maximum_numeric_error=max_error,maximum_normalization_error=max_norm,
                worlds=list(WORLDS),regimes=list(REGIMES),modes=list(MODES),
                identities=pins,quantities=quantities,tests=tests,
                scope='Independent frozen turn13 source books and seven candidates; '
                      'independent Decimal mass replay of episode slow/fast/adaptive '
                      'authority, every quote and clock. HEAD256 bindings and P0 '
                      'are supplied trace inputs; full-vector receipts come from C.')
    with args.output.open('x') as stream:
        json.dump(result,stream,indent=2,sort_keys=True,allow_nan=False)
        stream.write('\n')
    print(json.dumps({k:result[k] for k in ('verification_pass','gate_pass',
        'candidate_forecasts','authority_forecasts','maximum_numeric_error',
        'quantities','tests')},indent=2,sort_keys=True))


if __name__=='__main__':
    main()
