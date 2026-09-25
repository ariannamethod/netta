#!/usr/bin/env python3
"""One source-memory intervention; immutable stages for a single fresh batch."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(264, 272))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('local', 'global', 'pooled_full', 'pooled_small', 'permuted')
NAMESPACE = 'netta-episode-alternatives-v1'
N = 16384
MOVE = 8192
EARLY = 4096
HORIZONS = (1024, EARLY, MOVE, N)
SOURCES = ('sourceAB', 'sourceBA', 'sourceCD', 'sourceDC')
spec = importlib.util.spec_from_file_location('turn28_parent', ROOT/'turns/turn22/experiment.py')
parent = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = parent
spec.loader.exec_module(parent)
t13 = parent.t13
parent.HERE, parent.REPO = HERE, ROOT
t13.HERE, t13.REPO, t13.NAMESPACE, t13.WORLDS = HERE, ROOT, NAMESPACE, WORLDS


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as f:
        json.dump(value, f, indent=2, sort_keys=True, allow_nan=False)
        f.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifest(name):
    for rel, digest in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/rel) == digest, str(HERE/rel)


def freeze():
    for folder in ('data', 'memory', 'results'):
        assert not (HERE/folder).exists(), folder
    names = ['turns/turn28/'+s for s in ('PROTOCOL.md', 'INTERFACE.md', 'experiment.py',
             'verify.py', 'bank.c', 'bank', 'episode', 'Makefile', 'source_check.py')]
    names += ['turns/turn13/experiment.py', 'turns/turn13/episode.c',
              'turns/turn22/experiment.py', 'byte_recurrence/frontend.c',
              'byte_recurrence/frontend.h', 'portable_recurrence/recurrence.c',
              'portable_recurrence/recurrence.h', 'court4/transfer4_confirm_core.c']
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
         arms=ARMS, files={name: sha(ROOT/name) for name in names}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    for name, digest in frozen['files'].items():
        assert sha(ROOT/name) == digest, name


def generate_world(w):
    parent.generate_world(w)
    d = HERE/'data'/f'world{w}'
    commands = bytearray((d/'recombined.commands.bin').read_bytes())
    original = bytes(commands)
    cycle = (0, 3, 1, 2)
    rng = random.Random(t13.seed(w, 'commands-partial'))
    replaced = []
    for t in range(MOVE, N):
        if cycle[(t//48) % 4] in (0, 1):
            commands[t] = rng.randrange(4)
            replaced.append(t)
    cp = d/'partial.commands.bin'
    cp.write_bytes(commands)
    meta = json.loads((d/'GENERATOR.json').read_text())
    t13.emit(d/'partial.bin', cp, d/'recombined.alphabet.bin',
             meta['emission_seeds']['recombined'])
    base = (d/'recombined.bin').read_bytes()
    assert (d/'partial.bin').read_bytes()[:MOVE] == base[:MOVE]
    replaced_set = set(replaced)
    assert all(commands[t] == original[t] for t in range(N) if t not in replaced_set)
    save(d/'PARTIAL.json', dict(replaced_component_ids=[0, 1], replaced_positions=replaced,
        unchanged_command_positions=N-len(replaced), prefix_identical=True,
        seed=t13.seed(w, 'commands-partial'), scope='Generator only; not learner input.'))
    return w


def generate():
    # Inherited emitter/trace CLI uses this name. Makefile supplies the binary.
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(generate_world, WORLDS))
    save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(t13.extract_world, WORLDS))
    save(HERE/'EXTRACT_MANIFEST.json', manifest(HERE/'data'))


def split_counts(prefix, tapes):
    """Overlapping occurrences with a successor, inside each life only."""
    context = bytes(prefix)
    assert context and len(tapes) == 4
    books = [[0]*7 for _ in range(2)]
    for i, tape in enumerate(tapes):
        start = 0
        while (at := tape.find(context, start)) >= 0:
            start = at+1
            after = at+len(context)
            if after < len(tape):
                books[i//2][tape[after]] += 1
    return books


def build_memory(tapes):
    rules = t13.grow(tapes)
    children = [(r['left'], r['right']) for r in rules]
    capacity = (528-16-4*len(rules))//16
    bank_capacity = (528-16-4*len(rules))//32
    pool, steps = t13.joint_select(t13.branch_candidates(children, tapes), tapes, capacity)
    small = pool[:bank_capacity]
    records = []
    for record in small:
        counts = split_counts(record['prefix'], tapes)
        assert [a+b for a,b in zip(*counts)] == record['counts']
        assert max(sum(counts, []), default=0) <= 65535
        records.append(dict(record, book_counts=counts))

    def encode(rotate):
        data = struct.pack('<8sII', b'NETEB001', len(rules), len(records))
        data += b''.join(struct.pack('<HH', *r) for r in children)
        for r in records:
            books = r['book_counts']
            if rotate:
                books = [[c[0]]+c[2:]+c[1:2] for c in books]
            data += struct.pack('<BBH14H', r['rule_id'], r['prefix_len'], 0, *sum(books, []))
        assert len(data) <= 528
        return data

    archives = {'bank.bin': encode(False), 'permuted.bin': encode(True),
                'pooled_full.bin': t13.branch_archive(children, pool),
                'pooled_small.bin': t13.branch_archive(children, small)}
    metadata = dict(rules=rules, selected=pool, bank_selected=records,
        source_split=[list(SOURCES[:2]), list(SOURCES[2:])], joint_steps=steps,
        source_role_sha256=[hashlib.sha256(t).hexdigest() for t in tapes],
        bytes={name: len(data) for name,data in archives.items()},
        record_counts=dict(pooled_full=len(pool), local=len(records)))
    return archives, metadata


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for w in WORLDS:
        tapes, _ = t13.source_tapes(w)
        archives, metadata = build_memory(tapes)
        folder = HERE/'memory'/f'world{w}'
        folder.mkdir(parents=True, exist_ok=False)
        for name, data in archives.items():
            (folder/name).write_bytes(data)
        save(folder/'BOOKS.json', metadata)
        print('learned', w, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def run_target(pair):
    w, regime = pair
    folder = HERE/'results'/f'world{w}'
    folder.mkdir(parents=True, exist_ok=True)
    bank = HERE/'memory'/f'world{w}'
    cmd = [str(HERE/'bank'), 'predict']+[str(bank/name) for name in
        ('bank.bin', 'pooled_full.bin', 'pooled_small.bin', 'permuted.bin')]
    target = folder/(regime+'.tsv.gz')
    with (HERE/'data'/f'world{w}'/(regime+'.bin')).open('rb') as source, gzip.open(target, 'xb') as out:
        proc = subprocess.Popen(cmd, stdin=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while chunk := proc.stdout.read(1 << 20):
            out.write(chunk)
        stderr = proc.stderr.read()
        rc = proc.wait()
    target.with_suffix('.stderr').write_bytes(stderr)
    assert rc == 0, (cmd, rc, stderr.decode())
    return summarize(w, regime)


def empty_stats():
    return dict(gain=0.0, early=0.0, tail=0.0, minimum=0.0, peak=0.0, drawdown=0.0,
                activation=None, horizons={})


def summarize(w, regime):
    arms = {a: empty_stats() for a in ARMS}
    exact = dict(new=True, equal=True, inactive=True, history=True)
    max_norm = 0.0
    best = worst = None
    past = ''
    raw = (HERE/'data'/f'world{w}'/(regime+'.bin')).read_bytes()
    with gzip.open(HERE/'results'/f'world{w}'/(regime+'.tsv.gz'), 'rt') as f:
        for t, row in enumerate(csv.DictReader(f, delimiter='\t')):
            assert int(row['t']) == t and int(row['truth']) == raw[t]
            cold = float(row['logcold'])
            rank = int(row['rank'])
            exact['history'] &= row['history'] == (past or '-')
            past = (past+str(rank))[-32:]
            max_norm = max(max_norm, float(row['max_norm_error']))
            for arm, s in arms.items():
                live, cand = float(row[arm+'_live']), float(row[arm+'_candidate'])
                s['gain'] += live-cold
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
                assert abs(s['gain']-float(row[arm+'_gain_after'])) <= 1e-7
                if int(row[arm+'_activated_after']):
                    assert s['activation'] is None
                    s['activation'] = t+1
                if t+1 in HORIZONS:
                    s['horizons'][str(t+1)] = s['gain']
                if not rank:
                    exact['new'] &= cand == cold and live == cold
                if cand == cold:
                    exact['equal'] &= live == cold
                if not int(row[arm+'_active_before']):
                    exact['inactive'] &= live == cold
            exact['new'] &= int(row['new_exact']) == 1
            if t >= MOVE:
                delta = float(row['local_live'])-float(row['pooled_full_live'])
                sample = dict(row, difference=delta, raw_hex=raw[max(0,t-16):t+17].hex())
                if best is None or delta > best['difference']: best = sample
                if worst is None or delta < worst['difference']: worst = sample
        assert t+1 == N
    for s in arms.values():
        s['early'] = s['horizons'][str(EARLY)]
        s['tail'] = s['gain']-s['horizons'][str(MOVE)]
    return dict(world=w, regime=regime, arms=arms, max_norm_error=max_norm,
                exactness=exact, raw_help=best, raw_harm=worst)


def verdict(lives):
    lookup = {(r['world'],r['regime']):r for r in lives}
    mean = lambda seq: math.fsum(seq)/len(seq)
    vals = lambda regime,arm,key: [lookup[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime,arm,key: [a-b for a,b in zip(vals(regime,'local',key),vals(regime,arm,key))]
    early = vals('recombined','local','early')
    full = vals('recombined','local','gain')
    early_full = diffs('recombined','pooled_full','early')
    partial = diffs('partial','pooled_full','tail')
    global_diff = diffs('recombined','global','early')
    perm_diff = diffs('recombined','permuted','early')
    pooled_mean = mean(vals('recombined','pooled_full','gain'))
    q = dict(early_gain_per_byte=mean(early)/EARLY, early_positive=sum(x>0 for x in early),
        early_vs_full=mean(early_full), early_wins_full=sum(x>0 for x in early_full),
        full_retention=mean(full)/max(pooled_mean,1e-300), full_positive=sum(x>0 for x in full),
        partial_tail_vs_full=mean(partial), partial_tail_wins_full=sum(x>0 for x in partial),
        partial_whole_vs_full=mean(diffs('partial','pooled_full','gain')),
        early_vs_global=mean(global_diff),early_wins_global=sum(x>0 for x in global_diff),
        early_vs_permuted=mean(perm_diff),early_wins_permuted=sum(x>0 for x in perm_diff))
    conditions = dict(T1=q['early_gain_per_byte']>=.005 and q['early_positive']>=6,
        T2=q['early_vs_full']>1 and q['early_wins_full']>=5,
        T3=pooled_mean>0 and q['full_retention']>=.95 and q['full_positive']==8,
        T4=q['partial_tail_vs_full']>1 and q['partial_tail_wins_full']>=5 and q['partial_whole_vs_full']>=0,
        T5=q['early_vs_global']>1 and q['early_wins_global']>=5 and
           q['early_vs_permuted']>1 and q['early_wins_permuted']>=5)
    tables = {regime:{arm:{key:mean(vals(regime,arm,key)) for key in ('early','gain','tail')}
              for arm in ARMS} for regime in REGIMES}
    validity = dict(archive=all(p.stat().st_size<=528 for p in (HERE/'memory').rglob('*.bin')),
        source_counts=True, normalization=all(r['max_norm_error']<=1e-8 for r in lives),
        exact_quotes=all(all(r['exactness'].values()) for r in lives),
        bounds=all(s['minimum']>=-1-1e-7 and s['drawdown']<=16+1e-7
                   for r in lives for s in r['arms'].values()))
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        conditions=conditions, quantities=q, validity=validity,
        material_pass=all(conditions.values()), independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json')
    check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives)
    save(HERE/'RESULT.json', result)
    print(json.dumps({k:result[k] for k in ('conditions','quantities','validity','material_pass')},indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze','generate','extract','learn','evaluate'))
    args = parser.parse_args()
    if args.stage != 'freeze': check_freeze()
    globals()[args.stage]()
