#!/usr/bin/env python3
"""Post-result formatting only: turn21's frozen measurements into TSV tables."""

import csv
import json
from pathlib import Path


here = Path(__file__).resolve().parent
result = json.loads((here / 'RESULT.json').read_text())
assert len(result['lives']) == 32
assert len(result['modes']) == 27 and len(result['grid_points']) == 24
levels = {p: tuple(g) for p, g in zip(result['grid_points'], result['grid'])}

with (here / 'LIFE_TABLE.tsv').open('w', newline='') as stream:
    columns = ('world', 'regime', 'mode', 'high', 'low', 'early4096_bits',
               'whole16384_bits', 'tail8192_bits', 'admission_next_t',
               'max_odds_before', 'max_odds_after', 'odds_at_move', 'clips',
               'low_clips', 'high_clips', 'slow_count', 'fast_count',
               'minimum_prefix_bits', 'maximum_drawdown_bits')
    writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
    writer.writerow(columns)
    lives = 0
    for life in result['lives']:
        lives += 1
        for mode in result['modes']:
            row = life['modes'][mode]
            high, low = levels.get(mode, ('', ''))
            writer.writerow((life['world'], life['regime'], mode, high, low,
                             row['early'], row['gain'], row['tail'],
                             row['activation'], row['max_odds_before'],
                             row['max_odds_after'], row['odds_at_move'],
                             row['clip_count'], row['low_clip_count'],
                             row['high_clip_count'], row['slow_count'],
                             row['fast_count'], row['minimum'],
                             row['max_drawdown']))

with (here / 'BYTE_EXAMPLES.tsv').open('w', newline='') as stream:
    columns = ('world', 'regime', 'comparison', 'direction', 'left', 'right',
               't', 'truth', 'rank', 'matchedL', 'cold_log2', 'candidate_log2',
               'left_live', 'right_live', 'left_minus_right_bits',
               'w_before', 'w_after', 'hazard',
               'left_odds_before', 'left_odds_after', 'left_cap_after',
               'left_clipped', 'right_odds_before', 'right_odds_after',
               'right_cap_after', 'right_clipped')
    writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
    writer.writerow(columns)
    examples = 0
    for life in result['lives']:
        if life['regime'] not in ('switched', 'moved_mid'):
            continue
        for comparison, held in sorted(life['samples'].items()):
            for direction in ('help', 'harm'):
                row = held[direction]
                examples += 1
                writer.writerow((row['world'], row['regime'], row['comparison'],
                                 direction, row['left'], row['right'], row['t'],
                                 row['truth'], row['rank'], row['matchedL'],
                                 row['cold'], row['candidate'],
                                 row['left_live'], row['right_live'], row['delta'],
                                 row['witness_w_before'], row['witness_w_after'],
                                 row['hazard'],
                                 row['left_odds_before'], row['left_odds_after'],
                                 row['left_cap_after'], row['left_clipped'],
                                 row['right_odds_before'], row['right_odds_after'],
                                 row['right_cap_after'], row['right_clipped']))

print(f'{lives * len(result["modes"])} life rows; {examples} charged-byte examples')
