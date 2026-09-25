#!/usr/bin/env python3
"""One fresh batch for the preregistered witness latch with return."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import io
import json
import math
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(248, 256))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('slow', 'fast', 'witness', 'latch')
NAMESPACE = 'netta-witness-latch-v1'
N, MOVE, TOL = 16384, 8192, 1e-7
spec = importlib.util.spec_from_file_location('turn25_generator', ROOT/'turns/turn22/experiment.py')
parent = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = parent
spec.loader.exec_module(parent)
parent.HERE, parent.REPO, parent.WORLDS, parent.NAMESPACE = HERE, ROOT, WORLDS, NAMESPACE
parent.t13.HERE, parent.t13.REPO = HERE, ROOT
parent.t13.WORLDS, parent.t13.NAMESPACE = WORLDS, NAMESPACE

FROZEN = tuple('turns/turn25/'+name for name in (
    'PROTOCOL.md', 'INTERFACE.md', 'experiment.py', 'verify.py', 'latch.c',
    'latch.h', 'replay.c', 'fixture.c', 'Makefile', 'episode',
    'authority_replay', 'fixture', 'preflight.py')) + (
    'turns/turn22/experiment.py', 'turns/turn22/authority.c', 'turns/turn22/authority.h',
    'turns/turn13/experiment.py', 'turns/turn13/episode.c',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c')


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as out:
        json.dump(obj, out, indent=2, sort_keys=True, allow_nan=False)
        out.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): digest(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifests(*names):
    for name in names:
        for rel, wanted in json.loads((HERE/name).read_text()).items():
            assert digest(HERE/rel) == wanted, str(HERE/rel)


def freeze():
    assert all(not (HERE/x).exists() for x in ('data', 'memory', 'results'))
    save(HERE/'FREEZE.json', dict(created_utc=time.time(), namespace=NAMESPACE,
         worlds=WORLDS, regimes=REGIMES, modes=MODES,
         files={p: digest(ROOT/p) for p in FROZEN}))


def check_freeze():
    f = json.loads((HERE/'FREEZE.json').read_text())
    assert f['namespace'] == NAMESPACE and f['worlds'] == list(WORLDS)
    assert f['regimes'] == list(REGIMES) and f['modes'] == list(MODES)
    for name, wanted in f['files'].items():
        assert digest(ROOT/name) == wanted, str(ROOT/name)


def stats():
    return dict(gain=0., tail=0., early=0., minimum=0., peak=0., drawdown=0.,
                activation=None, horizons={}, slow_count=0, fast_count=0, tail_fast_count=0)


def replay(item):
    w, regime = item['world'], item['regime']
    source = HERE/'results'/f'world{w}'/(regime+'.tsv.gz')
    with gzip.open(source, 'rt') as stream:
        raw = list(csv.DictReader(stream, delimiter='\t'))
    assert len(raw) == N
    payload = ''.join(' '.join((r['t'], r['logcold'], r['episode_candidate'],
                               r['episode_matchedL']))+'\n' for r in raw)
    proc = subprocess.run([str(HERE/'authority_replay')], input=payload.encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    target = HERE/'results/authority'/f'world{w}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(proc.stdout)
    target.with_suffix('.log').write_bytes(proc.stderr)
    modes = {m: stats() for m in MODES}
    exact = dict.fromkeys(('admission', 'equal_price', 'inactive', 'new', 'clock', 'latch_law'), True)
    diagnostics = dict(first_set=None, first_return=None, set_count=0, return_count=0,
        sets_before_seam=0, returns_before_seam=0, first_set_after_seam=None, latch_at_seam=None)
    best = worst = None
    rows = csv.DictReader(io.StringIO(proc.stdout.decode()), delimiter='\t')
    for t, (r, src) in enumerate(zip(rows, raw, strict=True)):
        assert int(r['t']) == int(src['t']) == t
        cold, candidate = float(r['cold']), float(r['candidate'])
        matched, active, admitted = int(r['matched']), int(r['active_before']), int(r['admitted_after'])
        assert cold == float(src['logcold']) and candidate == float(src['episode_candidate'])
        assert matched == int(src['episode_matchedL'])
        exact['admission'] &= (active == int(src['episode_active_before']) and
            admitted == int(src['episode_activated_after']) and
            abs(float(r['shadow_before'])-float(src['episode_shadow_before'])) <= TOL)
        oldw, neww = float(r['w_before']), float(r['w_after'])
        oldl, newl = int(r['latch_before']), int(r['latch_after'])
        expected_w = (31./32.)*oldw+(candidate-cold) if active and matched >= 1 else oldw
        if admitted:
            expected_w = 0.
        exact['clock'] &= abs(expected_w-neww) <= 1e-10
        expected_l = (1 if neww <= -1 else 0 if neww >= 1 else oldl) if active else 0
        exact['latch_law'] &= newl == expected_l
        if active:
            exact['latch_law'] &= int(r['latch_slow_used']) == 1-oldl
        if t == MOVE:
            diagnostics['latch_at_seam'] = oldl
        if newl != oldl:
            kind = 'set' if newl else 'return'
            diagnostics[kind+'_count'] += 1
            if diagnostics['first_'+kind] is None:
                diagnostics['first_'+kind] = t
            if t < MOVE:
                diagnostics[kind+'s_before_seam'] += 1
            elif newl and diagnostics['first_set_after_seam'] is None:
                diagnostics['first_set_after_seam'] = t
        rank = int(src['rank'])
        for m, s in modes.items():
            value = float(r[m+'_live'])
            assert math.isfinite(value) and value <= 1e-8
            if m == 'slow':
                assert abs(value-float(src['episode_live'])) <= TOL
            if not active:
                exact['inactive'] &= value == cold
            if cold == candidate:
                exact['equal_price'] &= value == cold
            if rank == 0:
                exact['new'] &= candidate == cold and value == cold and src['new_exact'] == '1'
            s['gain'] += value-cold
            if t >= MOVE:
                s['tail'] += value-cold
            s['minimum'] = min(s['minimum'], s['gain'])
            s['peak'] = max(s['peak'], s['gain'])
            s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
            if admitted:
                assert s['activation'] is None
                s['activation'] = t+1
            if t+1 in (1024,4096,MOVE,N):
                s['horizons'][str(t+1)] = s['gain']
            used = int(r[m+'_slow_used'])
            s['slow_count'] += used == 1
            s['fast_count'] += used == 0
            s['tail_fast_count'] += used == 0 and t >= MOVE
        if t >= MOVE:
            sample = {k: float(r[k]) for k in ('cold','candidate','w_before','w_after')}
            sample.update(t=t, truth=int(src['truth']), rank=rank, matched=matched,
                latch_before=oldl, latch_after=newl,
                delta=float(r['latch_live'])-float(r['witness_live']))
            for m in MODES:
                for field in ('live', 'odds_before'):
                    sample[m+'_'+field] = float(r[m+'_'+field])
            if best is None or sample['delta'] > best['delta']:
                best = sample
            if worst is None or sample['delta'] < worst['delta']:
                worst = sample
    assert t+1 == N
    for s in modes.values():
        s['early'] = s['horizons']['4096']
    return dict(world=w, regime=regime, modes=modes, diagnostics=diagnostics,
        exactness=exact, max_norm_error=item['max_norm_error'],
        archive_bytes=(HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size,
        raw_help=best, raw_harm=worst)


def summarize(lives):
    by = {(x['world'],x['regime']):x for x in lives}
    mean = lambda xs: math.fsum(xs)/len(xs)
    def values(regime, mode, field):
        return [by[w,regime]['modes'][mode][field] for w in WORLDS]
    def pair(regime, mode, other, field):
        return [a-b for a,b in zip(values(regime,mode,field), values(regime,other,field))]
    bars, quantities = {}, {}
    slow_early = mean(values('recombined','slow','early'))
    slow_full = mean(values('recombined','slow','gain'))
    for m in MODES:
        lt, lw = pair('switched',m,'fast','tail'), pair('switched',m,'fast','gain')
        mf, ms = pair('moved_mid',m,'fast','tail'), pair('moved_mid',m,'slow','tail')
        full = values('recombined',m,'gain')
        er = mean(values('recombined',m,'early'))/slow_early if slow_early > 0 else None
        fr = mean(full)/slow_full if slow_full > 0 else None
        quantities[m] = dict(law_tail_vs_fast=lt, law_tail_mean=mean(lt),
            law_whole_vs_fast=lw, law_whole_mean=mean(lw), moved_tail_vs_fast=mf,
            moved_tail_mean=mean(mf), moved_tail_wins=sum(v>0 for v in mf),
            moved_tail_vs_slow=ms, moved_tail_vs_slow_mean=mean(ms),
            intact_early_retention=er, intact_full_retention=fr, intact_full_gains=full)
        bars[m] = dict(b1_law_tail_mean=mean(lt)>=-1, b2_law_whole_mean=mean(lw)>=0,
            b3_moved_tail_mean=mean(mf)>=1, b4_moved_tail_wins=sum(v>0 for v in mf)>=5,
            b5_moved_tail_vs_slow=mean(ms)>=-3, b6_intact_early_retention=er is not None and er>=.95,
            b7_intact_full_retention=fr is not None and fr>=.95, b8_intact_full_positive=all(v>0 for v in full))
    tables = {r:{m:{f:mean(values(r,m,f)) for f in ('gain','tail','early')} for m in MODES} for r in REGIMES}
    validity = dict(
        admission=all(x['exactness']['admission'] and len({s['activation'] for s in x['modes'].values()})==1 for x in lives),
        archive=all(x['archive_bytes']<=528 for x in lives),
        bounds=all(s['minimum']>=-1-TOL and s['drawdown']<=(10 if m=='fast' else 16)+TOL for x in lives for m,s in x['modes'].items()),
        normalization=all(0<=x['max_norm_error']<=1e-8 for x in lives),
        exact_quotes=all(x['exactness'][k] for x in lives for k in ('equal_price','inactive','new')),
        latch=all(x['exactness'][k] for x in lives for k in ('clock','latch_law')))
    return dict(bars=bars, quantities=quantities, tables=tables, validity=validity,
                material_pass=all(bars['latch'].values()))


def pack_result(lives):
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, modes=MODES,
        protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives,
        **summarize(lives), independent_reader_pending=True)


def evaluate():
    check_manifests('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists(), 'preserve existing results'
    pairs = [(w,r) for w in WORLDS for r in REGIMES]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        inherited = list(pool.map(lambda wr: parent.t13.run_target(*wr), pairs))
    lives = []
    for item in inherited:
        life = replay(item)
        lives.append(life)
        print(life['world'],life['regime'],life['modes']['latch']['gain'],flush=True)
    result = pack_result(lives)
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps({k:result[k] for k in ('bars','quantities','validity','material_pass')}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    if stage in ('generate','extract'):
        fn = parent.generate_world if stage == 'generate' else parent.t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(fn,WORLDS)))
        if stage == 'extract':
            save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))
    elif stage == 'learn':
        check_manifests('DATA_MANIFEST.json')
        for w in WORLDS:
            print('learned',parent.t13.learn_world(w),flush=True)
        save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))
    else:
        evaluate()


if __name__ == '__main__':
    main()
