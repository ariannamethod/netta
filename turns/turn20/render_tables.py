#!/usr/bin/env python3
"""Post-result formatting only: turn20's frozen measurements into TSV tables."""

import csv
import json
from pathlib import Path


here = Path(__file__).resolve().parent
result = json.loads((here / 'RESULT.json').read_text())
assert len(result['lives']) == 32
assert len(result['modes']) == 6

with (here / 'LIFE_TABLE.tsv').open('w', newline='') as stream:
    columns = ('world', 'regime', 'mode', 'early4096_bits', 'whole16384_bits',
               'tail8192_bits', 'admission_next_t', 'max_odds_before',
               'max_odds_after', 'clips', 'low_clips', 'high_clips',
               'slow_count', 'fast_count', 'minimum_prefix_bits',
               'maximum_drawdown_bits')
    writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
    writer.writerow(columns)
    for life in result['lives']:
        for mode in result['modes']:
            row = life['modes'][mode]
            writer.writerow((life['world'], life['regime'], mode,
                             row['early'], row['gain'], row['tail'],
                             row['activation'], row['max_odds_before'],
                             row['max_odds_after'], row['clip_count'],
                             row['low_clip_count'], row['high_clip_count'],
                             row['slow_count'], row['fast_count'],
                             row['minimum'], row['max_drawdown']))

with (here / 'BYTE_EXAMPLES.tsv').open('w', newline='') as stream:
    columns = ('world', 'regime', 'comparison', 'direction', 't', 'truth',
               'matchedL', 'cold_log2', 'candidate_log2', 'new_log2',
               'other_log2', 'new_minus_other_bits', 'w_before', 'w_after',
               'new_odds_before', 'new_odds_after', 'new_cap_after')
    writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
    writer.writerow(columns)
    for life in result['lives']:
        if life['regime'] not in ('switched', 'moved_mid'):
            continue
        for other, prefix in (('ceiling', 'raw_cap'), ('witness', 'raw_witness')):
            for direction, suffix in (('help', 'help'), ('harm', 'harm')):
                row = life[prefix + '_' + suffix]
                writer.writerow((life['world'], life['regime'], other, direction,
                                 row['t'], row['truth'], row['matchedL'],
                                 row['cold'], row['candidate'],
                                 row['witnessed_ceiling_live'], row[other+'_live'],
                                 row['delta'], row['witness_w_before'],
                                 row['witness_w_after'],
                                 row['witnessed_ceiling_odds_before'],
                                 row['witnessed_ceiling_odds_after'],
                                 row['witnessed_ceiling_cap_after']))

print('192 life rows; 64 charged-byte examples')
