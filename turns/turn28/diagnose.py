#!/usr/bin/env python3
"""Post-hoc additive accounting of saved turn28 prices, without repricing."""
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
WINDOWS = {'1024':(0,1024), '4096':(0,4096), '8192':(0,8192),
           '16384':(0,16384), 'tail':(8192,16384)}
CLASSES = ('bank_missing','full_longer','same','neither')
FIELDS = ('local_small','small_full','local_full','local_global',
          'candidate_local_small','candidate_small_full','candidate_local_full')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def empty():
    return dict(count=0, **{key:0.0 for key in FIELDS})


def add(cell, changes):
    cell['count'] += 1
    for key,value in changes.items():
        cell[key] += value


def row_example(world, regime, row, books):
    record = int(row['record'])
    full_record = int(row['full_record'])
    wanted = ('t','truth','rank','history','k','heads','matched','record',
              'full_matched','full_record','logcold','bank_a','bank_b',
              'local_w_before','global_w_before')
    x = dict(world=world, regime=regime, **{key:row[key] for key in wanted})
    for arm in ('local','global','pooled_full','pooled_small'):
        for suffix in ('candidate','live','odds_before','active_before'):
            x[arm+'_'+suffix] = row[arm+'_'+suffix]
    x['book_record'] = books['bank_selected'][record] if record >= 0 else None
    x['full_record_data'] = books['selected'][full_record] if full_record >= 0 else None
    x['local_minus_small'] = float(row['local_live'])-float(row['pooled_small_live'])
    x['local_minus_full'] = float(row['local_live'])-float(row['pooled_full_live'])
    return x


