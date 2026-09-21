#!/usr/bin/env python3
"""Frozen turn23 batch. This driver only stages inherited worlds and records C quotes."""
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

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(232, 240))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
MODES = ('split', 'slow', 'fast', 'h8l4', 'selfnorm')
NAMESPACE = 'netta-split-evidence-v1'
N = 16384
MOVE = N // 2
FROZEN = ('turns/turn23/PROTOCOL.md', 'turns/turn23/Makefile',
          'turns/turn23/split.c', 'turns/turn23/split',
          'turns/turn23/episode', 'turns/turn23/experiment.py',
          'turns/turn23/verify.py', 'turns/turn22/experiment.py',
          'turns/turn22/authority.c', 'turns/turn22/authority.h',
          'turns/turn22/replay.c', 'turns/turn22/authority_replay',
          'turns/turn13/episode.c', 'turns/turn13/experiment.py',
          'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
          'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
          'court4/transfer4_confirm_core.c')

spec = importlib.util.spec_from_file_location('turn23_parent', ROOT/'turns/turn22/experiment.py')
parent = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = parent
spec.loader.exec_module(parent)
parent.HERE, parent.REPO, parent.WORLDS, parent.NAMESPACE = HERE, ROOT, WORLDS, NAMESPACE
parent.t13.HERE, parent.t13.REPO = HERE, ROOT
parent.t13.WORLDS, parent.t13.NAMESPACE = WORLDS, NAMESPACE


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as f:
        json.dump(obj, f, indent=2, sort_keys=True, allow_nan=False)
        f.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifest(name):
    for rel, wanted in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/rel) == wanted, rel


def freeze():
    assert not (HERE/'data').exists() and not (HERE/'memory').exists() and not (HERE/'results').exists()
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
         files={name: sha(ROOT/name) for name in FROZEN}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    for name, wanted in frozen['files'].items():
        assert sha(ROOT/name) == wanted, name


