#!/usr/bin/env python3
"""Independent source recount and probability-space hierarchical replay.

Imports frozen readers only. No experiment writer or C code is imported.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal, localcontext
import gzip
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(320,328))
REGIMES = ('recombined','partial','switched','moved_mid','unrelated')
ARMS = ('hier','coarse','fine','null','pooled','cold')
NAMESPACE = 'netta-parent-relative-detail-v1'
N, MOVE, EARLY, TOL = 16384, 8192, 4096, 1e-7
spec = importlib.util.spec_from_file_location('turn36_reader34', ROOT/'turns/turn34/verify.py')
p34 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p34
spec.loader.exec_module(p34)
p33, p28 = p34.p33, p34.p28
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES
need, near, digest, compare = p28.need, p28.near, p28.digest, p28.compare


class State:
    def __init__(self):
        self.case = p33.Wealth()
        self.detail = [p33.Wealth(),p33.Wealth()]
        self.null = [p33.Wealth(),p33.Wealth()]
        self.fine = p34.SelWealth()
        self.permission = p33.Permission()
        self.visits = 0

    def vector(self):
        return [self.permission.u,self.case.a,self.case.b]+[
            x for group in (self.detail,self.null) for state in group for x in (state.a,state.b)]+self.fine.h

    def sources(self, values):
        pool,a,b,*_ = values
        refined = [self.detail[p].source(values[1+p],*values[3+2*p:5+2*p]) for p in (0,1)]
        null = [self.null[p].source(values[1+p],*values[7+2*p:9+2*p]) for p in (0,1)]
        return [self.case.source(pool,*refined),self.case.source(pool,a,b),
                self.fine.source(pool,values[3:7]),self.case.source(pool,*null)]

    def supports(self, values):
        refined = [self.detail[p].support(values[1+p],*values[3+2*p:5+2*p]) for p in (0,1)]
        null = [self.null[p].support(values[1+p],*values[7+2*p:9+2*p]) for p in (0,1)]
        return [self.case.support(values[0],*refined),self.case.support(*values[:3]),
                self.fine.support(values[0],values[3:7]),self.case.support(values[0],*null)]

    def observe(self, values, pooled_quote):
        for p in (0,1):
            self.detail[p].observe(values[1+p],*values[3+2*p:5+2*p])
            self.null[p].observe(values[1+p],*values[7+2*p:9+2*p])
        self.case.observe(*values[:3])
        self.fine.observe(values[0],values[3:7])
        self.permission.observe(values[0],pooled_quote)
        self.visits += 1


def floats(text):
    return list(map(float,text.split(',')))


def check_books(world):
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    pairs, expanded = p28.compose(meta['rules'], str(folder))
    need(len(pairs)==32 and len(meta['selected'])==24, 'full24 source shape')
    prefixes = []
    for rec in meta['selected']:
        owner,length = rec['rule_id'],rec['prefix_len']
        need(0<=owner<len(pairs) and 0<length<len(expanded[7+owner]),'source address')
        prefix = expanded[7+owner][:length]
        need(list(prefix)==rec['prefix'] and prefix not in prefixes,'distinct source prefix')
        prefixes.append(prefix)
    tapes = [p28.load_source(world,name) for name in p28.SOURCES]
    counts = p28.recount(tapes,prefixes)
    tables = {name:{} for name in ('pool','case','episode','null')}
    header = struct.pack('<8sII',b'NETEB004',len(pairs),24)+b''.join(struct.pack('<HH',*p) for p in pairs)
    real, scrambled = bytearray(header),bytearray(header)
    bank2,pool = [],[]
    for j,(prefix,rec) in enumerate(zip(prefixes,meta['selected'])):
        episodes = [[counts[i][prefix][r] for r in range(7)] for i in range(4)]
        parents = [[episodes[2*p][r]+episodes[2*p+1][r] for r in range(7)] for p in (0,1)]
        total = [parents[0][r]+parents[1][r] for r in range(7)]
        null = [[episodes[i^1 if r in (1,3,5) else i][r] for r in range(7)] for i in range(4)]
        need(rec['episode_counts']==episodes and rec['book_counts']==parents and rec['counts']==total,'recounted source vectors')
        need(all(null[2*p][r]+null[2*p+1][r]==parents[p][r] for p in (0,1) for r in range(7)),'null keeps parent exactly')
        address = (rec['rule_id'],rec['prefix_len'],0)
        real.extend(struct.pack('<BBH28H',*address,*sum(episodes,[])))
        scrambled.extend(struct.pack('<BBH28H',*address,*sum(null,[])))
        bank2.append((rec['rule_id'],prefix,parents)); pool.append((rec['rule_id'],prefix,total))
        for name,values in zip(tables,(total,parents,episodes,null)):
            tables[name][prefix]=(j,values)
    wanted = {'bank4_full.bin':bytes(real),'null4_full.bin':bytes(scrambled),
              'bank2_full.bin':p28.encode(pairs,bank2,True),'pooled_full.bin':p28.encode(pairs,pool)}
    need([len(wanted[k]) for k in ('bank4_full.bin','null4_full.bin','bank2_full.bin','pooled_full.bin')]==[1584,1584,912,528],'portable byte price')
    for name,data in wanted.items():
        need((folder/name).read_bytes()==data and meta['bytes'][name]==len(data),'rebuilt '+name)
    return tables,dict(world=world,counters=24*4*7,bytes={k:len(v) for k,v in wanted.items()},
        hashes={k:digest(folder/k) for k in wanted},role_histograms=[dict(sorted(Counter(t).items())) for t in tapes])


FIELDS = ('t','k','heads','cold_heads','truth','rank','history','logcold','matched','record','visits',
          'sources','corrected','state_before','state_after','norm','admission_shadow','admitted')+tuple(
          a+'_'+field for a in ARMS for field in ('candidate','live','odds','active','gain'))


def check_life(world,regime,tables,saved):
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw)==N,'complete raw life')
    if regime in ('partial','switched','moved_mid'):
        need(raw[:MOVE]==(HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()[:MOVE],'raw common prefix')
    state = [State() for _ in range(24)]
    outer = {a:p33.SharedOuter() for a in ARMS}
    activity = dict(matched=0,detail_active=0,detail_silent=0,activated=0,revoked=0)
    history,shadow,admitted,error = [],0.,False,0.
    best=worst=None
    def check(x,y,label):
        nonlocal error
        error=max(error,near(x,y,label,TOL))
    path = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    with gzip.open(path,'rt') as f:
        rows=csv.DictReader(f,delimiter='\t')
        need(tuple(rows.fieldnames)==FIELDS,'exact hierarchy trace header')
        for t in range(N):
            row=next(rows,None)
            label=f'{world}/{regime}/{t}'
            need(row is not None and int(row['t'])==t and int(row['truth'])==raw[t],label+' chronology')
            k,heads=int(row['k']),p28.ints(row['heads'])
            rank=p28.bindings(k,heads,raw[t],label)
            need(int(row['rank'])==rank and row['history']==(''.join(map(str,history)) or '-'),label+' pretruth history')
            cold,heads_price=float(row['logcold']),p28.floats(row['cold_heads'])
            need(math.isfinite(cold) and cold<=0 and len(heads_price)==k,label+' supplied P0')
            if rank: check(cold,heads_price[rank-1],label+' truth/head')
            match=[p28.suffix_match(tables[name],history) for name in tables]
            need(all(m[:2]==match[0][:2] for m in match),label+' identical addresses')
            length,record,pool_counts=match[0]
            need(int(row['matched'])==length and int(row['record'])==record,label+' causal match')
            counts=[pool_counts]+(match[1][2] if length else [None]*2)+(match[2][2] if length else [None]*4)+(match[3][2] if length else [None]*4)
            prices=[p28.source_candidate(cold,heads_price,rank,c) for c in counts]
            supplied=floats(row['sources'])
            need(len(supplied)==11,label+' sources')
            for i,(x,y) in enumerate(zip(supplied,prices)): check(x,y,label+f'/source{i}')
            s=state[record] if length else State()
            need(int(row['visits'])==s.visits,label+' visits')
            before=floats(row['state_before'])
            need(len(before)==15,label+' state width')
            for i,(x,y) in enumerate(zip(before,s.vector())): check(x,y,label+f'/before{i}')
            corr=s.sources(prices)
            recorded_corr=floats(row['corrected'])
            for i,(x,y) in enumerate(zip(recorded_corr,corr)): check(x,y,label+f'/corrected{i}')
            candidates={a:s.permission.quote(cold,corr[i]) for i,a in enumerate(ARMS[:4])}
            candidates.update(pooled=s.permission.quote(cold,prices[0]),cold=cold)
            for a in ARMS: check(row[a+'_candidate'],candidates[a],label+'/'+a+'/candidate')
            local,support0=p33.distribution(heads_price,pool_counts)
            supports=[support0]+[p33.distribution(heads_price,c)[1] for c in counts[1:]]
            refined=s.supports(supports)
            u=float(s.permission.u)
            c_support={a:[(1-u)*x+u*y for x,y in zip(local,refined[i])] for i,a in enumerate(ARMS[:4])}
            c_support.update(pooled=[(1-u)*x+u*y for x,y in zip(local,support0)],cold=local)
            residual=1-math.fsum(local)
            need(0<=float(row['norm'])<=1e-8,label+' full C normalization')
            for a in ARMS:
                p33.normalized(label+'/'+a,residual,c_support[a])
                o=outer[a]
                support=[float(o.cold)*x+float(o.source)*y for x,y in zip(local,c_support[a])] if o.active else local
                p33.normalized(label+'/'+a+'/live',residual,support)
            # Exact labels follow C only after all numerical quotes/states above agree.
            if max(before[3:7])<=0:
                need(recorded_corr[0]==recorded_corr[1],label+' silent detail equals coarse')
            if max(before[1:3])<=0:
                need(recorded_corr[0]==supplied[0],label+' silent cases equal pooled')
            if not length or not s.visits:
                need(float(row['hier_candidate'])==float(row['pooled_candidate']),label+' first/fallback')
            check(row['admission_shadow'],shadow,label+' admission shadow')
            need(int(row['admitted'])==int(admitted),label+' admission')
            shadow+=candidates['pooled']-cold
            admit=not admitted and shadow>=32
            if admit: admitted=True
            for a in ARMS:
                q=outer[a].step(t,cold,candidates[a],admit)
                for field,key in (('live','live'),('odds','odds_before'),('gain','gain_after')):
                    check(row[a+'_'+field],q[key],label+'/'+a+'/'+field)
                need(int(row[a+'_active'])==q['active_before'],label+' outer active')
                if not rank or not length:
                    need(float(row[a+'_candidate'])==cold and float(row[a+'_live'])==cold,label+' NEW/no-match passthrough')
                if float(row[a+'_candidate'])==cold or not q['active_before']:
                    need(float(row[a+'_live'])==cold,label+' exact equal/inactive')
            if length: s.observe(prices,candidates['pooled'])
            after=floats(row['state_after'])
            for i,(x,y) in enumerate(zip(after,s.vector())): check(x,y,label+f'/after{i}')
            if length:
                active,next_active=max(before[3:7])>0,max(after[3:7])>0
                activity['matched']+=1
                activity['detail_active' if active else 'detail_silent']+=1
                activity['activated']+=not active and next_active
                activity['revoked']+=active and not next_active
            if t>=MOVE:
                diff=float(row['hier_live'])-float(row['coarse_live'])
                sample=dict(row,difference=diff,raw_hex=raw[max(0,t-16):t+17].hex())
                if best is None or diff>best['difference']: best=sample
                if worst is None or diff<worst['difference']: worst=sample
            history=(history+[rank])[-32:]
        need(next(rows,None) is None,'no excess rows')
    life=dict(world=world,regime=regime,arms={a:o.result() for a,o in outer.items()},activity=activity,raw_help=best,raw_harm=worst)
    for a,s in life['arms'].items():
        need(s['minimum']>=-1-TOL and s['drawdown']<=16+TOL,f'{world}/{regime}/{a} bounds')
    error=max(error,compare(saved,life,f'life/{world}/{regime}'))
    return life,error


def rebuild(lives):
    lookup={(x['world'],x['regime']):x for x in lives}
    values=lambda r,a,k:[lookup[w,r]['arms'][a][k] for w in WORLDS]
    avg=lambda xs: math.fsum(xs)/8
    tables={r:{a:{k:avg(values(r,a,k)) for k in ('early','gain','tail')} for a in ARMS} for r in REGIMES}
    comparisons={}
    for r in REGIMES:
        comparisons[r]={}
        for a in ('coarse','fine','null','pooled'):
            comparisons[r][a]={}
            for k in ('early','gain','tail'):
                delta=[lookup[w,r]['arms']['hier'][k]-lookup[w,r]['arms'][a][k] for w in WORLDS]
                comparisons[r][a][k]=dict(mean=avg(delta),wins=sum(x>0 for x in delta),per_world=delta)
    retain={k:tables['recombined']['hier'][k]/tables['recombined']['coarse'][k] for k in ('early','gain')}
    switched=comparisons['switched']
    def paid(other,margin):
        result=switched[other]['tail']
        return result['mean']>margin and result['wins']>=5
    conditions=dict(H1=paid('coarse',672/100) and switched['coarse']['gain']['mean']>=0,
        H2=paid('fine',1),H3=min(retain.values())>=.99 and all(tables['recombined']['coarse'][k]>0 for k in retain) and
        all(x>0 for x in values('recombined','hier','gain')) and comparisons['partial']['coarse']['tail']['mean']>=0,
        H4=paid('null',1),H5=True)
    for w in WORLDS:
        for r in ('partial','switched','moved_mid'):
            need(all(lookup[w,r]['arms'][a]['horizons'][str(MOVE)]==lookup[w,'recombined']['arms'][a]['horizons'][str(MOVE)] for a in ARMS),'common prediction prefix')
    need(all(len({s['activation'] for s in life['arms'].values()})==1 for life in lives),'common admission')
    return dict(tables=tables,quantities=dict(comparisons=comparisons,retention=retain,
        portable_bytes=dict(hier=1584,coarse=912,fine=1584,null=1584,pooled=528,cold=0),
        recipient_bytes=dict(hier=1344,coarse=576,fine=960,null=1344,pooled=192,cold=0)),conditions=conditions)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json')
    args=parser.parse_args()
    need(not args.output.exists(),'creation-only reader receipt')
    freeze=json.loads((HERE/'FREEZE.json').read_text())
    result=json.loads((HERE/'RESULT.json').read_text())
    for k,v in dict(namespace=NAMESPACE,worlds=list(WORLDS),regimes=list(REGIMES),arms=list(ARMS)).items():
        need(freeze[k]==v and result[k]==v,'identity '+k)
    need(result['independent_reader_pending'] is True and result['conditions']['H5'] is False,'writer is pending')
    need(result['protocol_sha256']==digest(HERE/'PROTOCOL.md'),'protocol identity')
    required=('turns/turn36/verify.py','turns/turn34/verify.py','turns/turn33/verify.py','turns/turn30/verify.py','turns/turn28/verify.py')
    need(all(n in freeze['files'] for n in required),'frozen reader chain')
    for name,wanted in freeze['files'].items(): p28.check_artifact(ROOT/name,wanted)
    for name in ('DATA_MANIFEST.json','EXTRACT_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        for path,wanted in json.loads((HERE/name).read_text()).items(): p28.check_artifact(HERE/path,wanted)
    saved={(x['world'],x['regime']):x for x in result['lives']}
    need(len(saved)==len(result['lives'])==40,'complete life grid')
    lives,books,max_error=[],[],0.
    with localcontext() as context:
        context.prec=50
        for w in WORLDS:
            tables,book=check_books(w); books.append(book)
            for r in REGIMES:
                life,error=check_life(w,r,tables,saved[w,r]); lives.append(life)
                max_error=max(max_error,error)
                print('verified',w,r,flush=True)
    rebuilt=rebuild(lives)
    for key in ('tables','quantities'): max_error=max(max_error,compare(result[key],rebuilt[key],key))
    for k in ('H1','H2','H3','H4'): need(result['conditions'][k]==rebuilt['conditions'][k],k)
    material=all(rebuilt['conditions'].values())
    need(result['material_pass']==material,'material verdict')
    receipt=dict(verification_pass=True,material_pass=material,gate_pass=material,
        forecasts_checked=40*N*len(ARMS),source_counts_checked=sum(b['counters'] for b in books),
        max_error=max_error,conditions=rebuilt['conditions'],quantities=rebuilt['quantities'],tables=rebuilt['tables'],
        books=books,lives=lives,freeze_sha256=digest(HERE/'FREEZE.json'),result_sha256=digest(HERE/'RESULT.json'))
    with args.output.open('x') as f:
        json.dump(receipt,f,indent=2,sort_keys=True,allow_nan=False); f.write('\n')
    print(json.dumps({k:receipt[k] for k in ('verification_pass','material_pass','forecasts_checked','max_error','conditions')},sort_keys=True))


if __name__=='__main__': main()