def main():
    result = json.loads((HERE/'RESULT.json').read_text())
    inputs = {'RESULT.json':sha(HERE/'RESULT.json')}
    books_by_world = {}
    source_records = []
    for world in result['worlds']:
        path=HERE/'memory'/f'world{world}'/'BOOKS.json'
        books=json.loads(path.read_text()); books_by_world[world]=books
        inputs[str(path.relative_to(HERE))]=sha(path)
        for j, record in enumerate(books['bank_selected']):
            a,b=record['book_counts']
            na,nb=sum(a),sum(b)
            tv_all=0.5*sum(abs(x/na-y/nb) for x,y in zip(a,b)) if na and nb else None
            # Purely source-side KT conditional on all six repeat roles.
            # No recipient k, quote, or replacement predictor is constructed.
            ar,br=sum(a[1:]),sum(b[1:])
            tv_repeat=0.5*sum(abs((a[r]+.5)/(ar+3)-(b[r]+.5)/(br+3))
                            for r in range(1,7)) if ar and br else None
            source_records.append(dict(world=world,record=j,prefix=record['prefix'],
                counts=[a,b],tv_all_roles=tv_all,tv_repeat_six_kt=tv_repeat,
                same_counts=a==b,one_empty=(na==0 or nb==0),
                same_empirical_all=all(x*nb==y*na for x,y in zip(a,b)) if na and nb else None))

    life_tables=[]
    examples={}
    options={}
    same_context_candidate_max_error=0.0
    for world in result['worlds']:
        books=books_by_world[world]
        for regime in result['regimes']:
            path=HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
            inputs[str(path.relative_to(HERE))]=sha(path)
            table={window:{'all':empty(), **{name:empty() for name in CLASSES}}
                   for window in WINDOWS}
            with gzip.open(path,'rt') as f:
                for row in csv.DictReader(f,delimiter='\t'):
                    t=int(row['t']); short=int(row['matched']); long=int(row['full_matched'])
                    assert long >= short
                    category=('bank_missing' if long and not short else 'full_longer' if long>short
                              else 'same' if short else 'neither')
                    l,s,full,g=(float(row[a+'_live']) for a in ('local','pooled_small','pooled_full','global'))
                    cl,cs,cf=(float(row[a+'_candidate']) for a in ('local','pooled_small','pooled_full'))
                    changes=dict(local_small=l-s,small_full=s-full,local_full=l-full,
                        local_global=l-g,candidate_local_small=cl-cs,
                        candidate_small_full=cs-cf,candidate_local_full=cl-cf)
                    if category=='same':
                        same_context_candidate_max_error=max(same_context_candidate_max_error,abs(cs-cf))
                    for window,(start,stop) in WINDOWS.items():
                        if start <= t < stop:
                            add(table[window]['all'],changes)
                            add(table[window][category],changes)
                    if regime=='recombined':
                        cold=float(row['logcold']); wa=list(map(float,row['local_w_before'].split(',')))
                        a,b=float(row['bank_a']),float(row['bank_b'])
                        # Descriptive examples only: choose a helpful observed
                        # forecast with a genuinely dominant source component.
                        for channel,idx,price,other in (('A',1,a,b),('B',2,b,a)):
                            if short and wa[idx]>max(wa[i] for i in range(3) if i!=idx) and price>max(cold,other) and l>max(cold,s):
                                key=(world,channel,int(row['record']))
                                if key not in options or l-s > options[key]['local_minus_small']:
                                    options[key]=row_example(world,regime,row,books)
                        if short and category=='same' and l<min(cold,s):
                            if 'matched_harm' not in examples or l-s<examples['matched_harm']['local_minus_small']:
                                examples['matched_harm']=row_example(world,regime,row,books)
                        if category in ('bank_missing','full_longer'):
                            if 'coverage_harm' not in examples or l-full<examples['coverage_harm']['local_minus_full']:
                                examples['coverage_harm']=row_example(world,regime,row,books)
            life_tables.append(dict(world=world,regime=regime,windows=table))

    pairs=[]
    for (world,channel,j),a in options.items():
        if channel!='A': continue
        for (w2,c2,j2),b in options.items():
            if w2==world and c2=='B' and j2!=j:
                pairs.append((min(a['local_minus_small'],b['local_minus_small']),a,b))
    if pairs:
        _,a,b=max(pairs,key=lambda x:x[0])
        examples['different_records_A_B']=[a,b]

    aggregate={}
    maximum_identity_error=0.0
    for regime in result['regimes']:
        subset=[x for x in life_tables if x['regime']==regime]
        aggregate[regime]={}
        for window in WINDOWS:
            aggregate[regime][window]={}
            for category in ('all',)+CLASSES:
                cell={key:math.fsum(x['windows'][window][category][key] for x in subset)/len(subset)
                      for key in ('count',)+FIELDS}
                maximum_identity_error=max(maximum_identity_error,
                    abs(cell['local_small']+cell['small_full']-cell['local_full']))
                aggregate[regime][window][category]=cell
    for life in life_tables:
        saved=next(x for x in result['lives'] if x['world']==life['world'] and x['regime']==life['regime'])
        for window in WINDOWS:
            def score(arm):
                return saved['arms'][arm]['tail'] if window=='tail' else saved['arms'][arm]['horizons'][window]
            calculated=life['windows'][window]['all']
            assert abs(calculated['local_small']-(score('local')-score('pooled_small'))) < 1e-7
            assert abs(calculated['small_full']-(score('pooled_small')-score('pooled_full'))) < 1e-7

    def distribution(key):
        values=sorted(r[key] for r in source_records if r[key] is not None)
        quartiles=statistics.quantiles(values,n=4,method='inclusive')
        return dict(n=len(values),minimum=min(values),q25=quartiles[0],median=statistics.median(values),
                    q75=quartiles[2],maximum=max(values),mean=statistics.fmean(values))
    source_summary=dict(records=len(source_records),same_counts=sum(r['same_counts'] for r in source_records),
        one_empty=sum(r['one_empty'] for r in source_records),same_empirical_all=sum(r['same_empirical_all'] is True for r in source_records),
        tv_all_roles=distribution('tv_all_roles'),tv_repeat_six_kt=distribution('tv_repeat_six_kt'))
    unrelated=[]
    for life in result['lives']:
        if life['regime']=='unrelated':
            unrelated.append(dict(world=life['world'],arms={a:{k:life['arms'][a][k]
                for k in ('activation','gain','tail','minimum')} for a in result['arms']}))
    payload=dict(status='POST-HOC DESCRIPTIVE; fixed saved forecasts only; no new predictor',
        scope='40 retained recipient traces and source BOOKS metadata; independent reader not rerun',
        input_sha256=inputs,mean_price_decompositions=aggregate,per_life=life_tables,
        max_additive_identity_error=maximum_identity_error,
        same_context_pooled_candidate_max_error=same_context_candidate_max_error,
        source_summary=source_summary,source_records=source_records,examples=examples,unrelated=unrelated,
        limitation='local-small bundles separate sources with local P0 selection and fixed-share revision. '
                   'It is not a causal estimate of the value of distinct books alone. Context partitions '
                   'use pretruth matches but live quotes carry each arm\'s earlier authority.')
    (HERE/'DIAGNOSIS.json').write_text(json.dumps(payload,indent=2,sort_keys=True,allow_nan=False)+'\n')
    print(json.dumps(dict(source_summary=source_summary,max_identity_error=maximum_identity_error,
        decomposition={r:{w:aggregate[r][w]['all'] for w in ('4096','16384','tail')} for r in result['regimes']},
        example_positions={name:([(e['world'],e['t']) for e in x] if isinstance(x,list) else (x['world'],x['t'])) for name,x in examples.items()}),indent=2))


if __name__=='__main__':
    main()
