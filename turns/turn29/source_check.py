#!/usr/bin/env python3
"""Predataset fixtures for turn29 archive and binary-router arithmetic."""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def need(value, label):
    if not value: raise AssertionError(label)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def main():
    writer = load('turn29_fixture_writer', HERE/'experiment.py')
    parser = load('turn29_fixture_parser', ROOT/'turns/turn28/source_check.py')
    for name in ('generate','extract','learn','evaluate','run_target'):
        setattr(writer, name, lambda *a, **k: (_ for _ in ()).throw(AssertionError('world access')))

    tapes = [bytes([0,0,0,1])*128+bytes([6,6]), bytes([0,0,0,1])*128,
             bytes([0,0,0,2])*128, bytes([0,0,0,2])*128]
    archives, meta = writer.build_memory(tapes)
    need(set(archives) == {'bank.bin','permuted.bin','pooled_full.bin',
                           'pooled_small.bin','permuted_full.bin'}, 'five archives')
    children, full = parser.load_archive(archives['pooled_full.bin'], False)
    perm_children, perm = parser.load_archive(archives['permuted_full.bin'], False)
    need(children == perm_children and len(full) == len(perm) > 0, 'full archive shape')
    for a, b in zip(full, perm):
        need(a['prefix'] == b['prefix'] and a['rule_id'] == b['rule_id'], 'address identity')
        need(b['counts'] == writer.rotate_counts(a['counts']), 'full count permutation')
    need(len(archives['pooled_full.bin']) == len(archives['permuted_full.bin']) <= 528,
         'portable full size')

    rows = [json.loads(line) for line in subprocess.run(
        [str(HERE/'calibrate'), '--fixture'], text=True, capture_output=True, check=True
    ).stdout.splitlines()]
    local = [7/8, 7/8]; global_weight = 7/8
    share, prior = 2**-10, 7/8
    max_error = 0.0
    for row in rows:
        record = row['record']
        lw = prior if record < 0 else local[record]
        need(abs(row['local_before']-lw) <= 1e-14, 'fixture local before')
        need(abs(row['global_before']-global_weight) <= 1e-14, 'fixture global before')
        cold, source = 2**row['cold'], 2**row['source']
        lq = (1-lw)*cold+lw*source
        gq = (1-global_weight)*cold+global_weight*source
        max_error = max(max_error, abs(math.log2(lq)-row['local_candidate']),
                        abs(math.log2(gq)-row['global_candidate']))
        if record >= 0:
            local[record] = (1-share)*(lw*source/lq)+share*prior
            global_weight = (1-share)*(global_weight*source/gq)+share*prior
        need(abs(row['local_after']-(prior if record < 0 else local[record])) <= 1e-14,
             'fixture local after')
        need(abs(row['global_after']-global_weight) <= 1e-14, 'fixture global after')
    need(max_error <= 1e-14 and all(row['max_norm_error'] == 0 for row in rows),
         'fixture arithmetic and normalization')

    print(json.dumps(dict(source_fixture_pass=True, worlds_generated=0,
        recipient_predictions=0, fixture_rows=len(rows), max_router_error=max_error,
        rules=len(children), full_records=len(full), archive_bytes={k:len(v) for k,v in archives.items()},
        archive_sha256={k:hashlib.sha256(v).hexdigest() for k,v in archives.items()},
        writer_sha256=hashlib.sha256((HERE/'experiment.py').read_bytes()).hexdigest()),
        indent=2, sort_keys=True))


if __name__ == '__main__': main()
