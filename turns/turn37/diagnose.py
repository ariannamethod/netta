#!/usr/bin/env python3
"""Descriptive post-result diagnosis from sealed switched traces only."""
import csv
import gzip
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
WORLDS = range(328,336)
CONTROLS = ('coarse','fine','hier','null')
MOVE = 8192


def sample(world,row,raw,control='coarse'):
    t=int(row['t'])
    before=list(map(float,row['state_before'].split(',')))
    return dict(world=world,t=t,truth=int(row['truth']),rank=int(row['rank']),
        record=int(row['record']),history=row['history'],control=control,
        candidate_delta=float(row['direct_candidate'])-float(row[control+'_candidate']),
        received_delta=float(row['direct_live'])-float(row[control+'_live']),
        direct_candidate=float(row['direct_candidate']),
        control_candidate=float(row[control+'_candidate']),
        direct_live=float(row['direct_live']),control_live=float(row[control+'_live']),
        state_before=before,sources=list(map(float,row['sources'].split(','))),
        corrected=list(map(float,row['corrected'].split(','))),
        raw_hex=raw[max(0,t-16):t+17].hex())


def main():
    output=HERE/'DIAGNOSIS.json'
    assert not output.exists()
    per_world=[]
    veto=[]
    for world in WORLDS:
        raw=(HERE/'data'/f'world{world}'/'switched.bin').read_bytes()
        with gzip.open(HERE/'results'/f'world{world}'/'switched.tsv.gz','rt') as f:
            rows=list(csv.DictReader(f,delimiter='\t'))
        assert len(rows)==16384
        entry=dict(world=world)
        for control in CONTROLS:
            candidate=[float(row['direct_candidate'])-float(row[control+'_candidate']) for row in rows[MOVE:]]
            received=[float(row['direct_live'])-float(row[control+'_live']) for row in rows[MOVE:]]
            entry[control]=dict(candidate=math.fsum(candidate),received=math.fsum(received),
                absolute_received=math.fsum(map(abs,received)))
        per_world.append(entry)
        for row in rows[MOVE:]:
            before=list(map(float,row['state_before'].split(',')))
            if (max(before[1:3])<=0 and max(before[3:7])>0 and
                    float(row['direct_candidate'])!=float(row['coarse_candidate'])):
                veto.append(sample(world,row,raw))
    means={control:{key:math.fsum(entry[control][key] for entry in per_world)/8
        for key in ('candidate','received','absolute_received')} for control in CONTROLS}
    receipt=dict(means=means,per_world=per_world,parent_veto_tail=dict(
        events=len(veto),candidate_sum=math.fsum(x['candidate_delta'] for x in veto),
        received_sum=math.fsum(x['received_delta'] for x in veto),
        candidate_help=sum(x['candidate_delta']>0 for x in veto),
        received_help=sum(x['received_delta']>0 for x in veto),
        best_candidate=max(veto,key=lambda x:x['candidate_delta']),
        worst_candidate=min(veto,key=lambda x:x['candidate_delta'])))
    verify=json.loads((HERE/'VERIFY.json').read_text())
    for control in CONTROLS:
        measured=verify['quantities']['comparisons']['switched'][control]['tail']['mean']
        assert abs(measured-means[control]['received'])<1e-9
    with output.open('x') as f:
        json.dump(receipt,f,indent=2,sort_keys=True,allow_nan=False); f.write('\n')
    print(json.dumps(dict(means=means,parent_veto_tail={k:v for k,v in
        receipt['parent_veto_tail'].items() if not isinstance(v,dict)}),sort_keys=True))


if __name__=='__main__':
    main()
