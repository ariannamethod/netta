"""Deterministic presentation of the retained result; no predictor is run."""
import csv
import gzip
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
result = json.loads((HERE/'RESULT.json').read_text())
by = {(x['world'], x['regime']): x for x in result['lives']}
mean = lambda xs: math.fsum(xs)/len(xs)
tables = ['# Turn16: all paired outcomes', '',
          'Bits saved relative to P0. No world is omitted. Positive means cheaper prediction.',
          'Full results, including every other candidate arm, are in RESULT.json.', '']
for regime in result['regimes']:
    tables += ['## '+regime, '',
               '| world | slow full | fast full | budget full | return full | slow tail | fast tail | budget tail | return tail | return bytes |',
               '|---|---:|---:|---:|---:|---:|---:|---:|---:|---|']
    for world in result['worlds']:
        life = by[world, regime]
        states = [life['modes'][m]['episode'] for m in result['modes']]
        cells = [str(world)] + [f"{s['gain']:.6f}" for s in states] + [
            f"{s['tail']:.6f}" for s in states] + [str(states[-1]['returns'])]
        tables.append('| '+' | '.join(cells)+' |')
    tables += ['']
tables += ['## Early gain on recombined, 4096 bytes', '',
           '| world | slow | fast | budget | return | return minus budget |',
           '|---|---:|---:|---:|---:|---:|']
for world in result['worlds']:
    s = by[world, 'recombined']['modes']
    nums = [s[m]['episode']['horizons']['4096'] for m in result['modes']]
    tables += ['| '+str(world)+' | '+' | '.join(f'{n:.6f}' for n in nums+[nums[-1]-nums[-2]])+' |']
(HERE/'TABLES.md').write_text('\n'.join(tables)+'\n')

# Post-hoc illustrations chosen for declared roles, not for a second gate:
# the only post-seam surface return, the only post-seam law return, and
# the largest early return benefit. These choices are not new experiments.
selected = [(178, 'moved_mid'), (181, 'switched'), (177, 'recombined')]
examples = []
for world, regime in selected:
    life = by[world, regime]
    event = next(e for e in life['return_events'] if e['arm'] == 'episode')
    t0 = event['t']
    with gzip.open(HERE/'results'/f'world{world}'/(regime+'.tsv.gz'), 'rt') as stream:
        source = {int(r['t']): r for r in csv.DictReader(stream, delimiter='\t')
                  if t0 <= int(r['t']) <= t0+256}
    shown = []
    with gzip.open(HERE/'results/authority'/f'world{world}'/(regime+'.tsv.gz'), 'rt') as stream:
        for row in csv.DictReader(stream, delimiter='\t'):
            t = int(row['t'])
            if row['arm'] != 'episode' or t not in source:
                continue
            r = source[t]
            shown.append(dict(t=t, truth=int(r['truth']), rank=int(r['rank']),
                history=r['history'], heads=r['heads'], matched_length=int(r['episode_matchedL']),
                votes=r['episode_votes'], cold=float(row['cold']),
                candidate=float(row['candidate']), fast=float(row['fast_live']),
                budget=float(row['budget_live']), returned=float(row['return_live']),
                extra=float(row['return_live'])-float(row['budget_live']),
                odds_before=float(row['return_odds_before']),
                odds_after=float(row['return_odds_after']),
                support_before=float(row['return_support_before']),
                event=int(row['return_event'])))
    following = shown[1:]
    examples.append(dict(world=world, regime=regime, event=event,
                         next_256_extra=math.fsum(r['extra'] for r in following),
                         crossing=shown[0], next=shown[1],
                         help=max(following, key=lambda r:r['extra']),
                         harm=min(following, key=lambda r:r['extra']),
                         window=shown))
(HERE/'RAW.json').write_text(json.dumps(examples, indent=2)+'\n')
raw = ['# Turn16: observed returns and their prices', '',
       'Rows come from retained candidate and C authority TSVs. Event t changes the',
       'following quote t+1. Delta is return minus the identical-prior budget control.',
       'Selection is post hoc: sole post-seam surface return; sole post-seam law',
       'return; largest early benefit. Each displayed window includes losses.', '']
for x in examples:
    e = x['event']
    raw += [f"## World {x['world']}, {x['regime']}", '',
            f"Return after byte t={e['t']}; next byte {e['next_byte']}.",
            f"Support {e['support_at_return']:.9f} bits; pretruth odds {e['old_odds']:.9f}; next odds {e['odds_after']:.9f}.",
            f"Next 256 bytes: {x['next_256_extra']:+.9f} extra bits versus budget.", '',
            '| row | t | truth hex | rank | matched L | cold log2 | candidate log2 | budget log2 | return log2 | delta |',
            '|---|---:|---|---:|---:|---:|---:|---:|---:|---:|']
    for tag in ('crossing','next','help','harm'):
        r=x[tag]
        raw += [f"| {tag} | {r['t']} | {r['truth']:02x} | {r['rank']} | {r['matched_length']} | {r['cold']:.9f} | {r['candidate']:.9f} | {r['budget']:.9f} | {r['returned']:.9f} | {r['extra']:+.9f} |"]
    raw += ['', 'Crossing history: '+x['crossing']['history']+'.',
            'Crossing stored votes for NEW, ranks 1..6: '+x['crossing']['votes']+'.',
            'Crossing source price is still charged under old authority; the next quote uses the renewed prior.', '']
(HERE/'RAW.md').write_text('\n'.join(raw)+'\n')
print(json.dumps([dict(world=x['world'], regime=x['regime'],
                      next_256_extra=x['next_256_extra']) for x in examples], indent=2))
