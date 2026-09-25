#!/usr/bin/env python3
"""Predataset fixtures for turn30 archives, routers and the shared admission."""
from decimal import localcontext
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
ARMS = ('bank3', 'pooled2', 'permuted2', 'cold')


def need(value, label):
    if not value: raise AssertionError(label)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def rotate(counts):
    return [counts[0]]+counts[2:]+counts[1:2]


def main():
    writer = load('turn30_fixture_writer', HERE/'experiment.py')
    reader = load('turn30_fixture_reader', HERE/'verify.py')
    parser = load('turn30_fixture_parser', ROOT/'turns/turn28/source_check.py')
    for name in ('generate','extract','learn','evaluate','run_target'):
        setattr(writer, name, lambda *a, **k: (_ for _ in ()).throw(AssertionError('world access')))

    tapes = [bytes([0,0,0,1])*128+bytes([6,6]), bytes([0,0,0,1])*128,
             bytes([0,0,0,2])*128, bytes([0,0,0,2])*128]
    archives, meta = writer.build_memory(tapes)
    need(set(archives) == {'bank.bin','permuted.bin','pooled_full.bin','pooled_small.bin',
                           'permuted_small.bin','permuted_full.bin'}, 'six archives')
    children, full = parser.load_archive(archives['pooled_full.bin'], False)
    small_children, small = parser.load_archive(archives['pooled_small.bin'], False)
    perm_small_children, perm_small = parser.load_archive(archives['permuted_small.bin'], False)
    perm_full_children, perm_full = parser.load_archive(archives['permuted_full.bin'], False)
    bank_children, bank = parser.load_archive(archives['bank.bin'], True)
    need(children == small_children == perm_small_children == perm_full_children == bank_children,
         'one shared dictionary')
    need(small == full[:len(small)] and len(small) == len(bank) > 0, 'one address set')
    for stored, pooled in ((perm_small, small), (perm_full, full)):
        need(len(stored) == len(pooled), 'null archive length')
        for a, b in zip(stored, pooled):
            need(a['prefix'] == b['prefix'] and a['rule_id'] == b['rule_id'], 'address identity')
            need(a['counts'] == rotate(b['counts']), 'pooled count permutation')
    for i, record in enumerate(bank):
        a, b = record['counts'][:7], record['counts'][7:]
        need([x+y for x, y in zip(a, b)] == small[i]['counts'], 'A+B pooled exactly')
        need(record['prefix'] == small[i]['prefix'], 'bank/pooled address identity')
    extra = len(archives['bank.bin'])-len(archives['pooled_small.bin'])
    need(extra == 16*len(bank) > 0, 'distinctness costs one count vector per address')
    need(len(archives['permuted_small.bin']) == len(archives['pooled_small.bin']) and
         len(archives['permuted_full.bin']) == len(archives['pooled_full.bin']) <= 528,
         'portable archive sizes')

    rows = [json.loads(line) for line in subprocess.run(
        [str(HERE/'router3'), '--fixture'], text=True, capture_output=True, check=True
    ).stdout.splitlines()]
    max_error = 0.0
    with localcontext() as context:
        context.prec = 50
        three = [reader.Router3(), reader.Router3()]
        pooled = [reader.BinaryRouter(), reader.BinaryRouter()]
        permuted = [reader.BinaryRouter(), reader.BinaryRouter()]
        outer = {arm:reader.SharedOuter() for arm in ARMS}
        shadow, admitted, admissions = 0.0, False, []
        for row in rows:
            record, cold, step = row['record'], row['cold'], row['t']
            r3 = three[record] if record >= 0 else reader.Router3()
            rp = pooled[record] if record >= 0 else reader.BinaryRouter()
            rq = permuted[record] if record >= 0 else reader.BinaryRouter()
            for i, weight in enumerate(r3.weights):
                max_error = max(max_error, abs(float(weight)-row['bank3_w_before'][i]))
            max_error = max(max_error, abs(float(rp.memory)-row['pooled2_w_before']),
                            abs(float(rq.memory)-row['permuted2_w_before']))
            candidates = {'bank3':r3.quote(cold, row['a'], row['b']),
                          'pooled2':rp.quote(cold, row['pooled']),
                          'permuted2':rq.quote(cold, row['perm']), 'cold':cold}
            for arm in ARMS:
                max_error = max(max_error, abs(float(candidates[arm])-row[arm+'_candidate']))
            max_error = max(max_error, abs(shadow-row['admission_shadow_before']))
            need(int(admitted) == row['admitted_before'], 'fixture shared admission state')
            shadow += float(candidates['pooled2'])-cold
            admit = not admitted and shadow >= 32.0
            admitted = admitted or admit
            need(int(admitted) == row['admitted_after'], 'fixture admission decision')
            for arm in ARMS:
                paid = outer[arm].step(step, cold, candidates[arm], admit)
                need(paid['activated_after'] == row[arm+'_activated'], 'fixture activation '+arm)
                max_error = max(max_error, abs(float(paid['live'])-row[arm+'_live']),
                                abs(float(paid['gain_after'])-row[arm+'_gain']))
            if admit: admissions.append(step)
            if record >= 0:
                three[record].observe(cold, row['a'], row['b'])
                pooled[record].observe(row['pooled'], candidates['pooled2'])
                permuted[record].observe(row['perm'], candidates['permuted2'])
            after = three[record].weights if record >= 0 else list(reader.PRIOR3)
            for i, weight in enumerate(after):
                max_error = max(max_error, abs(float(weight)-row['bank3_w_after'][i]))
            max_error = max(max_error,
                abs(float(pooled[record].memory if record >= 0 else reader.PRIOR2)
                    -row['pooled2_w_after']),
                abs(float(permuted[record].memory if record >= 0 else reader.PRIOR2)
                    -row['permuted2_w_after']))
    need(len(admissions) == 1, 'fixture admits exactly once')
    need(len({row['t'] for row in rows if row['bank3_activated']} |
             {row['t'] for row in rows if row['pooled2_activated']} |
             {row['t'] for row in rows if row['permuted2_activated']} |
             {row['t'] for row in rows if row['cold_activated']}) == 1,
         'fixture arms admit together')
    need(max_error <= 1e-12 and all(row['max_norm_error'] == 0 for row in rows),
         'fixture arithmetic and normalization')
    need(rows[-1]['cold_gain'] == 0 and rows[-1]['bank3_gain'] != 0, 'cold baseline is the zero')

    print(json.dumps(dict(source_fixture_pass=True, worlds_generated=0, recipient_predictions=0,
        fixture_rows=len(rows), max_router_error=max_error, shared_admission_step=admissions[0],
        rules=len(children), full_records=len(full), bank_records=len(bank),
        extra_portable_bytes=extra,
        archive_bytes={name:len(data) for name, data in archives.items()},
        archive_sha256={name:hashlib.sha256(data).hexdigest() for name, data in archives.items()},
        writer_sha256=hashlib.sha256((HERE/'experiment.py').read_bytes()).hexdigest(),
        reader_sha256=hashlib.sha256((HERE/'verify.py').read_bytes()).hexdigest()),
        indent=2, sort_keys=True))


if __name__ == '__main__': main()
