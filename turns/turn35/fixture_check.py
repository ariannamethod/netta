#!/usr/bin/env python3
"""Independent Decimal reconstruction of the frozen turn35 hybrid fixture."""
import argparse
from decimal import localcontext
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

HERE=Path(__file__).resolve().parent


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=HERE/'FIXTURE_READER.json')
    args=parser.parse_args()
    if args.output.exists():raise FileExistsError(args.output)
    spec=importlib.util.spec_from_file_location('independent_fixture35',HERE/'verify.py')
    reader=importlib.util.module_from_spec(spec);spec.loader.exec_module(reader)
    run=subprocess.run([str(HERE/'hybrid_router'),'--fixture'],capture_output=True,text=True,check=True)
    rows=[json.loads(line) for line in run.stdout.splitlines()]
    wealths,permissions,visits={}, {}, {}
    error=checks=0
    witnessed=dict(first=False,silent=False,recovery=False,locality=False,new=False,
                   no_match=False,common=False,fine=False,coarse=False)
    fine_positive=[False]*4;fine_negative=[False]*4
    coarse_positive=[False]*2;coarse_negative=[False]*2
    previous={}

    def check(actual,expected,label):
        nonlocal error,checks
        error=max(error,reader.near(actual,expected,label));checks+=1

    with localcontext() as context:
        context.prec=50
        for t,row in enumerate(rows):
            assert row['t']==t
            record=row['record'];matched=record>=0;fine=bool(row['fine'])
            if matched:
                assert fine==(record==0)
                wealths.setdefault(record,reader.HybridWealth())
                permissions.setdefault(record,reader.Permission())
                visits.setdefault(record,0)
                wealth,permission=wealths[record],permissions[record]
            else:
                assert not fine
                wealth,permission=reader.HybridWealth(),reader.Permission()
            parts=row['parts'];assert len(parts)==4
            assert row['visits_before']==(visits[record] if matched else 0)
            before=(tuple(map(float,wealth.h)),float(permission.u))
            if matched and record in previous:witnessed['locality']|=before==previous[record]
            for i in range(4):check(row['wealth_before'][i],wealth.h[i],f'{t}/wealth/{i}/before')
            check(row['permission_before'],permission.u,f'{t}/permission/before')
            corrected=wealth.source(fine,row['pooled'],parts)
            quote=permission.quote(row['cold'],corrected)
            pooled_quote=permission.quote(row['cold'],row['pooled'])
            check(row['corrected'],corrected,f'{t}/corrected')
            check(row['hybrid'],quote,f'{t}/hybrid')
            check(row['pooled_quote'],pooled_quote,f'{t}/pooled')
            silent=wealth.silent(fine)
            if matched and visits[record]==0:
                witnessed['first']=True;assert row['hybrid']==row['pooled_quote']
            if matched and silent:
                witnessed['silent']=True;assert row['hybrid']==row['pooled_quote']
            if matched:
                witnessed['fine']|=fine;witnessed['coarse']|=not fine
                values=wealth.h[:4 if fine else 2]
                pos=fine_positive if fine else coarse_positive
                neg=fine_negative if fine else coarse_negative
                for i,value in enumerate(values):
                    pos[i]|=value>0;neg[i]|=value<0
            witnessed['new']|=row['rank']==0
            witnessed['no_match']|=not matched
            witnessed['common']|=matched and all(value==row['pooled']==row['cold'] for value in parts)
            old_silent=wealth.silent(fine)
            if matched:
                wealth.observe(fine,row['pooled'],parts)
                permission.observe(row['pooled'],row['pooled_quote'])
                visits[record]+=1
                previous[record]=(tuple(map(float,wealth.h)),float(permission.u))
            witnessed['recovery']|=matched and old_silent and not wealth.silent(fine)
            for i in range(4):check(row['wealth_after'][i],wealth.h[i],f'{t}/wealth/{i}/after')
            check(row['permission_after'],permission.u,f'{t}/permission/after')
            assert row['wealth_struct_bytes']==32 and 0<=row['max_norm_error']<=1e-8

    assert len(rows)==40 and sorted(wealths)==[0,1]
    assert all(witnessed.values()),witnessed
    assert all(fine_positive) and all(fine_negative),(fine_positive,fine_negative)
    assert any(coarse_positive) and all(coarse_negative),(coarse_positive,coarse_negative)
    receipt=dict(events=len(rows),records=sorted(wealths),numeric_checks=checks,
        max_error=error,passed=True,witnessed=witnessed,
        fine_positive=fine_positive,fine_negative=fine_negative,
        coarse_positive=coarse_positive,coarse_negative=coarse_negative,
        reader_sha256=reader.digest(HERE/'verify.py'),
        fixture_sha256=hashlib.sha256(run.stdout.encode()).hexdigest(),
        binary_sha256=reader.digest(HERE/'hybrid_router'),
        scope='One handcrafted no-world fixture: independent Decimal hybrid wealth '
              'at one fine and one coarse relation, pooled permission, first/silent, '
              'positive/negative, recovery, locality, NEW, no-match and exact common events.')
    with args.output.open('x') as stream:
        json.dump(receipt,stream,indent=2,sort_keys=True,allow_nan=False);stream.write('\n')
    print(json.dumps(receipt,sort_keys=True))


if __name__=='__main__':main()
