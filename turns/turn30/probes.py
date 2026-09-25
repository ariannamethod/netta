#!/usr/bin/env python3
"""Prefreeze probes: each gate tooth reddened alone, schemas both polarities."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

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


reader = load('turn30_probe_reader', HERE/'verify.py')
BASE = dict(bank3=(110.0, 420.0, 30.0), pooled2=(100.0, 400.0, 20.0),
            permuted2=(-0.5, -0.5, 0.0), cold=(0.0, 0.0, 0.0), full24=(150.0, 600.0, 40.0))


def synthetic():
    lives = []
    for index, world in enumerate(reader.WORLDS):
        for regime in reader.REGIMES:
            arms = {}
            for arm, (early, gain, tail) in BASE.items():
                arms[arm] = dict(gain=gain+index, early=early+index, tail=tail,
                                 minimum=-0.5, peak=gain+index, drawdown=2.0,
                                 activation=120 if arm == 'full24' else 100, horizons={})
            lives.append(dict(world=world, regime=regime, arms=arms, max_norm_error=1e-9,
                exactness=dict(new=True, equal=True, inactive=True, history=True,
                               cross_trace=True, shared_admission=True, router_weights=True),
                raw_help=None, raw_harm=None))
    books = [dict(portable_bytes=dict(bank3=528, pooled2=336, permuted2=336, cold=0, full24=528),
                  extra_bytes=192) for _ in reader.WORLDS]
    return lives, books


def find(lives, regime, arm):
    return [life['arms'][arm] for life in lives if life['regime'] == regime]


def probe(name, tooth, mutate):
    lives, books = synthetic()
    mutate(lives, books)
    got = reader.rebuild(lives, books)['conditions']
    need(got[tooth] is False, name+' did not redden '+tooth)
    for key, value in BASELINE.items():
        need(got[key] == value or key == tooth, name+' collateral on '+key)
    return dict(probe=name, tooth=tooth, reddened=True,
                collateral=[key for key, value in BASELINE.items()
                            if key != tooth and got[key] != value],
                conditions=got)


def d1_mean(lives, _books):
    for arms in find(lives, 'partial', 'bank3'): arms['early'] -= 20.0


def d1_wins(lives, _books):
    for index, arms in enumerate(find(lives, 'partial', 'bank3')):
        arms['early'] += 100.0 if index < 4 else -11.0


def d3_early(lives, _books):
    for pooled, bank in zip(find(lives, 'recombined', 'pooled2'),
                            find(lives, 'recombined', 'bank3')):
        bank['early'] = 0.9*pooled['early']


def d3_full(lives, _books):
    for pooled, bank in zip(find(lives, 'recombined', 'pooled2'),
                            find(lives, 'recombined', 'bank3')):
        bank['gain'] = 0.9*pooled['gain']


def d4_null(lives, _books):
    for arms in find(lives, 'switched', 'permuted2'): arms['gain'] = 10000.0


def d5_normalization(lives, _books):
    lives[0]['max_norm_error'] = 1e-7


def d5_exact(lives, _books):
    lives[3]['exactness']['new'] = False


def d5_prefix_bound(lives, _books):
    lives[5]['arms']['bank3']['minimum'] = -1.5


def d5_drawdown(lives, _books):
    lives[7]['arms']['pooled2']['drawdown'] = 17.0


def d5_admissions(lives, _books):
    lives[9]['arms']['cold']['activation'] = 101


def d2_bytes(_lives, books):
    books[0]['extra_bytes'] = 208


def schema_probes():
    result = dict(namespace=reader.NAMESPACE, worlds=list(reader.WORLDS),
        regimes=list(reader.REGIMES), arms=list(reader.ARMS), gated_arms=list(reader.GATED),
        protocol_sha256=reader.PROTOCOL_SHA, lives=[], tables={}, quantities={},
        conditions={key:True for key in ('D1','D2','D3','D4','D5','D6')},
        validity={key:True for key in ('archive','source_counts','normalization','exact_quotes',
                                       'bounds','identical_admissions','unrelated_disclosed')},
        material_pass=True, gate_pass=True, independent_reader_pending=True)
    need(reader.check_schema(result) is True, 'reader refuses the actual writer schema')
    refusals = {}
    for key in sorted(result):
        short = {name:value for name, value in result.items() if name != key}
        try:
            reader.check_schema(short)
        except AssertionError as error:
            need(key in str(error), 'refusal does not name '+key)
            refusals[key] = str(error)
            continue
        raise AssertionError('removed key accepted: '+key)
    for name, broken in (('conditions', {key:True for key in ('D1','D2','D3','D4','D5')}),
                         ('validity', {'archive':True})):
        try:
            reader.check_schema(dict(result, **{name:broken}))
        except AssertionError as error:
            refusals[name+' keys'] = str(error)
            continue
        raise AssertionError('removed '+name+' key accepted')
    return dict(accepts_writer_schema=True, refused_keys=sorted(refusals), refusals=refusals)


def output_probe():
    with tempfile.TemporaryDirectory() as folder:
        taken = Path(folder)/'VERIFY.json'
        taken.write_text('{}\n')
        done = subprocess.run([sys.executable, '-B', str(HERE/'verify.py'), '--output', str(taken)],
                              capture_output=True, text=True)
        rc = done.returncode
        need(rc == 1 and 'new output path' in done.stderr and str(taken) in done.stderr,
             'reader accepted an existing output path')
        free = Path(folder)/'fresh.json'
        missing = subprocess.run([sys.executable, '-B', str(HERE/'verify.py'), '--output', str(free)],
                                 capture_output=True, text=True)
        need('new output path' not in missing.stderr and not free.exists(),
             'reader refused a free output path')
    return dict(existing_output_rc=rc, existing_output_refusal=done.stderr.strip().splitlines()[-1],
                absent_result_rc=missing.returncode,
                absent_result_refusal=missing.stderr.strip().splitlines()[-1])


BASELINE = reader.rebuild(*synthetic())['conditions']


def main():
    need(all(BASELINE.values()), 'synthetic baseline is not green: '+json.dumps(BASELINE))
    teeth = [('d1_mean','D1',d1_mean), ('d1_wins','D1',d1_wins), ('d2_bytes','D2',d2_bytes),
             ('d3_early','D3',d3_early), ('d3_full','D3',d3_full), ('d4_null','D4',d4_null),
             ('d5_normalization','D5',d5_normalization), ('d5_exact','D5',d5_exact),
             ('d5_prefix_bound','D5',d5_prefix_bound), ('d5_drawdown','D5',d5_drawdown),
             ('d5_admissions','D5',d5_admissions)]
    print(json.dumps(dict(prefreeze_probes_pass=True, baseline=BASELINE,
        tooth_probes=[probe(*item) for item in teeth],
        schema=schema_probes(), output=output_probe(),
        reader_sha256=reader.digest(HERE/'verify.py'),
        writer_sha256=reader.digest(HERE/'experiment.py'),
        router3_sha256=reader.digest(HERE/'router3')), indent=2, sort_keys=True))


if __name__ == '__main__': main()
