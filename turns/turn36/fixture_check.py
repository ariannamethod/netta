#!/usr/bin/env python3
"""Handpriced two-level fixture, before fresh data."""
import argparse
import csv
from decimal import localcontext
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('turn36_fixture_reader',HERE/'verify.py')
reader=importlib.util.module_from_spec(spec)
sys.modules[spec.name]=reader
spec.loader.exec_module(reader)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path,default=HERE/'FIXTURE.json')
    args=parser.parse_args()
    assert not args.output.exists()
    trace=args.output.with_suffix('.tsv')
    with trace.open('xb') as out:
        proc=subprocess.run([str(HERE/'.build/hier_router'),'--fixture'],stdout=out)
    assert proc.returncode==0
    states=[reader.State(),reader.State()]
    votes=[[300,1,1],[2,4,100],[1,200,2],[1,1,50]]
    error,count=0.,0
    positive,negative=[False]*4,[False]*4
    def check(a,b,label):
        nonlocal error,count
        error=max(error,reader.near(a,b,label,1e-10)); count+=1
    with localcontext() as context,trace.open() as f:
        context.prec=50
        rows=list(csv.DictReader(f,delimiter='\t'))
        assert len(rows)==80
        for t,row in enumerate(rows):
            r,truth=int(row['record']),int(row['truth'])
            expected_r=-1 if t in (4,17) else 1 if t in (1,18,22) else 0
            expected_truth=23 if t==2 else 0 if t<14 else 1 if t<28 else 2 if t<42 else 0 if t<56 else 2 if t<68 else 1
            assert int(row['t'])==t and r==expected_r and truth==expected_truth
            prices=[-8.]*11
            if r>=0 and t!=3 and truth<3:
                def price(values): return math.log2((3/256)*(values[truth]+.5)/(sum(values)+1.5))
                prices[0]=price([sum(v[b] for v in votes) for b in range(3)])
                for p in (0,1): prices[1+p]=price([votes[2*p][b]+votes[2*p+1][b] for b in range(3)])
                for e in range(4):
                    prices[3+e]=price(votes[e])
                    prices[7+e]=price([votes[e^1][0],votes[e][1],votes[e^1][2]])
            for i,(a,b) in enumerate(zip(reader.floats(row['sources']),prices)): check(a,b,f'{t}/source{i}')
            s=states[r] if r>=0 else reader.State()
            before=reader.floats(row['state_before'])
            for i,(a,b) in enumerate(zip(before,s.vector())): check(a,b,f'{t}/before{i}')
            corr=s.sources(prices)
            actual_corr=reader.floats(row['corrected'])
            for i,(a,b) in enumerate(zip(actual_corr,corr)): check(a,b,f'{t}/corrected{i}')
            quote=s.permission.quote(-8.,prices[0])
            check(row['pooled_quote'],quote,f'{t}/permission')
            if max(before[3:7])<=0: assert actual_corr[0]==actual_corr[1]
            if max(before[1:3])<=0: assert actual_corr[0]==prices[0]
            if r>=0: s.observe(prices,quote)
            for i,(a,b) in enumerate(zip(reader.floats(row['state_after']),s.vector())): check(a,b,f'{t}/after{i}')
            for i,value in enumerate(s.vector()[3:7]):
                positive[i] |= value>0; negative[i] |= value<0
    assert all(positive) and all(negative),(positive,negative)
    receipt=dict(events=len(rows),checks=count,max_error=error,detail_positive=positive,detail_negative=negative,
                 exact_silence=True,trace_sha256=reader.digest(trace))
    with args.output.open('x') as f:
        json.dump(receipt,f,indent=2,sort_keys=True); f.write('\n')
    print(json.dumps(receipt,sort_keys=True))


if __name__=='__main__': main()
