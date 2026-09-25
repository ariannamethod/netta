#!/usr/bin/env python3
"""Post-hoc descriptions of saved turn25 prices; no predictor or new prices."""
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
SEAM = 8192
MODES = ('slow', 'fast', 'witness', 'latch')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_rows(path):
    with gzip.open(path, 'rt') as f:
        out = []
        for row in csv.DictReader(f, delimiter='\t'):
            out.append({k: (int(v) if k in ('t', 'matched', 'active_before',
                'admitted_after', 'latch_before', 'latch_after') or
                k.endswith('_slow_used') else float(v)) for k, v in row.items()})
    return out


def delta(row, baseline):
    return row['latch_live'] - row[baseline + '_live']


def totals(rows):
    return dict(count=len(rows), latch_vs_fast=math.fsum(delta(r, 'fast') for r in rows),
                latch_vs_witness=math.fsum(delta(r, 'witness') for r in rows))


def snap(row):
    keys = ('t', 'cold', 'candidate', 'matched', 'w_before', 'w_after',
            'latch_before', 'latch_after')
    x = {k: row[k] for k in keys}
    for mode in MODES:
        for suffix in ('live', 'odds_before', 'slow_used', 'odds_after'):
            x[mode + '_' + suffix] = row[mode + '_' + suffix]
    x['candidate_minus_cold'] = row['candidate'] - row['cold']
    x['latch_minus_fast'] = delta(row, 'fast')
    x['latch_minus_witness'] = delta(row, 'witness')
    return x


