#!/usr/bin/env python3
"""BANK2-REVISE-ROW: one fixed-share change, four frozen comparisons."""
import argparse
import concurrent.futures
import contextlib
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
# Explicit path and private name avoid importing this file as `experiment`.
_spec = importlib.util.spec_from_file_location('netta_frozen_turn3', REPO/'turn3/experiment.py')
t3 = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = t3
_spec.loader.exec_module(t3)
WORLDS = tuple(range(48, 56))
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
ARMS = ('row2', 'static3', 'revise3', 'null3')
N = 16384
RHO = 2.0 ** -10
HAZARD = 2.0 ** -16
PRIOR = (1/8, 7/16, 7/16)
LOG_PRIOR = tuple(math.log2(x) for x in PRIOR)
INITIAL_RATIO = math.log2(3.5)
STATIC_PATH_COST = -math.log1p(-RHO)/math.log(2.0)


def seed(world, component):
    raw = f'netta-revision-v1|{world}|{component}'.encode()
    return int.from_bytes(hashlib.sha256(raw).digest()[:8], 'big')


t3.HERE, t3.REPO, t3.WORLDS, t3.seed = HERE, REPO, WORLDS, seed
FROZEN_NAMES = ('turn5/PROTOCOL.md', 'turn5/experiment.py', 'turn5/verify.py',
    'turn5/revision.c', 'turn5/Makefile', 'turn5/revision',
    'turn3/experiment.py', 'turn3/verify.py', 'turn3/bank.c', 'turn3/bank',
    'turn3/PROTOCOL.md', 'turn3/Makefile',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'byte_recurrence/trace.c', 'byte_recurrence/collect.c',
    'byte_recurrence/Makefile', 'byte_recurrence/byte_trace', 'byte_recurrence/byte_collect',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c')


def freeze():
    t3.save(HERE/'CODE_FREEZE.json', {'created_utc': time.time(), 'worlds': WORLDS,
        'namespace': 'netta-revision-v1', 'protocol_sha256': t3.digest(HERE/'PROTOCOL.md'),
        'files': {name: t3.digest(REPO/name) for name in FROZEN_NAMES}})


def check_frozen():
    receipt = json.loads((HERE/'CODE_FREEZE.json').read_text())
    assert receipt['worlds'] == list(WORLDS) and receipt['namespace'] == 'netta-revision-v1'
    assert receipt['protocol_sha256'] == t3.digest(HERE/'PROTOCOL.md')
    for name, expected in receipt['files'].items():
        assert t3.digest(REPO/name) == expected, name


t3.check_freeze = check_frozen


def logsum(values):
    top = max(values)
    return top + math.log2(math.fsum(2.0**(x-top) for x in values))


def logweights(pair):
    values = (0.0, pair[0], pair[1])
    z = logsum(values)
    return tuple(x-z for x in values)


def quote3(cold, a, b, logs, complete):
    result = [logsum(tuple(w+math.log2(law[g]) for w,law in zip(logs,(cold,a,b))))
              for g in range(len(cold))]
    if complete:
        result[-1] = math.log2(cold[-1])
    return result


def revise(pair, delta_a, delta_b, sharing):
    posterior_logs = logweights((pair[0]+delta_a, pair[1]+delta_b))
    if not sharing:
        return [pair[0]+delta_a, pair[1]+delta_b]
    next_logs = tuple(t3.la(math.log1p(-RHO)/math.log(2.0)+p,
                           math.log2(RHO)+prior) for p,prior in zip(posterior_logs,LOG_PRIOR))
    result = [next_logs[1]-next_logs[0], next_logs[2]-next_logs[0]]
    weights = tuple(2.0**x for x in logweights(result))
    assert all(w >= RHO*p-1e-14 for w,p in zip(weights,PRIOR))
    return result


class Outer:
    def __init__(self):
        self.shadow = self.odds = self.gain = 0.0
        self.active = False
        self.activation = None
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons, self.candidate_horizons = {}, {}

    def step(self, t, cold, bank):
        active, shadow, odds = self.active, self.shadow, self.odds
        live = t3.la(cold, odds+bank)-t3.la(0.0,odds) if active else cold
        self.gain += live-cold
        self.shadow += bank-cold
        activated = False
        if active:
            z = odds+bank-cold
            self.odds = math.log1p(-HAZARD)/math.log(2.0)-t3.la(-z,math.log2(HAZARD))
        elif self.shadow >= 32.0:
            self.active, self.odds, self.activation, activated = True, 0.0, t+1, True
        self.minimum = min(self.minimum,self.gain)
        self.peak = max(self.peak,self.gain)
        self.drawdown = max(self.drawdown,self.peak-self.gain)
        assert self.minimum >= -1-1e-7 and self.drawdown <= 16+1e-7
        if t+1 in (1024,4096,8192,N):
            self.horizons[str(t+1)] = self.gain
            self.candidate_horizons[str(t+1)] = self.shadow
        return live, active, activated, shadow, odds

    def result(self):
        return {'gain':self.gain, 'at8192':self.horizons['8192'],
            'tail':self.gain-self.horizons['8192'], 'activation':self.activation,
            'horizons':self.horizons, 'minimum':self.minimum, 'drawdown':self.drawdown,
            'shadow':self.shadow, 'candidate_horizons':self.candidate_horizons,
            'candidate_tail':self.shadow-self.candidate_horizons['8192'], 'odds':self.odds}


FIELDS = ('t','pattern','truth','group','arm','logbase','logcold','logA','logB',
    'logbank','loglive','weight_cold','weight_A','weight_B',
    'inner_logA_before','inner_logB_before','inner_logA_after','inner_logB_after',
    'inner_odds_before','inner_odds_after','shadow_before','odds_before',
    'active_before','activated_after','gain_after')


def c_replay(world,regime,arm):
    folder = HERE/'results/c'/f'world{world:02d}'
    archive = HERE/'memory'/f'world{world:02d}'/'bank.bin'
    command = ([str(REPO/'turn3/bank'),str(archive),'row'] if arm=='row2' else
               [str(HERE/'revision'),str(archive),'static' if arm=='static3' else 'share'])
    path = folder/f'{regime}-{arm}.tsv.gz'
    t3.run_gzip(command, HERE/'data'/f'world{world:02d}'/f'{regime}.bin', path,
                folder/f'{regime}-{arm}.stderr')
    return path


def compare_c(c,t,p,truth,group,values,active,activated):
    assert int(c['t'])==t and c['pattern']==p and int(c['truth'])==truth and int(c['group'])==group
    assert int(c['active_before'])==int(active) and int(c['activated_after'])==int(activated)
    maximum=0.0
    for name,value in values.items():
        error=abs(float(c[name])-value)
        maximum=max(maximum,error)
        assert error <= 1e-7, (t,p,name,error)
    return maximum


def evaluate_life(world,regime,books,real,null,support):
    paths={arm:c_replay(world,regime,arm) for arm in ARMS[:3]}
    local={p:[0]*(t3.K[p]+1) for p in t3.PATTERNS}
    row2={p:0.0 for p in t3.PATTERNS}
    routers={arm:{p:[INITIAL_RATIO,INITIAL_RATIO] for p in t3.PATTERNS} for arm in ARMS[1:]}
    states={arm:Outer() for arm in ARMS}
    raw=(HERE/'data'/f'world{world:02d}'/f'{regime}.bin').read_bytes()
    assert len(raw)==N
    if regime=='mosaic_then_unrelated':
        assert raw[:8192]==(HERE/'data'/f'world{world:02d}'/'mosaic.bin').read_bytes()[:8192]
    output=HERE/'results/events'/f'world{world:02d}'/f'{regime}.tsv.gz'
    output.parent.mkdir(parents=True,exist_ok=True)
    norm_error=c_error=bound_excess=max_candidate_deficit=0.0
    complete=0;visited=set();cold_loss=0.0
    with contextlib.ExitStack() as stack:
        readers={arm:csv.DictReader(stack.enter_context(gzip.open(path,'rt')),delimiter='\t')
                 for arm,path in paths.items()}
        out=stack.enter_context(gzip.open(output,'xt',newline=''))
        writer=csv.writer(out,delimiter='\t');writer.writerow(FIELDS)
        for t,trace in enumerate(t3.rows(world,regime)):
            assert t<N and int(trace['t'])==t and int(trace['truth'])==raw[t]
            assert int(trace['trained_bytes'])==(t//1024)*1024
            base=[2.0**float(trace[f'logp{j}']) for j in range(256)]
            assert all(math.isfinite(x) and x>0 for x in base)
            norm_error=max(norm_error,abs(math.fsum(base)-1.0));assert norm_error<1e-8
            p=trace['pattern'];group=int(trace['group']);logbase=float(trace['logp_base_truth'])
            assert abs(logbase-math.log2(base[raw[t]]))<1e-10
            if p=='-':
                assert group==-1 and int(trace['context_len'])<6
                mass=[1.0];cold=[1.0];b=[];g=0
                pairs={'real':[cold[:],cold[:]],'null':[cold[:],cold[:]]}
            else:
                expansions=[bytes.fromhex(x) for x in trace['unit_hex'].split(',')]
                pattern,heads=t3.canonical([x[0] for x in expansions])
                assert len(expansions)==6 and pattern==p and heads==list(map(int,trace['heads'].split(',')))
                g=heads.index(raw[t]) if raw[t] in heads else t3.K[p]
                assert group==g
                mass=[base[h] for h in heads]+[math.fsum(v for c,v in enumerate(base) if c not in heads)]
                b=local[p];total=sum(b)
                cold=[.5*m+.5*(v+.5)/(total+.5*len(mass)) for m,v in zip(mass,b)]
                pairs={name:[t3.component(cold,mass,b,prior[p],support[j][p]) for j,prior in enumerate(pair)]
                       for name,pair in (('real',real),('null',null))}
            logcold=logbase+math.log2(cold[g]/mass[g]);cold_loss-=logcold
            assert abs(math.fsum(cold)-1.0)<1e-8
            for arm in ARMS:
                pair=pairs['null' if arm=='null3' else 'real']
                loga,logb=(logbase+math.log2(q[g]/mass[g]) for q in pair)
                if arm=='row2':
                    ell=row2[p] if p!='-' else 0.0
                    denominator=t3.la(0.0,ell)
                    inner_logs=(-denominator,ell-denominator)
                    weights=(0.0,2.0**inner_logs[0],2.0**inner_logs[1])
                    bank_logs=[t3.la(inner_logs[0]+math.log2(pair[0][j]),
                                     inner_logs[1]+math.log2(pair[1][j])) for j in range(len(cold))]
                    if p!='-':bank_logs[-1]=math.log2(cold[-1])
                    before=after=['',''];odds_before=ell
                else:
                    before=routers[arm][p][:] if p!='-' else [INITIAL_RATIO,INITIAL_RATIO]
                    logs=logweights(before);weights=tuple(2.0**x for x in logs)
                    assert all(math.isfinite(x) and x>=0 for x in weights)
                    assert abs(math.fsum(weights)-1.0)<1e-12
                    if arm in ('revise3','null3'):
                        assert all(w>=RHO*pi-1e-14 for w,pi in zip(weights,PRIOR))
                    bank_logs=quote3(cold,*pair,logs,p!='-')
                    odds_before=odds_after=''
                if p=='-':
                    bank_logs=[0.0]
                bank=[2.0**x for x in bank_logs]
                assert all(math.isfinite(x) and x>0 for x in bank)
                norm_error=max(norm_error,abs(math.fsum(bank)-1.0));assert norm_error<1e-8
                if p!='-':assert abs(bank[-1]-cold[-1])<1e-10
                logbank=logbase+bank_logs[g]-math.log2(mass[g])
                if p!='-' and g==t3.K[p]:logbank=logcold
                state=states[arm]
                if state.active:
                    z=t3.la(0.0,state.odds)
                    live_groups=[2.0**t3.la(math.log2(cold[j])-z,bank_logs[j]+state.odds-z)
                                 for j in range(len(cold))]
                else:live_groups=cold
                assert all(math.isfinite(x) and x>0 for x in live_groups)
                norm_error=max(norm_error,abs(math.fsum(live_groups)-1.0));assert norm_error<1e-8
                if p!='-':assert abs(live_groups[-1]-cold[-1])<1e-10
                live,active,activated,shadow,outer=state.step(t,logcold,logbank)
                # NEW carries no likelihood evidence; complete NEW still ticks fixed-share.
                da=db=0.0
                if p!='-' and g!=t3.K[p]:da,db=loga-logcold,logb-logcold
                if arm=='row2':
                    odds_after=ell+(logb-loga if p!='-' and g!=t3.K[p] else 0.0)
                    if p!='-':row2[p]=odds_after
                else:
                    after=revise(before,da,db,arm!='static3') if p!='-' else before[:]
                    if p!='-':routers[arm][p]=after
                if arm in readers:
                    event=next(readers[arm])
                    expected={'logbase':logbase,'logcold':logcold,'logA':loga,'logB':logb,
                        'logbank':logbank,'loglive':live,'shadow_before':shadow,'gain_after':state.gain}
                    if arm=='row2':expected.update(inner_odds_before=ell,outer_odds_before=outer)
                    else:
                        expected.update(inner_logA_before=before[0],inner_logB_before=before[1],
                            inner_logA_after=after[0],inner_logB_after=after[1],
                            weight_cold=weights[0],weight_A=weights[1],weight_B=weights[2],odds_before=outer)
                    c_error=max(c_error,compare_c(event,t,p,raw[t],group,expected,active,activated))
                writer.writerow((t,p,raw[t],group,arm,logbase,logcold,loga,logb,logbank,live,
                    *weights,*before,*after,odds_before,odds_after,shadow,outer,
                    int(active),int(activated),state.gain))
            if p!='-':
                complete+=1;visited.add(p);local[p][g]+=1
            allowed=(complete-len(visited))*STATIC_PATH_COST
            deficit=states['static3'].shadow-states['revise3'].shadow
            max_candidate_deficit=max(max_candidate_deficit,deficit)
            bound_excess=max(bound_excess,deficit-allowed)
            assert deficit<=allowed+1e-7,(world,regime,t,deficit,allowed)
        assert t+1==N
        for reader in readers.values():assert next(reader,None) is None
    return {'world':world,'regime':regime,'arms':{a:s.result() for a,s in states.items()},
        'checks':{'max_c_error':c_error,'max_norm_error':norm_error,'max_candidate_bound_excess':bound_excess},
        'candidate_static_bound':{'complete_events':complete,'visited_rows':len(visited),
            'final_bound':(complete-len(visited))*STATIC_PATH_COST,'maximum_deficit':max_candidate_deficit},
        'cold_loss':cold_loss,'event_sha256':t3.digest(output),
        'c_trace_sha256':{a:t3.digest(path) for a,path in paths.items()}}


def evaluate_world(world):
    books=t3.load_bank(HERE/'memory'/f'world{world:02d}'/'bank.bin')
    cargos,donors,_,_=t3.cargos(world,books)
    support=[{p:sum(c) for p,c in book.items()} for book in books]
    t3.save(HERE/'results'/f'world{world:02d}-memory.json', {'counts':books,'support':support,
        'null_donors':donors['null0'],'null_seed':seed(world,'head-bank-null-0'),
        'observation_law':'HEAD256-v1'})
    lives=[]
    for regime in REGIMES:
        lives.append(evaluate_life(world,regime,books,cargos['real'],cargos['null0'],support))
        print('evaluated',world,regime,lives[-1]['arms']['revise3']['gain'],flush=True)
    return lives


def summary_and_gate(lives):
    mosaic=[x for x in lives if x['regime']=='mosaic']
    changed=[x for x in lives if x['regime']=='mosaic_then_unrelated']
    mean=lambda xs:math.fsum(xs)/len(xs)
    gains={a:mean([x['arms'][a]['gain'] for x in mosaic]) for a in ARMS}
    revised=gains['revise3']
    retention={a:revised/gains[a] if gains[a]>0 else None for a in ('row2','static3')}
    tail={}
    for comparator in ('row2','static3'):
        diffs=[x['arms']['revise3']['tail']-x['arms'][comparator]['tail'] for x in changed]
        candidate=[x['arms']['revise3']['candidate_tail']-x['arms'][comparator]['candidate_tail'] for x in changed]
        tail[comparator]={'paired_differences':diffs,'mean_improvement':mean(diffs),
            'improved_worlds':sum(x>0 for x in diffs),
            'worst_improvement':min(x['arms']['revise3']['tail'] for x in changed)-min(x['arms'][comparator]['tail'] for x in changed),
            'candidate_paired_differences':candidate,'candidate_mean_improvement':mean(candidate)}
    summary={'mosaic_mean_gains':gains,'mosaic_gain_bpb':revised/N,
        'mosaic_null_contrast_bpb':(revised-gains['null3'])/N,
        'mosaic_positive_worlds':sum(x['arms']['revise3']['gain']>0 for x in mosaic),
        'retained_fraction':retention,'changed_tail':tail}
    tests={'material_gain':summary['mosaic_gain_bpb']>=.0075,
        'null_contrast':summary['mosaic_null_contrast_bpb']>=.0075,
        'all_worlds_positive':summary['mosaic_positive_worlds']==8}
    for a in ('row2','static3'):
        tests[f'retention_{a}']=retention[a] is not None and retention[a]>=.70
        tests[f'changed_tail_mean_{a}']=tail[a]['mean_improvement']>1.0
        tests[f'changed_tail_count_{a}']=tail[a]['improved_worlds']>=5
        tests[f'changed_tail_worst_{a}']=tail[a]['worst_improvement']>1.0
    return summary,tests


def evaluate():
    check_frozen()
    for kind in ('data','traces','memory'):t3.check_manifest(HERE/kind/'MANIFEST.json')
    if (HERE/'results').exists():raise RuntimeError('results already exist; no overwrite')
    with concurrent.futures.ProcessPoolExecutor(max_workers=4) as workers:
        worlds=list(workers.map(evaluate_world,WORLDS))
    lives=[life for world in worlds for life in world]
    summary,tests=summary_and_gate(lives)
    t3.save(HERE/'RESULT.json',{'worlds':WORLDS,'namespace':'netta-revision-v1',
        'protocol_sha256':t3.digest(HERE/'PROTOCOL.md'),'lives':lives,'summary':summary,
        'tests':tests,'gate_pass':all(tests.values()),'independent_gate_pending':True})
    t3.save(HERE/'ARTIFACT_MANIFESTS.json',{name:t3.manifest(HERE/name)
        for name in ('data','traces','memory','results')})
    print(json.dumps({'summary':summary,'tests':tests,'gate_pass':all(tests.values())},indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('stage',choices=('freeze','generate','extract','evaluate'))
    args=parser.parse_args()
    if args.stage=='freeze':freeze()
    elif args.stage=='generate':t3.generate()
    elif args.stage=='extract':t3.extract()
    else:evaluate()
