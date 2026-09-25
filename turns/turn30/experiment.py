#!/usr/bin/env python3
"""Turn30 writer: distinct A/B histories against pooled counts, same addresses."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(280, 288))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('bank3', 'pooled2', 'permuted2', 'cold', 'full24')
GATED = ('bank3', 'pooled2', 'permuted2', 'cold')
NAMESPACE = 'netta-distinct-histories-v1'
N = 16384
MOVE = 8192
EARLY = 4096
HORIZONS = (1024, EARLY, MOVE, N)

spec = importlib.util.spec_from_file_location('turn30_parent_turn28', ROOT/'turns/turn28/experiment.py')
p28 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p28
spec.loader.exec_module(p28)
t13 = p28.t13
p22 = p28.parent
p28.HERE, p28.ROOT, p28.WORLDS, p28.REGIMES, p28.NAMESPACE = HERE, ROOT, WORLDS, REGIMES, NAMESPACE
p22.HERE, p22.REPO, p22.WORLDS, p22.REGIMES, p22.NAMESPACE = HERE, ROOT, WORLDS, REGIMES[:-1], NAMESPACE
t13.HERE, t13.REPO, t13.WORLDS, t13.REGIMES, t13.NAMESPACE = HERE, ROOT, WORLDS, ('recombined','unrelated','switched'), NAMESPACE


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            h.update(part)
    return h.hexdigest()


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): sha(p) for p in sorted(Path(folder).rglob('*')) if p.is_file()}


def check_manifest(name):
    for rel, wanted in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/rel) == wanted, rel


def frozen_names():
    own = ('PROTOCOL.md','INTERFACE.md','experiment.py','verify.py','source_check.py',
           'probes.py','router3.c','router3','episode','turn29-calibrate','Makefile')
    inherited = ('turns/turn29/experiment.py','turns/turn29/verify.py','turns/turn29/calibrate.c',
        'turns/turn29/calibrate','turns/turn28/experiment.py','turns/turn28/verify.py',
        'turns/turn28/source_check.py','turns/turn28/bank.c','turns/turn28/bank',
        'turns/turn13/experiment.py','turns/turn13/episode.c','turns/turn22/experiment.py',
        'byte_recurrence/frontend.c','byte_recurrence/frontend.h',
        'portable_recurrence/recurrence.c','portable_recurrence/recurrence.h',
        'court4/transfer4_confirm_core.c')
    return ['turns/turn30/'+name for name in own] + list(inherited)


def freeze():
    for folder in ('data','memory','results'):
        assert not (HERE/folder).exists(), folder
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
        arms=ARMS, files={name:sha(ROOT/name) for name in frozen_names()}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES) and frozen['arms'] == list(ARMS)
    for name, wanted in frozen['files'].items():
        assert sha(ROOT/name) == wanted, name


def generate():
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(p28.generate_world, WORLDS))
    save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(t13.extract_world, WORLDS))
    save(HERE/'EXTRACT_MANIFEST.json', manifest(HERE/'data'))


def build_memory(tapes):
    """Turn28 archives verbatim, plus the pooled nulls of both address sets."""
    archives, meta = p28.build_memory(tapes)
    children = [(rule['left'], rule['right']) for rule in meta['rules']]
    bank_records = len(meta['bank_selected'])
    archives['permuted_small.bin'] = t13.branch_archive(
        children, t13.rotate_records(meta['selected'][:bank_records]))
    archives['permuted_full.bin'] = t13.branch_archive(
        children, t13.rotate_records(meta['selected']))
    assert len(archives['permuted_small.bin']) == len(archives['pooled_small.bin'])
    assert len(archives['permuted_full.bin']) == len(archives['pooled_full.bin']) <= 528
    for name in ('permuted_small.bin','permuted_full.bin'):
        meta['bytes'][name] = len(archives[name])
    meta['turn30'] = dict(bank_records=bank_records, full_records=len(meta['selected']),
        three_prior=[0.125,0.4375,0.4375], binary_prior=[0.125,0.875], share='2^-10',
        portable_bytes={'bank3':len(archives['bank.bin']),
                        'pooled2':len(archives['pooled_small.bin']),
                        'permuted2':len(archives['permuted_small.bin']),
                        'cold':0, 'full24':len(archives['pooled_full.bin'])},
        extra_bytes=len(archives['bank.bin'])-len(archives['pooled_small.bin']))
    return archives, meta


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for world in WORLDS:
        tapes, _ = t13.source_tapes(world)
        archives, meta = build_memory(tapes)
        folder = HERE/'memory'/f'world{world}'
        folder.mkdir(parents=True, exist_ok=False)
        for name, data in archives.items():
            (folder/name).write_bytes(data)
        save(folder/'BOOKS.json', meta)
        print('learned', world, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def pipe_trace(command, raw_path, output_path):
    with raw_path.open('rb') as source, gzip.open(output_path, 'xb') as out:
        proc = subprocess.Popen(command, stdin=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while chunk := proc.stdout.read(1 << 20):
            out.write(chunk)
        stderr = proc.stderr.read()
        rc = proc.wait()
    output_path.with_suffix('.stderr').write_bytes(stderr)
    assert rc == 0, (command, rc, stderr.decode())


def run_target(pair):
    world, regime = pair
    folder = HERE/'results'/f'world{world}'
    folder.mkdir(parents=True, exist_ok=True)
    memory = HERE/'memory'/f'world{world}'
    raw = HERE/'data'/f'world{world}'/(regime+'.bin')
    pipe_trace([str(HERE/'router3'), 'predict', str(memory/'bank.bin'),
                str(memory/'pooled_small.bin'), str(memory/'permuted_small.bin')],
               raw, folder/(regime+'.turn30.tsv.gz'))
    pipe_trace([str(HERE/'turn29-calibrate'), 'predict', str(memory/'pooled_full.bin'),
                str(memory/'permuted_full.bin')], raw, folder/(regime+'.full24.tsv.gz'))
    return summarize(world, regime)


def blank():
    return dict(gain=0.0, early=0.0, tail=0.0, minimum=0.0, peak=0.0,
                drawdown=0.0, activation=None, horizons={})


def summarize(world, regime):
    stats = {arm:blank() for arm in ARMS}
    exact = dict(new=True, equal=True, inactive=True, history=True, cross_trace=True,
                 shared_admission=True, router_weights=True)
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    turn30_path = HERE/'results'/f'world{world}'/(regime+'.turn30.tsv.gz')
    full24_path = HERE/'results'/f'world{world}'/(regime+'.full24.tsv.gz')
    max_norm = 0.0
    best = worst = None
    with gzip.open(turn30_path, 'rt') as fa, gzip.open(full24_path, 'rt') as fb:
        turn30_rows = csv.DictReader(fa, delimiter='\t')
        full24_rows = csv.DictReader(fb, delimiter='\t')
        for t in range(N):
            row = next(turn30_rows, None); full = next(full24_rows, None)
            assert row is not None and full is not None
            common = ('t','k','heads','cold_heads','truth','rank','history','logcold')
            exact['cross_trace'] &= all(row[key] == full[key] for key in common)
            assert int(row['t']) == t and int(row['truth']) == raw[t]
            exact['history'] &= row['history'] == full['history']
            cold = float(row['logcold']); rank = int(row['rank'])
            max_norm = max(max_norm, float(row['max_norm_error']), float(full['max_norm_error']))
            for field in ('bank3_w_before','bank3_w_after'):
                weights = [float(x) for x in row[field].split(',')]
                exact['router_weights'] &= (len(weights) == 3 and all(x > 0 for x in weights)
                                            and abs(math.fsum(weights)-1) <= 1e-10)
            for field in ('pooled2_w_before','pooled2_w_after',
                          'permuted2_w_before','permuted2_w_after'):
                exact['router_weights'] &= 0.0 < float(row[field]) < 1.0
            active = {int(row[arm+'_active_before']) for arm in GATED}
            fired = {int(row[arm+'_activated_after']) for arm in GATED}
            exact['shared_admission'] &= (len(active) == 1 and len(fired) == 1 and
                                          active == {int(row['admitted_before'])})
            for arm in ARMS:
                source, field = (full, 'local_full') if arm == 'full24' else (row, arm)
                live = float(source[field+'_live']); cand = float(source[field+'_candidate'])
                s = stats[arm]
                s['gain'] += live-cold
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
                assert abs(s['gain']-float(source[field+'_gain_after'])) <= 1e-7
                if int(source[field+'_activated_after']):
                    assert s['activation'] is None
                    s['activation'] = t+1
                if t+1 in HORIZONS: s['horizons'][str(t+1)] = s['gain']
                if not rank: exact['new'] &= cand == cold and live == cold
                if cand == cold: exact['equal'] &= live == cold
                if not int(source[field+'_active_before']): exact['inactive'] &= live == cold
            delta = float(row['bank3_live'])-float(row['pooled2_live'])
            sample = dict(row, difference=delta, raw_hex=raw[max(0,t-16):t+17].hex())
            if best is None or delta > best['difference']: best = sample
            if worst is None or delta < worst['difference']: worst = sample
        assert next(turn30_rows, None) is None and next(full24_rows, None) is None
    for s in stats.values():
        s['early'] = s['horizons'][str(EARLY)]
        s['tail'] = s['gain']-s['horizons'][str(MOVE)]
    return dict(world=world, regime=regime, arms=stats, max_norm_error=max_norm,
                exactness=exact, raw_help=best, raw_harm=worst)


def mean(seq): return math.fsum(seq)/len(seq)


def verdict(lives):
    lookup = {(life['world'],life['regime']):life for life in lives}
    vals = lambda regime,arm,key: [lookup[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime,a,b,key: [x-y for x,y in zip(vals(regime,a,key),vals(regime,b,key))]
    books = {world: json.loads((HERE/'memory'/f'world{world}'/'BOOKS.json').read_text())['turn30']
             for world in WORLDS}
    extra = sorted({books[world]['extra_bytes'] for world in WORLDS})
    portable = {arm: sorted({books[world]['portable_bytes'][arm] for world in WORLDS})
                for arm in ARMS}

    d1 = diffs('partial','bank3','pooled2','early')
    pooled_early = mean(vals('recombined','pooled2','early'))
    pooled_full = mean(vals('recombined','pooled2','gain'))
    bank_early = mean(vals('recombined','bank3','early'))
    bank_full = mean(vals('recombined','bank3','gain'))
    excess = {regime:{key: mean(vals(regime,'permuted2',key))-mean(vals(regime,'pooled2',key))
                      for key in ('early','gain','tail')} for regime in REGIMES}
    per_byte = {regime:{key: mean(diffs(regime,'bank3','pooled2',key))/extra[0]
                        for key in ('early','gain','tail')} for regime in REGIMES}
    shared_prefix_identity = all(
        lookup[w,regime]['arms'][arm]['early'] == lookup[w,'recombined']['arms'][arm]['early']
        for w in WORLDS for regime in ('partial','switched','moved_mid') for arm in ARMS)
    admissions = {arm: sum(lookup[w,r]['arms'][arm]['activation'] is not None
                           for w in WORLDS for r in REGIMES) for arm in ARMS}
    identical = all(len({lookup[w,r]['arms'][arm]['activation'] for arm in GATED}) == 1
                    for w in WORLDS for r in REGIMES)
    unrelated = [dict(world=w, arm=arm, activation=lookup[w,'unrelated']['arms'][arm]['activation'],
                      gain=lookup[w,'unrelated']['arms'][arm]['gain'])
                 for w in WORLDS for arm in ARMS
                 if lookup[w,'unrelated']['arms'][arm]['activation'] is not None]

    q = dict(
        d1_partial_early_vs_pooled2=mean(d1), d1_partial_early_wins=sum(x > 0 for x in d1),
        d1_bank3_partial_early=mean(vals('partial','bank3','early')),
        d1_pooled2_partial_early=mean(vals('partial','pooled2','early')),
        d2_extra_portable_bytes=extra, d2_portable_bytes=portable,
        d2_bits_per_extra_byte=per_byte,
        d3_recombined_early_retention=bank_early/max(pooled_early,1e-300),
        d3_recombined_full_retention=bank_full/max(pooled_full,1e-300),
        d3_pooled2_recombined_early=pooled_early, d3_pooled2_recombined_full=pooled_full,
        d3_bank3_recombined_early=bank_early, d3_bank3_recombined_full=bank_full,
        d4_permuted2_excess=excess,
        d4_max_gain_excess=max(excess[regime]['gain'] for regime in REGIMES),
        partial_tail_vs_pooled2=mean(diffs('partial','bank3','pooled2','tail')),
        partial_tail_wins=sum(x > 0 for x in diffs('partial','bank3','pooled2','tail')),
        partial_whole_vs_pooled2=mean(diffs('partial','bank3','pooled2','gain')),
        full24_recombined_early=mean(vals('recombined','full24','early')),
        full24_recombined_full=mean(vals('recombined','full24','gain')),
        full24_partial_early=mean(vals('partial','full24','early')),
        shared_prefix_identity=shared_prefix_identity,
        unrelated_admissions=unrelated, admitted_arms=admissions)

    validity = dict(
        archive=all(p.stat().st_size <= 528 for p in (HERE/'memory').rglob('*.bin')),
        source_counts=True,
        normalization=all(x['max_norm_error'] <= 1e-8 for x in lives),
        exact_quotes=all(all(x['exactness'].values()) for x in lives),
        bounds=all(s['minimum'] >= -1-1e-7 and s['drawdown'] <= 16+1e-7
                   for x in lives for s in x['arms'].values()),
        identical_admissions=identical,
        unrelated_disclosed=True)
    conditions = dict(
        D1=q['d1_partial_early_vs_pooled2'] > 0 and q['d1_partial_early_wins'] >= 5,
        D2=extra[0] > 0 and len(extra) == 1 and set(per_byte) == set(REGIMES),
        D3=pooled_early > 0 and pooled_full > 0 and
           q['d3_recombined_early_retention'] >= .95 and
           q['d3_recombined_full_retention'] >= .95,
        D4=all(excess[regime]['gain'] <= 0 for regime in REGIMES),
        D5=all(validity.values()),
        D6=False)
    material = conditions['D1'] and conditions['D3'] and conditions['D4'] and conditions['D5']
    tables = {regime:{arm:{key:mean(vals(regime,arm,key)) for key in ('early','gain','tail')}
              for arm in ARMS} for regime in REGIMES}
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS, gated_arms=GATED,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        quantities=q, conditions=conditions, validity=validity,
        material_pass=material, gate_pass=material and conditions['D6'],
        independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json'); check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives)
    save(HERE/'RESULT.json', result)
    print(json.dumps({key:result[key] for key in
                      ('conditions','validity','material_pass','gate_pass')},
                     indent=2, sort_keys=True))
    print(json.dumps({key:result['quantities'][key] for key in sorted(result['quantities'])
                      if not isinstance(result['quantities'][key], list)},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    args = parser.parse_args()
    if args.stage != 'freeze': check_freeze()
    globals()[args.stage]()
