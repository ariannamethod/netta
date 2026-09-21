#!/usr/bin/env python3
"""Independent probability-space reader of retained turn23 split quotes."""
import csv
import gzip
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
N = 16384
TOL = 1e-7


def equal(a, b):
    return math.isfinite(a) and math.isfinite(b) and abs(a-b) <= TOL


def check_life(w, regime, saved):
    source = HERE/'results'/f'world{w}'/(regime+'.tsv.gz')
    policy = HERE/'results/policies'/f'world{w}'/(regime+'-split.tsv.gz')
    control = HERE/'results/policies'/f'world{w}'/(regime+'-controls.tsv.gz')
    shadow = 0.
    active = False
    capital = [0., 0., 0.]  # absorbing cold, short, long
    clocks = [0., 0.]
    gain = tail = 0.
    minimum = peak = drawdown = 0.
    early = activation = None
    maximum_error = 0.
    with gzip.open(source, 'rt') as sf, gzip.open(policy, 'rt') as pf, gzip.open(control, 'rt') as cf:
        rows = zip(csv.DictReader(sf, delimiter='\t'),
                   csv.DictReader(pf, delimiter='\t'),
                   csv.DictReader(cf, delimiter='\t'))
        seen = 0
        for seen, (raw, c, ref) in enumerate(rows, start=1):
            t = seen-1
            assert int(raw['t']) == int(c['t']) == int(ref['t']) == t
            p0 = float(raw['logcold'])
            p1 = float(raw['episode_candidate'])
            matched = int(raw['episode_matchedL'])
            delta = p1-p0
            lane = None if matched == 0 else 0 if matched <= 2 else 1
            assert p0 == float(c['cold']) == float(ref['cold'])
            assert p1 == float(c['candidate']) == float(ref['candidate'])
            assert matched == int(c['matched']) == int(ref['matched'])
            assert int(c['active_before']) == active
            assert equal(float(c['shadow_before']), shadow)
            assert equal(float(c['short_before']), capital[1])
            assert equal(float(c['long_before']), capital[2])
            assert equal(float(c['short_w_before']), clocks[0])
            assert equal(float(c['long_w_before']), clocks[1])
            # This reader uses two full log-probability terms and a log-sum;
            # the C writer uses a normalized likelihood ratio.
            selected = capital[lane+1] if active and lane is not None else 0.
            if not active or p1 == p0 or lane is None:
                quote = p0
            else:
                quote = math.log2((1.-selected)*2.**p0 + selected*2.**p1)
            actual = float(c['live'])
            maximum_error = max(maximum_error, abs(quote-actual))
            assert equal(quote, actual)
            if not active or p1 == p0 or lane is None or int(raw['rank']) == 0:
                assert actual == p0
            shadow += delta
            first = False
            hazards = [-1., -1.]
            if not active and shadow >= 32.:
                active = True
                capital = [.5, .25, .25]
                clocks = [0., 0.]
                first = True
                activation = t+1
            elif active:
                # Reweight three unnormalized portfolios, then normalize.
                tentative = capital.copy()
                if lane is not None:
                    tentative[lane+1] *= 2.**delta
                mass = math.fsum(tentative)
                capital = [x/mass for x in tentative]
                hazards = [2.**-10 if w <= -1. else 2.**-16 for w in clocks]
                for i, hazard in enumerate(hazards):
                    moved = capital[i+1]*hazard
                    capital[0] += moved
                    capital[i+1] -= moved
                if lane is not None:
                    clocks[lane] = (31./32.)*clocks[lane] + delta
            assert int(c['admitted_after']) == first == int(raw['episode_activated_after'])
            assert int(ref['admitted_after']) == first
            for key, value in (('short_after',capital[1]),('long_after',capital[2]),
                               ('short_w_after',clocks[0]),('long_w_after',clocks[1]),
                               ('short_hazard',hazards[0]),('long_hazard',hazards[1])):
                error = abs(float(c[key])-value)
                maximum_error = max(maximum_error,error)
                assert error <= TOL, (w,regime,t,key,error)
            if active:
                assert all(x>=0 for x in capital)
                assert abs(math.fsum(capital)-1.) <= 1e-10
                assert capital[0] >= 2.**-16-1e-12
            d = actual-p0
            gain += d
            if t >= N//2: tail += d
            minimum = min(minimum,gain)
            peak = max(peak,gain)
            drawdown = max(drawdown,peak-gain)
            if t == 4095: early = gain
        assert seen == N
    expected = saved['policies']['split']
    assert activation == expected['activation']
    for key,value in (('gain',gain),('early',early),('tail',tail),
                      ('minimum',minimum),('drawdown',drawdown)):
        error = abs(expected[key]-value)
        maximum_error = max(maximum_error,error)
        assert error <= TOL, (w,regime,key,error)
    assert minimum >= -1-TOL and drawdown <= 16+TOL
    return dict(world=w, regime=regime, forecasts=seen,
                maximum_numeric_error=maximum_error)


def main():
    result = json.loads((HERE/'RESULT.json').read_text())
    lives = [check_life(x['world'],x['regime'],x) for x in result['lives']]
    out = dict(forecasts=sum(x['forecasts'] for x in lives),
               maximum_numeric_error=max(x['maximum_numeric_error'] for x in lives),
               lives=lives, verified=True, gate_pass=result['gate_pass'])
    path = HERE/'VERIFY.json'
    with path.open('x') as f:
        json.dump(out,f,indent=2,sort_keys=True,allow_nan=False)
        f.write('\n')
    print(json.dumps({k:v for k,v in out.items() if k!='lives'},indent=2))


if __name__ == '__main__': main()
