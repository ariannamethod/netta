#!/usr/bin/env python3
"""Descriptive reading of the frozen switched traces; no new predictions."""
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path

HERE=Path(__file__).resolve().parent


def main():
    extremes={}
    groups={key:dict(rows=0,live=[],candidate=[],absolute=[]) for key in
            ('all','positive_detail_under_silent_parent','speaking_identical_sibling_counts')}
    for w in range(320,328):
        book=json.loads((HERE/'memory'/f'world{w}'/'BOOKS.json').read_text())
        raw=(HERE/'data'/f'world{w}'/'switched.bin').read_bytes()
        with gzip.open(HERE/'results'/f'world{w}'/'switched.tsv.gz','rt') as f:
            for row in csv.DictReader(f,delimiter='\t'):
                t=int(row['t'])
                if t<8192: continue
                record=int(row['record'])
                state=list(map(float,row['state_before'].split(',')))
                flags=['all']
                if record>=0:
                    episodes=book['selected'][record]['episode_counts']
                    if any(state[1+p]<=0 and max(state[3+2*p:5+2*p])>0 for p in (0,1)):
                        flags.append('positive_detail_under_silent_parent')
                    if any(state[1+p]>0 and max(state[3+2*p:5+2*p])>0 and
                           episodes[2*p]==episodes[2*p+1] and
                           sum(episodes[2*p][1:int(row['k'])+1])>0 for p in (0,1)):
                        flags.append('speaking_identical_sibling_counts')
                live=float(row['hier_live'])-float(row['coarse_live'])
                candidate=float(row['hier_candidate'])-float(row['coarse_candidate'])
                for key in flags:
                    group=groups[key]; group['rows']+=1
                    group['live'].append(live); group['candidate'].append(candidate); group['absolute'].append(abs(live))
                for other in ('coarse','fine'):
                    difference=float(row['hier_live'])-float(row[other+'_live'])
                    for kind,better in (('help',lambda a,b:a>b),('harm',lambda a,b:a<b)):
                        key=other+'_'+kind
                        if key not in extremes or better(difference,extremes[key]['difference']):
                            extremes[key]=dict(world=w,regime='switched',t=t,other=other,difference=difference,row=dict(row),
                                source_record=book['selected'][record] if record>=0 else None,
                                raw_hex=raw[max(0,t-16):t+17].hex())
    summary={key:dict(rows=g['rows'],received_bits_per_life=math.fsum(g['live'])/8,
             candidate_bits_per_life=math.fsum(g['candidate'])/8,absolute_received_bits_per_life=math.fsum(g['absolute'])/8)
             for key,g in groups.items()}
    report=dict(scope='Descriptive subsets can overlap; no causal decomposition or alternate predictor.',
        subsets=summary,extremes=extremes,script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    with (HERE/'DIAGNOSIS.json').open('x') as f:
        json.dump(report,f,indent=2,sort_keys=True); f.write('\n')
    lines=['# Turn36 raw switched-tail help and harm\n',
           'Mechanically selected largest and smallest received hier−control byte differences over worlds 320–327 and t>=8192. All quotes precede truth. Full original rows and counts are in DIAGNOSIS.json.\n']
    for key,x in extremes.items():
        row=x['row']; state=list(map(float,row['state_before'].split(',')))
        lines += [f'## {key}: world{x["world"]}, t{x["t"]}\n',
            f'Truth decimal {row["truth"]}, rank {row["rank"]}, record {row["record"]}. Paid difference **{x["difference"]:+.12f} bits**.\n',
            f'Raw neighborhood: `{x["raw_hex"]}`. Pretruth relation history: `{row["history"]}`.\n',
            f'Pooled permission {state[0]:.12g}; case wealth A/B {state[1:3]}; parent-relative detail wealth {state[3:7]}; flat fine wealth {state[11:15]}.\n',
            '| arm | candidate log2 probability | received log2 probability | outer log2 odds |\n|---|---:|---:|---:|\n']
        for a in ('hier','coarse','fine','null','pooled','cold'):
            lines.append(f'| {a} | {float(row[a+"_candidate"]):.12f} | {float(row[a+"_live"]):.12f} | {float(row[a+"_odds"]):.12f} |\n')
        if x['source_record']:
            lines.append('\nSource counts in NEW, repeat1..6 order:\n\n```json\n'+json.dumps(
                {k:x['source_record'][k] for k in ('prefix','episode_counts','book_counts','counts')},indent=2)+'\n```\n')
    with (HERE/'RAW.md').open('x') as f: f.write('\n'.join(lines))
    print(json.dumps(summary,sort_keys=True))


if __name__=='__main__': main()