def main():
    result_path = HERE/'RESULT.json'
    result = json.loads(result_path.read_text())
    input_hashes = {'RESULT.json': sha(result_path)}
    per_world = []
    examples = []
    for world in result['worlds']:
        path = HERE/'results/authority'/f'world{world}'/'switched.tsv.gz'
        input_hashes[str(path.relative_to(HERE))] = sha(path)
        rows = read_rows(path)
        tail = rows[SEAM:]
        first_fast = next(r['t'] for r in tail if r['latch_slow_used'] == 0)
        sets = [r['t'] for r in rows if r['latch_before'] == 0 and r['latch_after'] == 1]
        returns = [r['t'] for r in rows if r['latch_before'] == 1 and r['latch_after'] == 0]
        # This subset is a recorded difference in current transition rates.
        # Its past effect on odds is NOT undone or repriced here.
        held = [r for r in tail if r['latch_slow_used'] == 0 and r['witness_slow_used'] == 1]
        neutral = [r for r in held if r['candidate'] == r['cold']]
        first_set = next(t for t in sets if t >= SEAM)
        first_return = next((t for t in returns if t > first_set), None)
        terminal = rows[-1]
        entry = dict(world=world,
            seam={m: rows[SEAM][m+'_odds_before'] for m in MODES},
            first_lifetime_set=sets[0], first_lifetime_return=returns[0],
            first_tail_set=first_set, first_tail_fast_update=first_fast,
            first_tail_quote_affected_by_that_update=first_fast+1,
            first_tail_return=first_return,
            tail_set_positions=[t for t in sets if t >= SEAM],
            tail_return_positions=[t for t in returns if t >= SEAM],
            pre_first_fast=totals([r for r in tail if r['t'] < first_fast]),
            on_first_fast=totals([r for r in tail if r['t'] == first_fast]),
            after_first_fast=totals([r for r in tail if r['t'] > first_fast]),
            before_first_affected_quote=totals([r for r in tail if r['t'] <= first_fast]),
            first_fast_row=snap(rows[first_fast]), tail=totals(tail),
            sign={name: totals([r for r in tail if test(r['candidate']-r['cold'])])
                  for name, test in [('help', lambda x:x>0), ('harm', lambda x:x<0),
                                     ('equal', lambda x:x==0)]},
            held_fast_while_witness_slow=totals(held),
            same_current_hazard=totals([r for r in tail if r['latch_slow_used']==r['witness_slow_used']]),
            neutral_held=totals(neutral),
            neutral_held_matched=sum(r['matched'] >= 1 for r in neutral),
            neutral_releases=sum(r['latch_before']==1 and r['latch_after']==0
                                 and r['candidate']==r['cold'] for r in tail),
            fast_updates={m:sum(r[m+'_slow_used']==0 for r in tail) for m in MODES},
            final_odds={m:terminal[m+'_odds_after'] for m in MODES},
            max_order_excess_latch_above_witness=max(r['latch_odds_before']-r['witness_odds_before'] for r in tail),
            max_order_excess_fast_above_latch=max(r['fast_odds_before']-r['latch_odds_before'] for r in tail))
        # Only a consistency check on additive summaries of existing prices.
        life = next(l for l in result['lives'] if l['world']==world and l['regime']=='switched')
        for baseline in ('fast', 'witness'):
            expected = life['modes']['latch']['tail']-life['modes'][baseline]['tail']
            assert abs(entry['tail']['latch_vs_'+baseline]-expected) < 1e-8
        per_world.append(entry)
        if neutral and not any(e['kind']=='neutral_hold' for e in examples):
            t = neutral[0]['t']
            examples.append(dict(kind='neutral_hold', world=world, regime='switched',
                                 center=t, rows=[snap(r) for r in rows[t-2:t+3]]))
        # Select the RESULT's own extrema; this adds no new outcome criterion.
        switched = [l for l in result['lives'] if l['regime']=='switched']
        for key, select in [('raw_help', max), ('raw_harm', min)]:
            life = select(switched, key=lambda x:x[key]['delta'])
            if life['world']==world:
                t=life[key]['t']
                examples.append(dict(kind=key, world=world, regime='switched',
                    center=t, rows=[snap(r) for r in rows[t-2:t+3]]))

    # Two already published moved-surface extrema provide the opposite regime.
    moved = [l for l in result['lives'] if l['regime']=='moved_mid']
    for key, select in [('raw_help', max), ('raw_harm', min)]:
        life=select(moved, key=lambda x:x[key]['delta'])
        world,t=life['world'],life[key]['t']
        path=HERE/'results/authority'/f'world{world}'/'moved_mid.tsv.gz'
        input_hashes[str(path.relative_to(HERE))]=sha(path)
        rows=read_rows(path)
        examples.append(dict(kind=key, world=world, regime='moved_mid', center=t,
                             rows=[snap(r) for r in rows[t-2:t+3]]))

    mean = lambda xs: math.fsum(xs)/len(xs)
    aggregate = {}
    for section in ('pre_first_fast','on_first_fast','after_first_fast',
                    'before_first_affected_quote','tail','held_fast_while_witness_slow',
                    'same_current_hazard','neutral_held'):
        aggregate[section] = {key: mean([w[section][key] for w in per_world])
                             for key in ('count','latch_vs_fast','latch_vs_witness')}
    aggregate['sign'] = {sign: {key:mean([w['sign'][sign][key] for w in per_world])
        for key in ('count','latch_vs_fast','latch_vs_witness')} for sign in ('help','harm','equal')}
    aggregate['first_fast_delays']=[w['first_tail_fast_update']-SEAM for w in per_world]
    aggregate['total_neutral_held']=sum(w['neutral_held']['count'] for w in per_world)
    aggregate['total_neutral_releases']=sum(w['neutral_releases'] for w in per_world)
    aggregate['mean_seam_odds']={m:mean([w['seam'][m] for w in per_world]) for m in MODES}
    aggregate['tail_fast_updates']={m:sum(w['fast_updates'][m] for w in per_world) for m in MODES}
    payload=dict(status='POST-HOC DESCRIPTIVE; no intervention, new predictions, or gate',
        units='bits; t is zero-based; partitions average the eight saved switched lives',
        input_sha256=input_hashes, aggregate=aggregate, worlds=per_world, examples=examples,
        limit='Present-rate partitions do not identify the effect of removing a past latch transition. '
              'All compared odds and prices are the recorded complete histories.')
    (HERE/'DIAGNOSIS.json').write_text(json.dumps(payload,indent=2,sort_keys=True,allow_nan=False)+'\n')
    print(json.dumps(aggregate,indent=2,sort_keys=True))
    for w in per_world:
        print(w['world'], 'set/fast/return', w['first_tail_set'], w['first_tail_fast_update'],
              w['first_tail_return'], 'before/after',w['before_first_affected_quote']['latch_vs_fast'],
              w['after_first_fast']['latch_vs_fast'],'seam',w['seam'])


if __name__=='__main__':
    main()