def generate():
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(parent.generate_world, WORLDS))
    save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(parent.t13.extract_world, WORLDS))
    # The source-trace additions are sealed before learning.
    save(HERE/'EXTRACT_MANIFEST.json', manifest(HERE/'data'))


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for w in WORLDS:
        parent.t13.learn_world(w)
        print('learned', w, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def stats():
    return dict(gain=0., early=None, tail=0., minimum=0., peak=0., drawdown=0.,
                activation=None, last=None)


def evaluate_life(w, regime, inherited):
    source = HERE/'results'/f'world{w}'/(regime+'.tsv.gz')
    with gzip.open(source, 'rt') as f:
        raw = list(csv.DictReader(f, delimiter='\t'))
    assert len(raw) == N
    payload = ''.join(f"{r['t']} {r['logcold']} {r['episode_candidate']} {r['episode_matchedL']}\n"
                      for r in raw).encode()
    outputs = {}
    for name, binary in (('split', HERE/'split'),
                         ('controls', ROOT/'turns/turn22/authority_replay')):
        proc = subprocess.run([str(binary)], input=payload, capture_output=True, check=True)
        target = HERE/'results/policies'/f'world{w}'/(regime+'-'+name+'.tsv.gz')
        target.parent.mkdir(parents=True, exist_ok=True)
        with gzip.open(target, 'xb') as f:
            f.write(proc.stdout)
        target.with_suffix('.log').write_bytes(proc.stderr)
        outputs[name] = list(csv.DictReader(io.StringIO(proc.stdout.decode()), delimiter='\t'))
        assert len(outputs[name]) == N
    summ = {m: stats() for m in MODES}
    sample = {'help': None, 'harm': None}
    exact = dict(inactive=True, equal_price=True, protected_new=True,
                 admission=True, normalized=True)
    for t, (r, a, c) in enumerate(zip(raw, outputs['split'], outputs['controls'])):
        assert int(r['t']) == int(a['t']) == int(c['t']) == t
        cold = float(r['logcold'])
        candidate = float(r['episode_candidate'])
        matched = int(r['episode_matchedL'])
        assert cold == float(a['cold']) == float(c['cold'])
        assert candidate == float(a['candidate']) == float(c['candidate'])
        assert matched == int(a['matched']) == int(c['matched'])
        exact['admission'] &= (int(a['admitted_after']) ==
                               int(c['admitted_after']) ==
                               int(r['episode_activated_after']))
        exact['normalized'] &= (float(r['max_norm_error']) <= 1e-8 and
                                all(math.isfinite(float(row)) for row in
                                    (a['live'], c['slow_live'], c['fast_live'])))
        split = float(a['live'])
        exact['inactive'] &= bool(int(a['active_before']) or split == cold)
        exact['equal_price'] &= bool(candidate != cold or split == cold)
        exact['protected_new'] &= bool(int(r['rank']) != 0 or split == cold)
        prices = {'split': split, **{m: float(c[m+'_live']) for m in MODES if m != 'split'}}
        for mode, live in prices.items():
            s = summ[mode]
            s['gain'] += live - cold
            if t < 4096 and t == 4095: s['early'] = s['gain']
            if t >= MOVE: s['tail'] += live - cold
            s['minimum'] = min(s['minimum'], s['gain'])
            s['peak'] = max(s['peak'], s['gain'])
            s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
            if int(a['admitted_after']): s['activation'] = t+1
            s['last'] = live
        if t >= MOVE and regime in ('switched', 'moved_mid'):
            diff = split - prices['fast']
            for key, better in (('help', lambda x: diff > x), ('harm', lambda x: diff < x)):
                old = sample[key]
                if old is None or better(old['split_minus_fast']):
                    sample[key] = dict(t=t, truth=int(r['truth']), matched=matched,
                                       cold=cold, candidate=candidate, split=split,
                                       fast=prices['fast'], split_minus_fast=diff,
                                       short_mass=float(a['short_before']),
                                       long_mass=float(a['long_before']))
    for mode, s in summ.items():
        assert s['early'] is not None
        bound = 10 if mode == 'fast' else 16
        assert s['minimum'] >= -1-1e-7 and s['drawdown'] <= bound+1e-7, (w, regime, mode, s)
    assert all(exact.values()), (w, regime, exact)
    assert all(s['activation'] == inherited['arms']['episode']['activation'] for s in summ.values())
    return dict(world=w, regime=regime, policies=summ, exact=exact, sample=sample,
                source_sha256=sha(source),
                split_sha256=sha(HERE/'results/policies'/f'world{w}'/(regime+'-split.tsv.gz')),
                controls_sha256=sha(HERE/'results/policies'/f'world{w}'/(regime+'-controls.tsv.gz')),
                archive_bytes=(HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size)


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    vals = lambda r,m,f: [by[w,r]['policies'][m][f] for w in WORLDS]
    diff = lambda r,a,b,f: [x-y for x,y in zip(vals(r,a,f),vals(r,b,f))]
    mean = lambda xs: math.fsum(xs)/len(xs)
    law_tail = diff('switched','split','fast','tail')
    law_full = diff('switched','split','fast','gain')
    moved_tail = diff('moved_mid','split','fast','tail')
    moved_slow = diff('moved_mid','split','slow','tail')
    intact_early = mean(vals('recombined','split','early'))
    intact_full = mean(vals('recombined','split','gain'))
    slow_early = mean(vals('recombined','slow','early'))
    slow_full = mean(vals('recombined','slow','gain'))
    metrics = dict(law_tail_vs_fast=law_tail, law_full_vs_fast=law_full,
                   moved_tail_vs_fast=moved_tail, moved_tail_vs_slow=moved_slow,
                   intact_early_retention=intact_early/slow_early if slow_early>0 else None,
                   intact_full_retention=intact_full/slow_full if slow_full>0 else None)
    tests = dict(
        law_tail_mean=mean(law_tail)>=-1,
        law_tail_worlds=sum(x>=-1 for x in law_tail)>=5,
        law_whole=mean(law_full)>=0,
        moved_tail_mean=mean(moved_tail)>=1,
        moved_tail_worlds=sum(x>0 for x in moved_tail)>=5,
        moved_vs_slow=mean(moved_slow)>=-3,
        intact_early=slow_early>0 and intact_early/slow_early>=.95,
        intact_full=slow_full>0 and intact_full/slow_full>=.95,
        intact_worlds=all(x>0 for x in vals('recombined','split','gain')),
        unrelated_no_admission=all(by[w,'unrelated']['policies']['split']['activation'] is None for w in WORLDS),
        archive=all(x['archive_bytes']<=528 for x in lives),
        exact=all(all(x['exact'].values()) for x in lives),
        bounds=all(s['minimum']>=-1-1e-7 and s['drawdown']<=(10 if m=='fast' else 16)+1e-7
                   for x in lives for m,s in x['policies'].items()))
    table = {r:{m:{field: mean(vals(r,m,field)) for field in ('early','tail','gain')}
                for m in MODES} for r in REGIMES}
    return metrics, tests, table


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json')
    check_manifest('MEMORY_MANIFEST.json')
    assert not (HERE/'results').exists()
    pairs = [(w,r) for w in WORLDS for r in REGIMES]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        inherited = list(pool.map(lambda wr: parent.t13.run_target(*wr), pairs))
    lives = []
    for item in inherited:
        life = evaluate_life(item['world'], item['regime'], item)
        lives.append(life)
        print(item['world'], item['regime'], life['policies']['split']['gain'], flush=True)
    metrics, tests, table = summarize(lives)
    save(HERE/'RESULT.json',dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
         protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, metrics=metrics,
         tests=tests, table=table, gate_pass=all(tests.values()),
         independent_reader_pending=True))
    save(HERE/'RESULTS_MANIFEST.json',manifest(HERE/'results'))
    print(json.dumps(dict(metrics=metrics, tests=tests, gate_pass=all(tests.values())), indent=2))


def main():
    p=argparse.ArgumentParser()
    p.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    stage=p.parse_args().stage
    if stage=='freeze': freeze(); return
    check_freeze()
    if stage=='generate': generate()
    elif stage=='extract': extract()
    elif stage=='learn': learn()
    else: evaluate()


if __name__=='__main__': main()
