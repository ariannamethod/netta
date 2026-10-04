#!/usr/bin/env python3
"""Independent turn35 archive, selector, state, forecast and gate reader.

No turn35 writer or C implementation is imported.  The reader recounts the
four source lives, independently derives the density ordering, rebuilds all
five archives byte-for-byte, and replays every forecast with Decimal state.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NAMESPACE = 'netta-conditional-granularity-v1'
WORLDS = tuple(range(312, 320))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('dense10', 'sparse10', 'sel4', 'earned2', 'pooled', 'cold')
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024, EARLY, MOVE, N)
FINE = 10
TOL = 1e-7
RHO = Decimal(1) / 1024
PROTOCOL_SHA = '275bc7c2e565895625e26d3946932b764477bf3cf5d8307a46f28ba0878f7965'

spec = importlib.util.spec_from_file_location(
    'turn35_frozen_reader34', ROOT/'turns/turn34/verify.py')
p34 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p34
spec.loader.exec_module(p34)
p28 = p34.p28
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES
need, near, digest, compare = p28.need, p28.near, p28.digest, p28.compare
power2, mass_log2 = p28.power2, p28.mass_log2
SharedOuter, Permission, Wealth = p34.SharedOuter, p34.Permission, p34.Wealth
SelWealth = p34.SelWealth
distribution, normalized = p34.distribution, p34.normalized

COMMON = (
    't', 'k', 'heads', 'cold_heads', 'truth', 'rank', 'history', 'logcold',
    'matched', 'record', 'record_visits_before', 'dense_fine', 'sparse_fine',
    'source_pooled', 'source_e1', 'source_e2', 'source_e3', 'source_e4',
    'source_a2', 'source_b2', 'source_d1', 'source_d2', 'source_d3',
    'source_d4', 'source_s1', 'source_s2', 'source_s3', 'source_s4',
    'corrected_dense10', 'corrected_sparse10', 'corrected_sel4',
    'corrected_earned2', 'max_norm_error', 'new_exact',
    'admission_shadow_before', 'admitted_before', 'permission_u_before',
    'permission_u_after')
STATE_ORDER = tuple(
    f'{prefix}_h{i}_{stage}'
    for prefix in ('dense10', 'sparse10', 'sel4')
    for stage in ('before', 'after') for i in range(1, 5)) + (
    'earned2_ha_before', 'earned2_hb_before',
    'earned2_ha_after', 'earned2_hb_after')
OUTER_FIELDS = ('candidate', 'live', 'shadow_before', 'odds_before',
                'active_before', 'activated_after', 'gain_after')
FIELDS = COMMON + STATE_ORDER + tuple(
    arm+'_'+field for arm in ARMS for field in OUTER_FIELDS)


class HybridWealth:
    """Four slots when fine, two case slots and two exact zeros when coarse."""
    def __init__(self):
        self.h = [Decimal(0)]*4

    @staticmethod
    def excess(value):
        return max(power2(value)-1, Decimal(0))

    def silent(self, fine):
        return all(value <= 0 for value in self.h[:4 if fine else 2])

    def source(self, fine, pooled, parts):
        if self.silent(fine):
            return pooled
        count = 4 if fine else 2
        excess = [self.excess(value) for value in self.h[:count]]
        scale = max([pooled]+list(parts[:count]))
        mass = power2(pooled-scale) + sum(
            weight*power2(price-scale) for weight, price in zip(excess, parts[:count]))
        return scale + mass_log2(mass/(1+sum(excess)))

    def support(self, fine, pooled, parts):
        if self.silent(fine):
            return list(pooled)
        count = 4 if fine else 2
        excess = [self.excess(value) for value in self.h[:count]]
        denominator = 1+sum(excess)
        return [float((Decimal.from_float(p) + sum(
                    weight*Decimal.from_float(column[i])
                    for weight, column in zip(excess, parts[:count]))) / denominator)
                for i, p in enumerate(map(float, pooled))]

    def observe(self, fine, pooled, parts, matched=True):
        if not matched:
            return
        count = 4 if fine else 2
        for i in range(count):
            mass = (1-RHO)*power2(self.h[i])*power2(parts[i]-pooled) + RHO
            self.h[i] = Decimal.from_float(mass_log2(mass))
        for i in range(count, 4):
            self.h[i] = Decimal(0)
        need(all(value >= -10-Decimal('1e-10') for value in self.h[:count]),
             'hybrid wealth fixed-share floor')


def information_density(episodes):
    """Independent implementation of the frozen source-only score."""
    parents = [[x+y for x, y in zip(episodes[0], episodes[1])],
               [x+y for x, y in zip(episodes[2], episodes[3])]]
    gain = 0.0
    for episode, parent in zip(episodes, (parents[0], parents[0], parents[1], parents[1])):
        ne, np = sum(episode), sum(parent)
        for n, pn in zip(episode, parent):
            if n:
                gain += n*math.log2(((n+.5)/(ne+3.5))/((pn+.5)/(np+3.5)))
    return gain/(1+sum(map(sum, episodes)))


def verify_books(world):
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    need(meta.get('source_split') ==
         [['sourceAB'], ['sourceBA'], ['sourceCD'], ['sourceDC']],
         str(folder)+' fixed source-life split')
    need(meta.get('case_split') ==
         [['sourceAB','sourceBA'], ['sourceCD','sourceDC']],
         str(folder)+' fixed paired cases')
    pairs, expanded = p28.compose(meta['rules'], str(folder))
    selected = meta['selected']
    capacity = (528-16-4*len(pairs))//16
    need(isinstance(selected,list) and len(selected)==24 and len(selected)<=capacity,
         str(folder)+' full24 pooled selection')
    prefixes, owners = [], []
    for i, record in enumerate(selected):
        owner, length = record['rule_id'], record['prefix_len']
        need(type(owner) is int and 0<=owner<len(pairs) and type(length) is int and
             1<=length<len(expanded[7+owner]), str(folder)+f' addressing {i}')
        prefix = expanded[7+owner][:length]
        need(record['prefix']==list(prefix) and prefix not in prefixes,
             str(folder)+f' unique prefix {i}')
        prefixes.append(prefix); owners.append(owner)

    tapes = [p28.load_source(world,name) for name in p28.SOURCES]
    for tape, wanted in zip(tapes, meta['source_role_sha256']):
        need(hashlib.sha256(bytes(tape)).hexdigest()==wanted,
             str(folder)+' source-role identity')
    counts = p28.recount(tapes,prefixes)
    episodes, cases, pooled = [], [], []
    scores = []
    for i,(prefix,owner) in enumerate(zip(prefixes,owners)):
        ep = [[counts[life][prefix][rank] for rank in range(7)] for life in range(4)]
        a = [x+y for x,y in zip(ep[0],ep[1])]
        b = [x+y for x,y in zip(ep[2],ep[3])]
        total = [x+y for x,y in zip(a,b)]
        score = information_density(ep)
        need(selected[i]['episode_counts']==ep and selected[i]['book_counts']==[a,b] and
             selected[i]['counts']==total and selected[i]['total']==sum(total),
             str(folder)+f' recounted source counts {i}')
        near(selected[i]['information_density'],score,str(folder)+f' score {i}',1e-15)
        episodes.append((owner,prefix,ep)); cases.append((owner,prefix,[a,b]))
        pooled.append((owner,prefix,total)); scores.append(score)
    dense = sorted(range(24),key=lambda i:(-scores[i],i))[:FINE]
    sparse = sorted(range(24),key=lambda i:(scores[i],i))[:FINE]
    need(meta['dense_indices']==dense and meta['sparse_indices']==sparse and
         meta['fine_records']==FINE, str(folder)+' independent source-only ordering')

    def encode4():
        data=struct.pack('<8sII',b'NETEB004',len(pairs),24)
        data+=b''.join(struct.pack('<HH',*pair) for pair in pairs)
        for owner,prefix,ep in episodes:
            data+=struct.pack('<BBH28H',owner,len(prefix),0,*sum(ep,[]))
        return data

    def encode_hybrid(indices):
        chosen=set(indices)
        data=struct.pack('<8sII',b'NETEH010',len(pairs),24)
        data+=b''.join(struct.pack('<HH',*pair) for pair in pairs)
        for i,(owner,prefix,books) in enumerate(cases):
            data+=struct.pack('<BBH14H',owner,len(prefix),int(i in chosen),*sum(books,[]))
        for i,(_,_,ep) in enumerate(episodes):
            if i in chosen: data+=struct.pack('<14H',*(ep[0]+ep[2]))
        return data

    expected = {
        'dense10.bin':encode_hybrid(dense), 'sparse10.bin':encode_hybrid(sparse),
        'bank4_full.bin':encode4(), 'bank2_full.bin':p28.encode(pairs,cases,True),
        'pooled_full.bin':p28.encode(pairs,pooled)}
    for name,data in expected.items():
        need((folder/name).read_bytes()==data and meta['bytes'][name]==len(data),
             str(folder/name)+' independently rebuilt bytes')
    hbytes=len(expected['dense10.bin'])-len(expected['bank2_full.bin'])
    fbytes=len(expected['bank4_full.bin'])-len(expected['bank2_full.bin'])
    need(hbytes==meta['added_hybrid_bytes']==280 and
         fbytes==meta['added_full_bytes']==672 and
         len(expected['dense10.bin'])==len(expected['sparse10.bin']),
         str(folder)+' exact conditional-memory price')
    tables = {
        'episodes':{prefix:(i,rows) for i,(_,prefix,rows) in enumerate(episodes)},
        'cases':{prefix:(i,rows) for i,(_,prefix,rows) in enumerate(cases)},
        'pooled':{prefix:(i,rows) for i,(_,prefix,rows) in enumerate(pooled)},
        'dense':set(dense), 'sparse':set(sparse)}
    receipt = dict(world=world, source_bytes=4*N, source_lives=4,
        rules=len(pairs), records=24, counts_checked=24*7,
        episode_counts_checked=24*28, score_values_checked=24,
        dense_indices=dense, sparse_indices=sparse,
        added_hybrid_bytes=hbytes, added_full_bytes=fbytes,
        bytes={name:len(data) for name,data in expected.items()},
        hashes={name:digest(folder/name) for name in expected},
        source_role_histograms=[dict(sorted(Counter(tape).items())) for tape in tapes])
    return tables,receipt


def check_life(world,regime,tables,saved):
    path=HERE/'results'/f'world{world}'/(regime+'.turn35.tsv.gz')
    raw=(HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw)==N,str(path)+' raw horizon')
    if regime in ('partial','switched','moved_mid'):
        need(raw[:MOVE]==(HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()[:MOVE],
             str(path)+' shared raw prefix')
    nrecords=len(tables['episodes'])
    dense=[HybridWealth() for _ in range(nrecords)]
    sparse=[HybridWealth() for _ in range(nrecords)]
    full=[SelWealth() for _ in range(nrecords)]
    coarse=[Wealth() for _ in range(nrecords)]
    permissions=[Permission() for _ in range(nrecords)]
    visits=[0]*nrecords
    states={arm:SharedOuter() for arm in ARMS}
    history=[]; admitted=False; admission_shadow=0.0
    max_error=max_norm=0.0
    residual={arm:dict(matched=0,first=0,silent=0,active=0,became_active=0,fell_silent=0)
              for arm in ('dense10','sparse10','sel4','earned2')}
    fine_visits=dict(dense10=0,sparse10=0)
    best=worst=selector_help=selector_harm=None
    exact=dict(new=True,equal=True,inactive=True,history=True,
               shared_admission=True,router_state=True,first=True,
               dense10_silent=True,sparse10_silent=True,
               sel4_silent=True,earned2_silent=True,source_tags=True)

    def check(actual,expected,label,tolerance=TOL):
        nonlocal max_error
        max_error=max(max_error,near(actual,expected,label,tolerance))

    with gzip.open(path,'rt') as stream:
        rows=csv.DictReader(stream,delimiter='\t')
        need(tuple(rows.fieldnames or ())==FIELDS,str(path)+' exact trace header')
        for t in range(N):
            row=next(rows,None); label=f'{path}:{t}'
            need(row is not None and int(row['t'])==t and int(row['truth'])==raw[t],
                 label+' chronology')
            k,heads=int(row['k']),p28.ints(row['heads'])
            rank=p28.bindings(k,heads,raw[t],label)
            need(int(row['rank'])==rank,label+' HEAD256 rank')
            need(row['history']==(''.join(map(str,history)) or '-'),label+' history')
            cold=float(row['logcold']); head_logs=p28.floats(row['cold_heads'])
            need(math.isfinite(cold) and cold<=0 and len(head_logs)==k and
                 all(math.isfinite(v) and v<=0 for v in head_logs),label+' supplied P0')
            if rank: check(cold,head_logs[rank-1],label+'/truth-head',1e-10)
            else: need(math.exp2(cold)<=1-math.fsum(math.exp2(v) for v in head_logs)+1e-12,
                       label+' NEW residual')
            need(int(row['new_exact'])==1,label+' protected NEW')
            norm=float(row['max_norm_error'])
            need(math.isfinite(norm) and 0<=norm<=1e-8,label+' vector normalization')
            max_norm=max(max_norm,norm)

            em=p28.suffix_match(tables['episodes'],history)
            cm=p28.suffix_match(tables['cases'],history)
            pm=p28.suffix_match(tables['pooled'],history)
            need(em[:2]==cm[:2]==pm[:2],label+' shared selected address')
            length,record,epbooks=em; casebooks=cm[2]; pooled_counts=pm[2]
            need(int(row['matched'])==length and int(row['record'])==record,
                 label+' longest address')
            df=bool(length and record in tables['dense'])
            sf=bool(length and record in tables['sparse'])
            need(int(row['dense_fine'])==df and int(row['sparse_fine'])==sf,
                 label+' source-only fine tags')
            ecounts=epbooks if length else [None]*4
            acounts,bcounts=casebooks if length else (None,None)
            zeros=[0]*7 if length else None
            dcounts=ecounts if df else [acounts,bcounts,zeros,zeros]
            scounts=ecounts if sf else [acounts,bcounts,zeros,zeros]
            all_counts=[pooled_counts]+list(ecounts)+[acounts,bcounts]+list(dcounts)+list(scounts)
            prices=[p28.source_candidate(cold,head_logs,rank,c) for c in all_counts]
            price_fields=('source_pooled','source_e1','source_e2','source_e3','source_e4',
                          'source_a2','source_b2','source_d1','source_d2','source_d3','source_d4',
                          'source_s1','source_s2','source_s3','source_s4')
            for field,value in zip(price_fields,prices):
                check(row[field],value,label+'/'+field)
                if not rank or not length: need(float(row[field])==cold,label+' exact fallback '+field)
            pool_price=prices[0]; eprices=prices[1:5]; aprice,bprice=prices[5:7]
            dprices=prices[7:11]; sprices=prices[11:15]

            local_support,pooled_support=distribution(head_logs,pooled_counts)
            esupports=[distribution(head_logs,c)[1] for c in ecounts]
            _,asupport=distribution(head_logs,acounts); _,bsupport=distribution(head_logs,bcounts)
            dsupports=[distribution(head_logs,c)[1] for c in dcounts]
            ssupports=[distribution(head_logs,c)[1] for c in scounts]
            residual_mass=1-math.fsum(local_support)
            dh=dense[record] if length else HybridWealth()
            sh=sparse[record] if length else HybridWealth()
            fh=full[record] if length else SelWealth()
            cw=coarse[record] if length else Wealth()
            permission=permissions[record] if length else Permission()
            before_visits=visits[record] if length else 0
            need(int(row['record_visits_before'])==before_visits,label+' visits')

            def state_fields(stage):
                check(row['permission_u_'+stage],permission.u,label+'/permission/'+stage)
                for prefix,state in (('dense10',dh),('sparse10',sh),('sel4',fh)):
                    for i in range(4): check(row[f'{prefix}_h{i+1}_{stage}'],state.h[i],
                                              label+f'/{prefix}/h{i+1}/'+stage)
                check(row['earned2_ha_'+stage],cw.a,label+'/earned2/a/'+stage)
                check(row['earned2_hb_'+stage],cw.b,label+'/earned2/b/'+stage)

            state_fields('before')
            corrected=dict(
                dense10=dh.source(df,pool_price,dprices),
                sparse10=sh.source(sf,pool_price,sprices),
                sel4=fh.source(pool_price,eprices),
                earned2=cw.source(pool_price,aprice,bprice))
            for arm,value in corrected.items(): check(row['corrected_'+arm],value,label+'/corrected/'+arm)
            candidates={arm:permission.quote(cold,value) for arm,value in corrected.items()}
            candidates['pooled']=permission.quote(cold,pool_price); candidates['cold']=cold

            source_supports=dict(
                dense10=dh.support(df,pooled_support,dsupports),
                sparse10=sh.support(sf,pooled_support,ssupports),
                sel4=fh.support(pooled_support,esupports),
                earned2=cw.support(pooled_support,asupport,bsupport),
                pooled=pooled_support,cold=local_support)
            u=float(permission.u)
            candidate_supports={arm:(local_support if arm=='cold' else
                [(1-u)*x+u*y for x,y in zip(local_support,support)])
                for arm,support in source_supports.items()}
            for arm,support in candidate_supports.items():
                normalized(label+'/'+arm+'/candidate',residual_mass,support)
                if states[arm].active and candidates[arm]!=cold:
                    sw,cwgt=float(states[arm].source),float(states[arm].cold)
                    live_support=[cwgt*x+sw*y for x,y in zip(local_support,support)]
                else: live_support=local_support
                normalized(label+'/'+arm+'/live',residual_mass,live_support)

            if length:
                if df: fine_visits['dense10']+=1
                if sf: fine_visits['sparse10']+=1
                for arm,state,fine in (('dense10',dh,df),('sparse10',sh,sf),('sel4',fh,True)):
                    before=[float(row[f'{arm}_h{i}_before']) for i in range(1,5 if fine else 3)]
                    after=[float(row[f'{arm}_h{i}_after']) for i in range(1,5 if fine else 3)]
                    silent=max(before)<=0; after_silent=max(after)<=0; r=residual[arm]; r['matched']+=1
                    if before_visits==0:
                        r['first']+=1; exact['first'] &= float(row[arm+'_candidate'])==float(row['pooled_candidate'])
                    if silent:
                        r['silent']+=1; exact[arm+'_silent'] &= float(row[arm+'_candidate'])==float(row['pooled_candidate'])
                    else: r['active']+=1
                    r['became_active']+=silent and not after_silent; r['fell_silent']+=not silent and after_silent
                before=[float(row['earned2_ha_before']),float(row['earned2_hb_before'])]
                after=[float(row['earned2_ha_after']),float(row['earned2_hb_after'])]
                silent=max(before)<=0; after_silent=max(after)<=0; r=residual['earned2']; r['matched']+=1
                if before_visits==0:
                    r['first']+=1; exact['first'] &= float(row['earned2_candidate'])==float(row['pooled_candidate'])
                if silent:
                    r['silent']+=1; exact['earned2_silent'] &= float(row['earned2_candidate'])==float(row['pooled_candidate'])
                else:r['active']+=1
                r['became_active']+=silent and not after_silent; r['fell_silent']+=not silent and after_silent
            if not df: exact['router_state'] &= dh.h[2:]==[0,0]
            if not sf: exact['router_state'] &= sh.h[2:]==[0,0]

            need(int(row['admitted_before'])==int(admitted),label+' admission state')
            check(row['admission_shadow_before'],admission_shadow,label+'/admission shadow')
            admission_shadow+=candidates['pooled']-cold
            admit=not admitted and admission_shadow>=32
            if admit: admitted=True
            wanted={arm:states[arm].step(t,cold,candidates[arm],admit) for arm in ARMS}
            for arm,fields in wanted.items():
                for field,value in fields.items():
                    actual=row[arm+'_'+field]
                    if type(value) is int: need(int(actual)==value,label+'/'+arm+'/'+field)
                    else: check(actual,value,label+'/'+arm+'/'+field)
                if not rank: exact['new'] &= float(row[arm+'_candidate'])==cold and float(row[arm+'_live'])==cold
                if float(row[arm+'_candidate'])==cold: exact['equal'] &= float(row[arm+'_live'])==cold
                if not fields['active_before']: exact['inactive'] &= float(row[arm+'_live'])==cold
            exact['shared_admission'] &= (len({v['active_before'] for v in wanted.values()})==1 and
                                           len({v['activated_after'] for v in wanted.values()})==1)

            if length:
                dh.observe(df,pool_price,dprices); sh.observe(sf,pool_price,sprices)
                fh.observe(pool_price,eprices); cw.observe(pool_price,aprice,bprice)
                permission.observe(pool_price,candidates['pooled']); visits[record]+=1
            state_fields('after')
            d=float(row['dense10_live'])-float(row['earned2_live'])
            s=float(row['dense10_live'])-float(row['sparse10_live'])
            sample=dict(row,dense_over_coarse=d,dense_over_sparse=s,
                        raw_hex=raw[max(0,t-16):t+17].hex())
            if t>=MOVE:
                if best is None or d>best['dense_over_coarse']:best=sample
                if worst is None or d<worst['dense_over_coarse']:worst=sample
                if selector_help is None or s>selector_help['dense_over_sparse']:selector_help=sample
                if selector_harm is None or s<selector_harm['dense_over_sparse']:selector_harm=sample
            history=(history+[rank])[-32:]
        need(next(rows,None) is None,str(path)+' excess rows')
    exact['router_state'] &= True
    life=dict(world=world,regime=regime,arms={arm:states[arm].result() for arm in ARMS},
              max_norm_error=max_norm,exactness=exact,residual=residual,
              fine_visits=fine_visits,raw_help=best,raw_harm=worst,
              selector_help=selector_help,selector_harm=selector_harm)
    max_error=max(max_error,compare(saved,life,f'RESULT/{world}/{regime}'))
    return life,N*len(ARMS),max_error


def mean(values): return math.fsum(values)/len(values)


def rebuild(lives,books):
    lookup={(life['world'],life['regime']):life for life in lives}
    values=lambda regime,arm,key:[lookup[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs=lambda regime,left,right,key:[a-b for a,b in zip(values(regime,left,key),values(regime,right,key))]
    tables={r:{a:{k:mean(values(r,a,k)) for k in ('early','gain','tail')} for a in ARMS} for r in REGIMES}
    comparisons={}
    for r in REGIMES:
        comparisons[r]={}
        for left,right in (('dense10','earned2'),('dense10','sparse10'),('sel4','earned2'),('dense10','pooled')):
            name=left+'_minus_'+right; comparisons[r][name]={}
            for key in ('early','gain','tail'):
                delta=diffs(r,left,right,key)
                comparisons[r][name][key]=dict(mean=mean(delta),wins=sum(x>0 for x in delta),per_world=delta)
    hbytes=sorted({b['added_hybrid_bytes'] for b in books}); fbytes=sorted({b['added_full_bytes'] for b in books})
    need(hbytes==[280] and fbytes==[672],'uniform archive prices')
    m10,m24=.01*hbytes[0],.01*fbytes[0]
    portable={arm:sorted({0 if arm=='cold' else b['bytes'][{
        'dense10':'dense10.bin','sparse10':'sparse10.bin','sel4':'bank4_full.bin',
        'earned2':'bank2_full.bin','pooled':'pooled_full.bin'}[arm]] for b in books}) for arm in ARMS}
    recipient={arm:sorted({b['records']*{'dense10':40,'sparse10':40,'sel4':40,
        'earned2':24,'pooled':8,'cold':0}[arm] for b in books}) for arm in ARMS}
    prefix_identity=all(lookup[w,r]['arms'][a]['horizons'][str(MOVE)]==
        lookup[w,'recombined']['arms'][a]['horizons'][str(MOVE)]
        for w in WORLDS for r in ('partial','switched','moved_mid') for a in ARMS)
    admission_identity=all(len({s['activation'] for s in life['arms'].values()})==1 for life in lives)
    d=comparisons['switched']['dense10_minus_earned2']; s=comparisons['switched']['dense10_minus_sparse10']
    f=comparisons['switched']['sel4_minus_earned2']; partial_d=comparisons['partial']['dense10_minus_earned2']['tail']
    dense_positive=sum(x>0 for x in values('recombined','dense10','gain'))
    retention={k:tables['recombined']['dense10'][k]/max(tables['recombined']['earned2'][k],1e-300)
               for k in ('early','gain')}
    quantities=dict(comparisons=comparisons,added_hybrid_bytes=hbytes,added_full_bytes=fbytes,
        priced_margin_m10_bits=m10,priced_margin_m24_bits=m24,fine_fraction=FINE/24,
        retention=retention,portable_bytes=portable,recipient_router_bytes=recipient,
        shared_prefix_identity=prefix_identity,recombined_dense_positive=dense_positive,
        fine_visits={r:{a:sum(lookup[w,r]['fine_visits'][a] for w in WORLDS)
                        for a in ('dense10','sparse10')} for r in REGIMES},
        unrelated_admissions=[dict(world=w,arm=a,activation=lookup[w,'unrelated']['arms'][a]['activation'],
                                   gain=lookup[w,'unrelated']['arms'][a]['gain'])
                              for w in WORLDS for a in ARMS
                              if lookup[w,'unrelated']['arms'][a]['activation'] is not None])
    validity=dict(archive=all(b['bytes']['pooled_full.bin']<=528 and b['bytes']['bank2_full.bin']<=1056 and
        b['bytes']['bank4_full.bin']<=2112 and b['bytes']['dense10.bin']==b['bytes']['sparse10.bin']
        for b in books),source_counts=True,source_only_selection=True,
        normalization=all(l['max_norm_error']<=1e-8 for l in lives),
        exact_quotes=all(all(l['exactness'].values()) for l in lives),
        bounds=all(st['minimum']>=-1-TOL and st['drawdown']<=16+TOL for l in lives for st in l['arms'].values()),
        identical_admissions=admission_identity,shared_prefix=prefix_identity,unrelated_disclosed=True)
    conditions=dict(G1=d['tail']['mean']>m10 and d['tail']['wins']>=5 and d['gain']['mean']>=0,
        G2=s['tail']['mean']>0 and s['tail']['wins']>=5 and portable['dense10']==portable['sparse10'],
        G3=f['tail']['mean']>0 and d['tail']['mean']>=.5*f['tail']['mean'] and
           hbytes[0]/fbytes[0]<=FINE/24+1e-15,
        G4=dense_positive==8 and min(retention.values())>=.99 and partial_d['mean']>=0,
        G5=all(validity.values()))
    material=all(conditions.values())
    return dict(tables=tables,quantities=quantities,validity=validity,
                conditions=conditions,material_pass=material,gate_pass=material)


def check_schema(result):
    for key in ('namespace','worlds','regimes','arms','protocol_sha256','lives','tables',
                'quantities','conditions','validity','material_pass','independent_reader_pending','gate_pass'):
        need(key in result,'RESULT missing '+key)
    need(set(result['conditions'])=={'G1','G2','G3','G4','G5'},'condition keys')
    need(set(result['validity'])=={'archive','source_counts','source_only_selection','normalization',
        'exact_quotes','bounds','identical_admissions','shared_prefix','unrelated_disclosed'},'validity keys')


def identity(result):
    frozen=json.loads((HERE/'FREEZE.json').read_text())
    for key,wanted in dict(namespace=NAMESPACE,worlds=list(WORLDS),regimes=list(REGIMES),arms=list(ARMS)).items():
        need(result[key]==wanted and frozen[key]==wanted,'identity '+key)
    need(frozen['fine_records']==FINE,'identity fine count')
    p28.check_artifact(HERE/'PROTOCOL.md',PROTOCOL_SHA)
    need(result['protocol_sha256']==PROTOCOL_SHA,'RESULT protocol identity')
    need(result['independent_reader_pending'] is True and result['conditions']['G5'] is False and
         result['gate_pass'] is False,'writer cannot award reader gate')
    required=('turns/turn35/verify.py','turns/turn35/hybrid_router.c','turns/turn35/PROTOCOL.md',
              'turns/turn34/verify.py','turns/turn34/sel4_router.c','turns/turn33/verify.py',
              'turns/turn30/verify.py','turns/turn28/verify.py')
    need(all(name in frozen['files'] for name in required),'frozen reader dependencies')
    for name,wanted in frozen['files'].items():p28.check_artifact(ROOT/name,wanted)
    counts={}
    for name in ('DATA_MANIFEST.json','EXTRACT_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        manifest=json.loads((HERE/name).read_text())
        for relative,wanted in manifest.items():p28.check_artifact(HERE/relative,wanted)
        counts[name]=len(manifest)
    return dict(reader_sha256=digest(Path(__file__)),freeze_sha256=digest(HERE/'FREEZE.json'),
        result_sha256=digest(HERE/'RESULT.json'),freeze_files=len(frozen['files']),
        manifest_entries=counts,protocol_sha256=PROTOCOL_SHA)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json'); args=parser.parse_args()
    need(not args.output.exists(),'new output path '+str(args.output))
    result=json.loads((HERE/'RESULT.json').read_text()); check_schema(result); pins=identity(result)
    recorded={(life['world'],life['regime']):life for life in result['lives']}
    need(len(recorded)==len(result['lives']) and set(recorded)==
         {(w,r) for w in WORLDS for r in REGIMES},'complete unique life grid')
    lives=[]; books=[]; count=0; max_error=0.0
    with localcontext() as context:
        context.prec=50
        for world in WORLDS:
            tables,source=verify_books(world); books.append(source)
            for regime in REGIMES:
                life,forecasts,error=check_life(world,regime,tables,recorded[world,regime])
                lives.append(life); count+=forecasts; max_error=max(max_error,error)
                print('verified',world,regime,flush=True)
    need(count==len(WORLDS)*len(REGIMES)*N*len(ARMS),'complete forecast count')
    rebuilt=rebuild(lives,books)
    for key in ('tables','quantities','validity','material_pass'):
        max_error=max(max_error,compare(result[key],rebuilt[key],'RESULT/'+key))
    for key in ('G1','G2','G3','G4'):
        need(result['conditions'][key]==rebuilt['conditions'][key],'RESULT/conditions/'+key)
    need(rebuilt['conditions']['G5'],'independent reconstruction validity')
    receipt=dict(verification_pass=True,material_pass=rebuilt['material_pass'],
        gate_pass=rebuilt['gate_pass'],forecasts_checked=count,max_error=max_error,
        source_counts_checked=sum(b['episode_counts_checked'] for b in books),
        score_values_checked=sum(b['score_values_checked'] for b in books),
        conditions=rebuilt['conditions'],quantities=rebuilt['quantities'],
        validity=rebuilt['validity'],tables=rebuilt['tables'],pins=pins,
        source_books=books,lives=lives,
        scope='Independent source recount, source-only score/order, five byte-exact archives; '
              'conditional two/four-state wealth, pooled permission, common admission, all outer '
              'trajectories, metrics and gates. Inherited address selection and HEAD256 P0/bindings '
              'remain supplied pinned inputs.')
    with args.output.open('x') as stream:
        json.dump(receipt,stream,indent=2,sort_keys=True,allow_nan=False);stream.write('\n')
    print(json.dumps({k:receipt[k] for k in ('verification_pass','material_pass','gate_pass',
        'forecasts_checked','max_error','source_counts_checked','score_values_checked','conditions')},sort_keys=True))


if __name__=='__main__':main()
