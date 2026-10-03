#!/usr/bin/env python3
"""One frozen trial of conditional case evidence independent of permission."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(288,296))
REGIMES = ('recombined','partial','switched','moved_mid','unrelated')
ARMS = ('factored','flat3','balanced','pooled','permuted','cold')
NAMESPACE = 'netta-conditional-case-v1'
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024,EARLY,MOVE,N)
spec = importlib.util.spec_from_file_location('turn31_source28', ROOT/'turns/turn28/experiment.py')
p28 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p28
spec.loader.exec_module(p28)
t13,p22 = p28.t13,p28.parent
p28.HERE,p28.ROOT,p28.WORLDS,p28.REGIMES,p28.NAMESPACE = HERE,ROOT,WORLDS,REGIMES,NAMESPACE
p22.HERE,p22.REPO,p22.WORLDS,p22.REGIMES,p22.NAMESPACE = HERE,ROOT,WORLDS,REGIMES,NAMESPACE
t13.HERE,t13.REPO,t13.WORLDS,t13.REGIMES,t13.NAMESPACE = HERE,ROOT,WORLDS,('recombined','unrelated','switched'),NAMESPACE


def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for b in iter(lambda:f.read(1<<20),b''): h.update(b)
    return h.hexdigest()


def save(path,value):
    path=Path(path)
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('x') as f:
        json.dump(value,f,indent=2,sort_keys=True,allow_nan=False)
        f.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)):sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifest(name):
    for rel,wanted in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/rel)==wanted,str(HERE/rel)


def frozen_names():
    own=('PROTOCOL.md','INTERFACE.md','experiment.py','verify.py','fixture_check.py','case_router.c',
         'case_router','episode','Makefile')
    inherited=('turns/turn30/verify.py','turns/turn30/router3.c',
        'turns/turn29/calibrate.c','turns/turn28/experiment.py','turns/turn28/verify.py',
        'turns/turn13/experiment.py','turns/turn13/episode.c','turns/turn22/experiment.py',
        'byte_recurrence/frontend.c','byte_recurrence/frontend.h',
        'portable_recurrence/recurrence.c','portable_recurrence/recurrence.h',
        'court4/transfer4_confirm_core.c')
    return ['turns/turn31/'+p for p in own]+list(inherited)


def freeze():
    for name in ('data','memory','results'): assert not (HERE/name).exists(),name
    save(HERE/'FREEZE.json',dict(namespace=NAMESPACE,worlds=WORLDS,regimes=REGIMES,
        arms=ARMS,files={p:sha(ROOT/p) for p in frozen_names()}))


def check_freeze():
    frozen=json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace']==NAMESPACE and frozen['worlds']==list(WORLDS)
    assert frozen['regimes']==list(REGIMES) and frozen['arms']==list(ARMS)
    for name,wanted in frozen['files'].items(): assert sha(ROOT/name)==wanted,name


def generate():
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(p28.generate_world,WORLDS))
    save(HERE/'DATA_MANIFEST.json',manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(t13.extract_world,WORLDS))
    save(HERE/'EXTRACT_MANIFEST.json',manifest(HERE/'data'))


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for w in WORLDS:
        tapes,_=t13.source_tapes(w)
        archives,meta=p28.build_memory(tapes)
        d=HERE/'memory'/f'world{w}'
        d.mkdir(parents=True,exist_ok=False)
        for name,data in archives.items():
            with (d/name).open('xb') as f: f.write(data)
        save(d/'BOOKS.json',meta)
        print('learned',w,flush=True)
    save(HERE/'MEMORY_MANIFEST.json',manifest(HERE/'memory'))


def run_target(pair):
    w,regime=pair
    d=HERE/'results'/f'world{w}'
    d.mkdir(parents=True,exist_ok=True)
    m=HERE/'memory'/f'world{w}'
    command=[str(HERE/'case_router'),'predict']+[str(m/p) for p in
             ('bank.bin','pooled_small.bin','permuted.bin')]
    outpath=d/(regime+'.turn31.tsv.gz')
    with (HERE/'data'/f'world{w}'/(regime+'.bin')).open('rb') as source, gzip.open(outpath,'xb') as out:
        proc=subprocess.Popen(command,stdin=source,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        while chunk:=proc.stdout.read(1<<20): out.write(chunk)
        stderr=proc.stderr.read()
        rc=proc.wait()
    with outpath.with_suffix('.stderr').open('xb') as f: f.write(stderr)
    assert rc==0,(command,rc,stderr.decode())
    return summarize(w,regime)


def blank():
    return dict(gain=0.,early=0.,tail=0.,minimum=0.,peak=0.,drawdown=0.,activation=None,horizons={})


def summarize(w,regime):
    stats={a:blank() for a in ARMS}
    exact=dict(new=True,equal=True,inactive=True,history=True,shared_admission=True,router_weights=True)
    raw=(HERE/'data'/f'world{w}'/(regime+'.bin')).read_bytes()
    assert len(raw)==N
    if regime in ('partial','switched','moved_mid'):
        assert raw[:MOVE]==(HERE/'data'/f'world{w}'/'recombined.bin').read_bytes()[:MOVE]
    history=[]
    max_norm=0.
    best=worst=None
    with gzip.open(HERE/'results'/f'world{w}'/(regime+'.turn31.tsv.gz'),'rt') as f:
        rows=csv.DictReader(f,delimiter='\t')
        for t in range(N):
            row=next(rows,None)
            assert row is not None and int(row['t'])==t and int(row['truth'])==raw[t]
            exact['history'] &= row['history']==(''.join(map(str,history)) or '-')
            rank=int(row['rank']); cold=float(row['logcold'])
            max_norm=max(max_norm,float(row['max_norm_error']))
            for field in ('flat3_w_before','flat3_w_after'):
                ws=list(map(float,row[field].split(',')))
                exact['router_weights'] &= len(ws)==3 and all(0<x<1 for x in ws) and abs(math.fsum(ws)-1)<=1e-10
            for arm,letters in (('factored','uv'),('permuted','uv'),('balanced','u'),('pooled','u')):
                for letter in letters:
                    for side in ('before','after'):
                        exact['router_weights'] &= 0<float(row[f'{arm}_{letter}_{side}'])<1
            active={int(row[a+'_active_before']) for a in ARMS}
            fired={int(row[a+'_activated_after']) for a in ARMS}
            exact['shared_admission'] &= len(active)==1 and len(fired)==1 and active=={int(row['admitted_before'])}
            for a in ARMS:
                cand,live=float(row[a+'_candidate']),float(row[a+'_live'])
                s=stats[a]
                s['gain']+=live-cold
                s['minimum']=min(s['minimum'],s['gain'])
                s['peak']=max(s['peak'],s['gain'])
                s['drawdown']=max(s['drawdown'],s['peak']-s['gain'])
                assert abs(s['gain']-float(row[a+'_gain_after']))<=1e-7
                if int(row[a+'_activated_after']):
                    assert s['activation'] is None
                    s['activation']=t+1
                if t+1 in HORIZONS: s['horizons'][str(t+1)]=s['gain']
                if not rank: exact['new'] &= cand==cold and live==cold
                if cand==cold: exact['equal'] &= live==cold
                if not int(row[a+'_active_before']): exact['inactive'] &= live==cold
            delta=float(row['factored_live'])-float(row['flat3_live'])
            sample=dict(row,difference=delta,raw_hex=raw[max(0,t-16):t+17].hex())
            if best is None or delta>best['difference']: best=sample
            if worst is None or delta<worst['difference']: worst=sample
            history=(history+[rank])[-32:]
        assert next(rows,None) is None
    for s in stats.values():
        s['early']=s['horizons'][str(EARLY)]
        s['tail']=s['gain']-s['horizons'][str(MOVE)]
    return dict(world=w,regime=regime,arms=stats,max_norm_error=max_norm,
                exactness=exact,raw_help=best,raw_harm=worst)


def mean(xs): return math.fsum(xs)/len(xs)


def verdict(lives):
    lookup={(x['world'],x['regime']):x for x in lives}
    vals=lambda r,a,k:[lookup[w,r]['arms'][a][k] for w in WORLDS]
    diffs=lambda r,a,b,k:[x-y for x,y in zip(vals(r,a,k),vals(r,b,k))]
    tables={r:{a:{k:mean(vals(r,a,k)) for k in ('early','gain','tail')} for a in ARMS} for r in REGIMES}
    comparisons={}
    for r in REGIMES:
        comparisons[r]={}
        for b in ('flat3','balanced','pooled'):
            comparisons[r][b]={}
            for k in ('early','gain','tail'):
                ds=diffs(r,'factored',b,k)
                comparisons[r][b][k]=dict(mean=mean(ds),wins=sum(d>0 for d in ds),per_world=ds)
    retention={name:tables[r]['factored'][k]/max(tables[r]['pooled'][k],1e-300)
               for name,r,k in (('recombined_early','recombined','early'),('recombined_full','recombined','gain'),('partial_full','partial','gain'))}
    null_excess={r:tables[r]['permuted']['gain']-tables[r]['factored']['gain'] for r in REGIMES}
    flat_balanced={r:{k:mean(diffs(r,'flat3','balanced',k)) for k in ('early','gain','tail')} for r in REGIMES}
    portable={a:[] for a in ARMS}
    recipient={a:[] for a in ARMS}
    for w in WORLDS:
        meta=json.loads((HERE/'memory'/f'world{w}'/'BOOKS.json').read_text())
        n=len(meta['bank_selected'])
        for a in ARMS:
            byte=0 if a=='cold' else meta['bytes']['pooled_small.bin' if a=='pooled' else 'bank.bin']
            portable[a].append(byte)
            recipient[a].append(n*{'factored':16,'flat3':24,'balanced':8,'pooled':8,'permuted':16,'cold':0}[a])
    portable={a:sorted(set(xs)) for a,xs in portable.items()}
    recipient={a:sorted(set(xs)) for a,xs in recipient.items()}
    admission_identity=all(len({s['activation'] for s in x['arms'].values()})==1 for x in lives)
    prefix_identity=all(lookup[w,r]['arms'][a]['horizons'][str(MOVE)]==lookup[w,'recombined']['arms'][a]['horizons'][str(MOVE)]
       for w in WORLDS for r in ('partial','switched','moved_mid') for a in ARMS)
    unrelated=[dict(world=w,arm=a,activation=lookup[w,'unrelated']['arms'][a]['activation'],gain=lookup[w,'unrelated']['arms'][a]['gain'])
       for w in WORLDS for a in ARMS if lookup[w,'unrelated']['arms'][a]['activation'] is not None]
    q=dict(comparisons=comparisons,retention=retention,permuted_excess=null_excess,
           flat_minus_balanced=flat_balanced,portable_bytes=portable,recipient_router_bytes=recipient,
           shared_prefix_identity=prefix_identity,unrelated_admissions=unrelated,
           recombined_positive=sum(v>0 for v in vals('recombined','factored','gain')))
    validity=dict(archive=all(p.stat().st_size<=528 for p in (HERE/'memory').rglob('*.bin')),
        source_counts=True,normalization=all(x['max_norm_error']<=1e-8 for x in lives),
        exact_quotes=all(all(x['exactness'].values()) for x in lives),
        bounds=all(s['minimum']>=-1-1e-7 and s['drawdown']<=16+1e-7 for x in lives for s in x['arms'].values()),
        identical_admissions=admission_identity,shared_prefix=prefix_identity,unrelated_disclosed=True)
    cond=dict(F1=all(comparisons['partial'][b]['tail']['mean']>1 and comparisons['partial'][b]['tail']['wins']>=5 for b in ('flat3','balanced')),
        F2=all(tables[r]['pooled'][k]>0 for r,k in (('recombined','early'),('recombined','gain'),('partial','gain'))) and min(retention.values())>=.95 and q['recombined_positive']==8,
        F3=all(v<0 for v in null_excess.values()),F4=False)
    material=all(cond[k] for k in ('F1','F2','F3')) and all(validity.values())
    return dict(namespace=NAMESPACE,worlds=WORLDS,regimes=REGIMES,arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'),lives=lives,tables=tables,quantities=q,
        conditions=cond,validity=validity,material_pass=material,gate_pass=False,independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json'); check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives=list(pool.map(run_target,[(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json',manifest(HERE/'results'))
    result=verdict(lives)
    save(HERE/'RESULT.json',result)
    print(json.dumps({k:result[k] for k in ('conditions','validity','material_pass','gate_pass','tables')},indent=2,sort_keys=True))


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('stage',choices=('freeze','generate','extract','learn','evaluate'))
    args=parser.parse_args()
    if args.stage!='freeze': check_freeze()
    globals()[args.stage]()
