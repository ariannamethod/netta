#!/usr/bin/env python3
"""Direct turn34 G4 recount without importing either turn34 program."""
import argparse
import csv
import gzip
import json
import math
from pathlib import Path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('turn34',type=Path)
    args=parser.parse_args()
    values=[]
    for world in range(304,312):
        path=args.turn34/'results'/f'world{world}'/'switched.turn34.tsv.gz'
        total=0.0
        with gzip.open(path,'rt') as stream:
            rows=csv.DictReader(stream,delimiter='\t')
            for row in rows:
                if int(row['t'])>=8192:
                    total+=float(row['sel4_live'])-float(row['earned2_live'])
        values.append(total)
    answer=dict(per_world=values,mean=math.fsum(values)/len(values),
                wins=sum(value>0 for value in values),price_bits=6.72,
                pass_gate=math.fsum(values)/len(values)>6.72 and
                          sum(value>0 for value in values)>=5)
    print(json.dumps(answer,indent=2,sort_keys=True))


if __name__=='__main__':main()
