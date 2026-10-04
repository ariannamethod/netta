#!/usr/bin/env python3
"""Exercise the complete writer/reader interface on already-open world304."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import sys
from decimal import localcontext

HERE=Path(__file__).resolve().parent
DON=Path('/Users/ataeff/arianna/netta-don-turn34-20261003/turns/turn34')


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec); sys.modules[name]=module
    spec.loader.exec_module(module)
    return module


def main():
    dry=HERE/'.build/preflight304'
    dry.mkdir()
    (dry/'data').mkdir()
    (dry/'data/world304').symlink_to(DON/'data/world304',target_is_directory=True)
    memory=dry/'memory/world304'; memory.mkdir(parents=True)
    for name in ('bank4_full.bin','bank2_full.bin','pooled_full.bin'):
        shutil.copyfile(DON/'memory/world304'/name,memory/name)
    data=bytearray((memory/'bank4_full.bin').read_bytes())
    nr,nb=struct.unpack_from('<II',data,8)
    for j in range(nb):
        start=16+4*nr+60*j+4
        v=list(struct.unpack_from('<28H',data,start))
        for p in (0,2):
            for r in (1,3,5):
                a,b=p*7+r,(p+1)*7+r; v[a],v[b]=v[b],v[a]
        struct.pack_into('<28H',data,start,*v)
    with (memory/'null4_full.bin').open('xb') as f: f.write(data)
    meta=json.loads((DON/'memory/world304/BOOKS.json').read_text())
    meta['bytes']={p.name:p.stat().st_size for p in memory.iterdir()}
    with (memory/'BOOKS.json').open('x') as f: json.dump(meta,f)
    (dry/'.build').mkdir()
    (dry/'.build/hier_router').symlink_to(HERE/'.build/hier_router')
    writer=load('preflight_writer',HERE/'experiment.py'); writer.HERE=dry
    saved=writer.run_target((304,'recombined'))
    reader=load('preflight_reader',HERE/'verify.py')
    reader.HERE=dry; reader.p28.HERE=dry
    with localcontext() as ctx:
        ctx.prec=50
        tables,book=reader.check_books(304)
        _,error=reader.check_life(304,'recombined',tables,saved)
    receipt=dict(world=304,namespace='already-open Don34 input',forecasts=16384*6,
                 max_error=error,source_counts=book['counters'],purpose='prefreeze interface check')
    with (HERE/'PREFLIGHT.json').open('x') as f:
        json.dump(receipt,f,indent=2,sort_keys=True); f.write('\n')
    print(json.dumps(receipt,sort_keys=True))


if __name__=='__main__': main()
