#!/usr/bin/env python3
"""Exercise the complete writer/reader interface on already-open turn36 world320."""
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import sys
from decimal import localcontext

HERE=Path(__file__).resolve().parent
ASTRA=Path('/Users/ataeff/arianna-codex/repos/netta-astra-turn36-20261005/turns/turn36')


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec); sys.modules[name]=module
    spec.loader.exec_module(module)
    return module


def main():
    dry=HERE/'.build/preflight320'
    dry.mkdir()
    (dry/'data').mkdir()
    (dry/'data/world320').symlink_to(ASTRA/'data/world320',target_is_directory=True)
    memory=dry/'memory/world320'; memory.mkdir(parents=True)
    for name in ('bank4_full.bin','bank2_full.bin','pooled_full.bin','null4_full.bin'):
        shutil.copyfile(ASTRA/'memory/world320'/name,memory/name)
    meta=json.loads((ASTRA/'memory/world320/BOOKS.json').read_text())
    meta['bytes']={p.name:p.stat().st_size for p in memory.iterdir()}
    with (memory/'BOOKS.json').open('x') as f: json.dump(meta,f)
    (dry/'.build').mkdir()
    (dry/'.build/direct_router').symlink_to(HERE/'.build/direct_router')
    writer=load('preflight_writer37',HERE/'experiment.py'); writer.HERE=dry
    saved=writer.run_target((320,'recombined'))
    reader=load('preflight_reader37',HERE/'verify.py')
    reader.HERE=dry; reader.p28.HERE=dry
    with localcontext() as ctx:
        ctx.prec=50
        tables,book=reader.check_books(320)
        _,error=reader.check_life(320,'recombined',tables,saved)
    receipt=dict(world=320,namespace='already-open Astra36 input',forecasts=16384*7,
                 max_error=error,source_counts=book['counters'],purpose='prefreeze interface check')
    with (HERE/'PREFLIGHT.json').open('x') as f:
        json.dump(receipt,f,indent=2,sort_keys=True); f.write('\n')
    print(json.dumps(receipt,sort_keys=True))


if __name__=='__main__': main()
