#!/usr/bin/env python3
"""Independent incoming HMM audit: probability forward recursion, every prefix."""
import csv
import hashlib
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'byte_recurrence/scratch/hmm16'
H=2**-16
TOL=1e-8

def near(a,b):
    assert math.isfinite(a) and math.isfinite(b)
    error=abs(a-b)
    assert error<TOL,(a,b,error)
    return error

def capital(logratio):
    return max(0.,logratio)+math.log1p(2**(-abs(logratio)))/math.log(2.)-1

def main():
    fixed=json.loads((ROOT/'byte_recurrence/HMM_FREEZE.json').read_text())
    assert all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in fixed.items())
    result=json.loads((ROOT/'byte_recurrence/HMM_RESULT.json').read_text())
    checked=[]; maximum=0.
    for w in range(24,32):
        for regime in ('preserved','unrelated','changed_tail'):
            raw=(OUT/'data'/f'world{w:02d}'/(regime+'.bin')).read_bytes()
            paths=[OUT/'traces'/f'world{w:02d}'/(regime+'-'+m+'.tsv') for m in ('static','hmm')]
            active=False; nactive=0; shadow=0.; weight=.5; gain=0.; old_logratio=0.
            minimum=0.; peak=0.; drawdown=0.; extra_max=0.; at8192=0.; old_at8192=0.; activation=None
            with paths[0].open() as f, paths[1].open() as g:
                for t,(a,b) in enumerate(zip(csv.DictReader(f,delimiter='\t'),csv.DictReader(g,delimiter='\t'),strict=True)):
                    assert int(a['t'])==int(b['t'])==t
                    assert int(a['truth'])==int(b['truth'])==raw[t]
                    assert a['pattern']==b['pattern'] and a['group']==b['group']
                    assert int(a['active_before'])==int(b['active_before'])==int(active)
                    c=float(a['logcold']); s=float(a['logcandidate']); delta=s-c
                    maximum=max(maximum,near(c,float(b['logcold'])),near(s,float(b['logcandidate'])),
                                near(shadow,float(a['shadow_before'])),near(shadow,float(b['shadow_before'])))
                    if active:
                        nactive+=1
                        assert weight<=1-H+1e-14 and weight>=0
                        ell=float(b['odds_before'])
                        quotedweight=(1/(1+2**(-ell))) if ell>=0 else (2**ell/(1+2**ell))
                        maximum=max(maximum,near(weight,quotedweight))
                        relative=(1-weight)+weight*2**delta
                        loglive=c+math.log2(relative)
                        gain+=math.log2(relative)
                        weight=(1-H)*weight*2**delta/relative
                        old_logratio+=delta
                    else:
                        loglive=c
                    static_gain=capital(old_logratio) if active else 0.
                    maximum=max(maximum,near(loglive,float(b['loglive'])),near(gain,float(b['gain_after'])),
                                near(static_gain,float(a['gain_after'])))
                    minimum=min(minimum,gain);peak=max(peak,gain);drawdown=max(drawdown,peak-gain)
                    assert minimum>=-1-TOL and drawdown<=16+TOL
                    extra=static_gain-gain
                    assert extra<=max(0,nactive-1)*(-math.log1p(-H)/math.log(2))+TOL
                    extra_max=max(extra_max,extra)
                    shadow+=delta
                    crossing=(not active and shadow>=32.)
                    assert int(a['activated_after'])==int(b['activated_after'])==int(crossing)
                    if crossing:
                        active=True;weight=.5;old_logratio=0.;activation=t+1
                    if t==8191: at8192=gain;old_at8192=static_gain
                assert t+1==len(raw)==16384
            supplied=next(x for x in result['lives'] if x['world']==w and x['regime']==regime)
            for key,value in [('gain',gain),('static_gain',static_gain),('at8192',at8192),('tail_gain',gain-at8192),('minimum',minimum),('drawdown',drawdown)]:
                maximum=max(maximum,near(value,supplied[key]))
            assert supplied['activation']==activation and supplied['active_events']==nactive
            checked.append(dict(world=w,regime=regime,gain=gain,static_gain=static_gain,tail_gain=gain-at8192,
                                static_tail_gain=static_gain-old_at8192,maximum_drawdown=drawdown,minimum=minimum,
                                maximum_extra_loss_at_any_prefix=extra_max,activation=activation))
    report={'status':'PASS','raw_byte_predictions_checked':len(checked)*16384,
            'all_prefix_lifetime_interval_and_static_cost_bounds_checked':True,
            'maximum_numeric_difference':maximum,'lives':checked,
            'scope':'Fresh exact Sol reproduction; independent probability recursion and gate chronology. No new mechanism or targets.'}
    with (ROOT/'turn3/evidence/HMM_AUDIT.json').open('x') as f: json.dump(report,f,indent=2);f.write('\n')
    print({k:v for k,v in report.items() if k!='lives'})

if __name__=='__main__':main()
