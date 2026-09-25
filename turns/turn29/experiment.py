#!/usr/bin/env python3
"""Turn29 writer: full pooled coverage with recipient-local permission."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(272, 280))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('local_full', 'global_full', 'pooled_full', 'bank_local', 'permuted_full')
FULL_ARMS = ('local_full', 'global_full', 'pooled_full', 'permuted_full')
NAMESPACE = 'netta-local-pooled-permission-v1'
N = 16384
MOVE = 8192
EARLY = 4096
HORIZONS = (1024, EARLY, MOVE, N)

spec = importlib.util.spec_from_file_location('turn29_parent_turn28', ROOT/'turns/turn28/experiment.py')
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
    own = ('PROTOCOL.md','INTERFACE.md','experiment.py','verify.py','calibrate.c',
           'calibrate','episode','turn28-bank','Makefile','source_check.py')
    inherited = ('turns/turn28/experiment.py','turns/turn28/verify.py','turns/turn28/bank.c',
        'turns/turn28/bank','turns/turn13/experiment.py','turns/turn13/episode.c',
        'turns/turn22/experiment.py','byte_recurrence/frontend.c','byte_recurrence/frontend.h',
        'portable_recurrence/recurrence.c','portable_recurrence/recurrence.h',
        'court4/transfer4_confirm_core.c')
    return ['turns/turn29/'+name for name in own] + list(inherited)


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


def rotate_counts(counts):
    return [counts[0]] + counts[2:] + counts[1:2]


def build_memory(tapes):
    archives, meta = p28.build_memory(tapes)
    children = [(rule['left'], rule['right']) for rule in meta['rules']]
    permuted = []
    for record in meta['selected']:
        changed = dict(record)
        changed['counts'] = rotate_counts(record['counts'])
        permuted.append(changed)
    archives['permuted_full.bin'] = t13.branch_archive(children, permuted)
    assert len(archives['permuted_full.bin']) == len(archives['pooled_full.bin']) <= 528
    meta['bytes']['permuted_full.bin'] = len(archives['permuted_full.bin'])
    meta['turn29'] = dict(full_records=len(meta['selected']), binary_prior=[1,7],
                          share='2^-10', portable_bytes=len(archives['pooled_full.bin']))
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
    pipe_trace([str(HERE/'calibrate'), 'predict', str(memory/'pooled_full.bin'),
                str(memory/'permuted_full.bin')], raw, folder/(regime+'.full.tsv.gz'))
    pipe_trace([str(HERE/'turn28-bank'), 'predict', str(memory/'bank.bin'),
                str(memory/'pooled_full.bin'), str(memory/'pooled_small.bin'),
                str(memory/'permuted.bin')], raw, folder/(regime+'.bank.tsv.gz'))
    return summarize(world, regime)


def blank():
    return dict(gain=0.0, early=0.0, tail=0.0, minimum=0.0, peak=0.0,
                drawdown=0.0, activation=None, horizons={})


def summarize(world, regime):
    stats = {arm:blank() for arm in ARMS}
    exact = dict(new=True, equal=True, inactive=True, history=True, cross_trace=True)
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    full_path = HERE/'results'/f'world{world}'/(regime+'.full.tsv.gz')
    bank_path = HERE/'results'/f'world{world}'/(regime+'.bank.tsv.gz')
    max_norm = 0.0
    best = worst = bank_best = bank_worst = None
    with gzip.open(full_path, 'rt') as fa, gzip.open(bank_path, 'rt') as fb:
        full_rows = csv.DictReader(fa, delimiter='\t')
        bank_rows = csv.DictReader(fb, delimiter='\t')
        for t in range(N):
            full = next(full_rows, None); bank = next(bank_rows, None)
            assert full is not None and bank is not None
            common = ('t','k','heads','cold_heads','truth','rank','history','logcold')
            exact['cross_trace'] &= all(full[key] == bank[key] for key in common)
            assert int(full['t']) == t and int(full['truth']) == raw[t]
            exact['history'] &= full['history'] == bank['history']
            cold = float(full['logcold']); rank = int(full['rank'])
            max_norm = max(max_norm, float(full['max_norm_error']), float(bank['max_norm_error']))
            rows = {arm:full for arm in FULL_ARMS}
            rows['bank_local'] = bank
            fields = {arm:arm for arm in FULL_ARMS}; fields['bank_local'] = 'local'
            for arm, row in rows.items():
                field = fields[arm]
                live, candidate = float(row[field+'_live']), float(row[field+'_candidate'])
                s = stats[arm]
                s['gain'] += live-cold
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
                assert abs(s['gain']-float(row[field+'_gain_after'])) <= 1e-7
                if int(row[field+'_activated_after']):
                    assert s['activation'] is None
                    s['activation'] = t+1
                if t+1 in HORIZONS: s['horizons'][str(t+1)] = s['gain']
                if not rank: exact['new'] &= candidate == cold and live == cold
                if candidate == cold: exact['equal'] &= live == cold
                if not int(row[field+'_active_before']): exact['inactive'] &= live == cold
            if t >= MOVE:
                delta = float(full['local_full_live'])-float(full['pooled_full_live'])
                sample = dict(full, difference=delta, raw_hex=raw[max(0,t-16):t+17].hex())
                if best is None or delta > best['difference']: best = sample
                if worst is None or delta < worst['difference']: worst = sample
                bank_delta = float(full['local_full_live'])-float(bank['local_live'])
                bank_sample = dict(full, bank_local_live=bank['local_live'],
                                   difference=bank_delta, raw_hex=raw[max(0,t-16):t+17].hex())
                if bank_best is None or bank_delta > bank_best['difference']: bank_best = bank_sample
                if bank_worst is None or bank_delta < bank_worst['difference']: bank_worst = bank_sample
        assert next(full_rows, None) is None and next(bank_rows, None) is None
    for s in stats.values():
        s['early'] = s['horizons'][str(EARLY)]
        s['tail'] = s['gain']-s['horizons'][str(MOVE)]
    return dict(world=world, regime=regime, arms=stats, max_norm_error=max_norm,
                exactness=exact, raw_help=best, raw_harm=worst,
                bank_help=bank_best, bank_harm=bank_worst)


def verdict(lives):
    lookup = {(life['world'],life['regime']):life for life in lives}
    mean = lambda seq: math.fsum(seq)/len(seq)
    vals = lambda regime,arm,key: [lookup[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime,other,key: [a-b for a,b in zip(vals(regime,'local_full',key),
                                                          vals(regime,other,key))]
    early = vals('recombined','local_full','early')
    full = vals('recombined','local_full','gain')
    e_pool = diffs('recombined','pooled_full','early')
    f_bank = diffs('recombined','bank_local','gain')
    p_tail = diffs('partial','pooled_full','tail')
    p_whole = diffs('partial','pooled_full','gain')
    e_global = diffs('recombined','global_full','early')
    e_perm = diffs('recombined','permuted_full','early')
    pooled_mean = mean(vals('recombined','pooled_full','gain'))
    q = dict(early_gain_per_byte=mean(early)/EARLY,
        early_positive=sum(x>0 for x in early),
        early_vs_pooled=mean(e_pool), early_wins_pooled=sum(x>0 for x in e_pool),
        full_retention=mean(full)/max(pooled_mean,1e-300),
        full_positive=sum(x>0 for x in full),
        full_vs_bank=mean(f_bank), full_wins_bank=sum(x>0 for x in f_bank),
        partial_tail_vs_pooled=mean(p_tail), partial_tail_wins_pooled=sum(x>0 for x in p_tail),
        partial_whole_vs_pooled=mean(p_whole),
        early_vs_global=mean(e_global), early_wins_global=sum(x>0 for x in e_global),
        early_vs_permuted=mean(e_perm), early_wins_permuted=sum(x>0 for x in e_perm))
    conditions = dict(
        T1=q['early_gain_per_byte']>=.005 and q['early_positive']>=6,
        T2=q['early_vs_pooled']>1 and q['early_wins_pooled']>=5,
        T3=pooled_mean>0 and q['full_retention']>=.95 and q['full_positive']==8 and
           q['full_vs_bank']>1 and q['full_wins_bank']>=5,
        T4=q['partial_tail_vs_pooled']>1 and q['partial_tail_wins_pooled']>=5 and
           q['partial_whole_vs_pooled']>=0,
        T5=q['early_vs_global']>1 and q['early_wins_global']>=5 and
           q['early_vs_permuted']>1 and q['early_wins_permuted']>=5)
    tables = {regime:{arm:{key:mean(vals(regime,arm,key)) for key in ('early','gain','tail')}
              for arm in ARMS} for regime in REGIMES}
    validity = dict(
        archive=all(p.stat().st_size<=528 for p in (HERE/'memory').rglob('*.bin')),
        source_counts=True,
        normalization=all(x['max_norm_error']<=1e-8 for x in lives),
        exact_quotes=all(all(x['exactness'].values()) for x in lives),
        bounds=all(s['minimum']>=-1-1e-7 and s['drawdown']<=16+1e-7
                   for x in lives for s in x['arms'].values()))
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        quantities=q, conditions=conditions, validity=validity,
        material_pass=all(conditions.values()), independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json'); check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives)
    save(HERE/'RESULT.json', result)
    print(json.dumps({key:result[key] for key in ('conditions','quantities','validity','material_pass')},
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    args = parser.parse_args()
    if args.stage != 'freeze': check_freeze()
    globals()[args.stage]()
