#!/usr/bin/env python3
"""One frozen trial of distinct source cases as earned pooled residuals."""
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
WORLDS = tuple(range(296, 304))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('earned', 'pooled', 'permuted', 'flat3', 'cold')
NAMESPACE = 'netta-earned-case-residual-v1'
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024, EARLY, MOVE, N)
spec = importlib.util.spec_from_file_location('turn33_source28', ROOT/'turns/turn28/experiment.py')
p28 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p28
spec.loader.exec_module(p28)
t13, p22 = p28.t13, p28.parent
p28.HERE, p28.ROOT, p28.WORLDS, p28.REGIMES, p28.NAMESPACE = HERE, ROOT, WORLDS, REGIMES, NAMESPACE
p22.HERE, p22.REPO, p22.WORLDS, p22.REGIMES, p22.NAMESPACE = HERE, ROOT, WORLDS, REGIMES, NAMESPACE
t13.HERE, t13.REPO, t13.WORLDS, t13.REGIMES, t13.NAMESPACE = HERE, ROOT, WORLDS, ('recombined', 'unrelated', 'switched'), NAMESPACE


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def manifest(folder):
    return {str(path.relative_to(HERE)): sha(path)
            for path in sorted(folder.rglob('*')) if path.is_file()}


def check_manifest(name):
    for rel, wanted in json.loads((HERE/name).read_text()).items():
        assert sha(HERE/rel) == wanted, str(HERE/rel)


def frozen_names():
    own = ('PROTOCOL.md', 'INTERFACE.md', 'experiment.py', 'verify.py',
           'fixture_check.py', 'earned_router.c', 'earned_router', 'episode', 'Makefile')
    inherited = ('turns/turn31/case_router.c', 'turns/turn31/verify.py',
        'turns/turn30/verify.py', 'turns/turn30/router3.c',
        'turns/turn29/calibrate.c', 'turns/turn28/experiment.py', 'turns/turn28/verify.py',
        'turns/turn13/experiment.py', 'turns/turn13/episode.c', 'turns/turn22/experiment.py',
        'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
        'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
        'court4/transfer4_confirm_core.c')
    return ['turns/turn33/'+name for name in own] + list(inherited)


def freeze():
    for name in ('data', 'memory', 'results'):
        assert not (HERE/name).exists(), name
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS,
        regimes=REGIMES, arms=ARMS, files={name: sha(ROOT/name) for name in frozen_names()}))


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
    rules = t13.grow(tapes)
    children = [(rule['left'], rule['right']) for rule in rules]
    capacity = (528 - 16 - 4*len(rules)) // 16
    selected, steps = t13.joint_select(t13.branch_candidates(children, tapes), tapes, capacity)
    records = []
    for record in selected:
        counts = p28.split_counts(record['prefix'], tapes)
        assert [a+b for a, b in zip(*counts)] == record['counts']
        assert max(sum(counts, []), default=0) <= 65535
        records.append(dict(record, book_counts=counts))

    def encode(rotate):
        data = struct.pack('<8sII', b'NETEB001', len(rules), len(records))
        data += b''.join(struct.pack('<HH', *rule) for rule in children)
        for record in records:
            books = record['book_counts']
            if rotate:
                books = [[counts[0]]+counts[2:]+counts[1:2] for counts in books]
            data += struct.pack('<BBH14H', record['rule_id'], record['prefix_len'], 0,
                                *sum(books, []))
        assert len(data) <= 1056
        return data

    archives = {'bank_full.bin': encode(False), 'permuted_full.bin': encode(True),
                'pooled_full.bin': t13.branch_archive(children, selected)}
    meta = dict(rules=rules, selected=records, source_split=[list(p28.SOURCES[:2]),
        list(p28.SOURCES[2:])], joint_steps=steps,
        source_role_sha256=[hashlib.sha256(tape).hexdigest() for tape in tapes],
        bytes={name: len(data) for name, data in archives.items()},
        record_count=len(records), added_case_bytes=len(archives['bank_full.bin'])-
                                                   len(archives['pooled_full.bin']))
    return archives, meta


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for world in WORLDS:
        tapes, _ = t13.source_tapes(world)
        archives, meta = build_memory(tapes)
        folder = HERE/'memory'/f'world{world}'
        folder.mkdir(parents=True, exist_ok=False)
        for name, data in archives.items():
            with (folder/name).open('xb') as stream:
                stream.write(data)
        save(folder/'BOOKS.json', meta)
        print('learned', world, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def run_target(pair):
    world, regime = pair
    folder = HERE/'results'/f'world{world}'
    folder.mkdir(parents=True, exist_ok=True)
    memory = HERE/'memory'/f'world{world}'
    command = [str(HERE/'earned_router'), 'predict'] + [str(memory/name) for name in
        ('bank_full.bin', 'pooled_full.bin', 'permuted_full.bin')]
    target = folder/(regime+'.turn33.tsv.gz')
    with (HERE/'data'/f'world{world}'/(regime+'.bin')).open('rb') as source, \
         gzip.open(target, 'xb') as output:
        proc = subprocess.Popen(command, stdin=source, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        while chunk := proc.stdout.read(1 << 20):
            output.write(chunk)
        stderr = proc.stderr.read()
        rc = proc.wait()
    with target.with_suffix('.stderr').open('xb') as stream:
        stream.write(stderr)
    assert rc == 0, (command, rc, stderr.decode())
    return summarize(world, regime)


def blank():
    return dict(gain=0.0, early=0.0, tail=0.0, minimum=0.0, peak=0.0,
                drawdown=0.0, activation=None, horizons={})


def summarize(world, regime):
    stats = {arm: blank() for arm in ARMS}
    exact = dict(new=True, equal=True, inactive=True, history=True,
                 shared_admission=True, router_state=True, silent=True, first=True)
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    assert len(raw) == N
    if regime in ('partial', 'switched', 'moved_mid'):
        assert raw[:MOVE] == (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()[:MOVE]
    history, max_norm = [], 0.0
    best = worst = first_divergence = None
    residual = dict(matched=0, first=0, silent=0, active=0,
                    became_active=0, fell_silent=0)
    path = HERE/'results'/f'world{world}'/(regime+'.turn33.tsv.gz')
    with gzip.open(path, 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        for t in range(N):
            row = next(rows, None)
            assert row is not None and int(row['t']) == t and int(row['truth']) == raw[t]
            exact['history'] &= row['history'] == (''.join(map(str, history)) or '-')
            rank, cold = int(row['rank']), float(row['logcold'])
            record, visits = int(row['record']), int(row['record_visits_before'])
            max_norm = max(max_norm, float(row['max_norm_error']))
            for side in ('before', 'after'):
                u = float(row[f'permission_u_{side}'])
                exact['router_state'] &= 0 < u < 1
                for prefix in ('earned', 'permuted'):
                    for case in ('ha', 'hb'):
                        value = float(row[f'{prefix}_{case}_{side}'])
                        exact['router_state'] &= math.isfinite(value) and value >= -10-1e-10
                weights = list(map(float, row[f'flat3_w_{side}'].split(',')))
                exact['router_state'] &= (len(weights) == 3 and all(0 < x < 1 for x in weights)
                                          and abs(math.fsum(weights)-1) <= 1e-10)
            earned_before = (float(row['earned_ha_before']), float(row['earned_hb_before']))
            if record >= 0:
                residual['matched'] += 1
                now = max(earned_before) > 0
                after_now = max(float(row['earned_ha_after']),
                                float(row['earned_hb_after'])) > 0
                if visits == 0:
                    residual['first'] += 1
                    exact['first'] &= float(row['earned_candidate']) == float(row['pooled_candidate'])
                if not now:
                    residual['silent'] += 1
                    exact['silent'] &= float(row['earned_candidate']) == float(row['pooled_candidate'])
                else:
                    residual['active'] += 1
                if not now and after_now:
                    residual['became_active'] += 1
                if now and not after_now:
                    residual['fell_silent'] += 1
            active = {int(row[arm+'_active_before']) for arm in ARMS}
            fired = {int(row[arm+'_activated_after']) for arm in ARMS}
            exact['shared_admission'] &= (len(active) == 1 and len(fired) == 1 and
                                           active == {int(row['admitted_before'])})
            for arm in ARMS:
                candidate, live = float(row[arm+'_candidate']), float(row[arm+'_live'])
                state = stats[arm]
                state['gain'] += live-cold
                state['minimum'] = min(state['minimum'], state['gain'])
                state['peak'] = max(state['peak'], state['gain'])
                state['drawdown'] = max(state['drawdown'], state['peak']-state['gain'])
                assert abs(state['gain']-float(row[arm+'_gain_after'])) <= 1e-7
                if int(row[arm+'_activated_after']):
                    assert state['activation'] is None
                    state['activation'] = t+1
                if t+1 in HORIZONS:
                    state['horizons'][str(t+1)] = state['gain']
                if not rank:
                    exact['new'] &= candidate == cold and live == cold
                if candidate == cold:
                    exact['equal'] &= live == cold
                if not int(row[arm+'_active_before']):
                    exact['inactive'] &= live == cold
            difference = float(row['earned_live'])-float(row['pooled_live'])
            sample = dict(row, difference=difference,
                          raw_hex=raw[max(0, t-16):t+17].hex())
            if t >= MOVE:
                if best is None or difference > best['difference']:
                    best = sample
                if worst is None or difference < worst['difference']:
                    worst = sample
            if first_divergence is None and float(row['earned_candidate']) != float(row['pooled_candidate']):
                first_divergence = sample
            history = (history+[rank])[-32:]
        assert next(rows, None) is None
    for state in stats.values():
        state['early'] = state['horizons'][str(EARLY)]
        state['tail'] = state['gain']-state['horizons'][str(MOVE)]
    return dict(world=world, regime=regime, arms=stats, max_norm_error=max_norm,
                exactness=exact, residual=residual, raw_help=best, raw_harm=worst,
                first_divergence=first_divergence)


def mean(values):
    return math.fsum(values)/len(values)


def verdict(lives):
    lookup = {(life['world'], life['regime']): life for life in lives}
    values = lambda regime, arm, key: [lookup[w, regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime, other, key: [a-b for a, b in
        zip(values(regime, 'earned', key), values(regime, other, key))]
    tables = {regime: {arm: {key: mean(values(regime, arm, key))
              for key in ('early', 'gain', 'tail')} for arm in ARMS} for regime in REGIMES}
    comparisons = {}
    for regime in REGIMES:
        comparisons[regime] = {}
        for other in ('pooled', 'permuted', 'flat3'):
            comparisons[regime][other] = {}
            for key in ('early', 'gain', 'tail'):
                delta = diffs(regime, other, key)
                comparisons[regime][other][key] = dict(mean=mean(delta),
                    wins=sum(value > 0 for value in delta), per_world=delta)
    retention = dict(recombined_early=tables['recombined']['earned']['early']/
                      max(tables['recombined']['pooled']['early'], 1e-300),
                     recombined_full=tables['recombined']['earned']['gain']/
                      max(tables['recombined']['pooled']['gain'], 1e-300))
    books = [json.loads((HERE/'memory'/f'world{world}'/'BOOKS.json').read_text())
             for world in WORLDS]
    extras = sorted({book['added_case_bytes'] for book in books})
    assert extras == [384], extras
    margin = 0.01*extras[0]
    portable = {arm: sorted({0 if arm == 'cold' else book['bytes'][
        'pooled_full.bin' if arm == 'pooled' else 'bank_full.bin'] for book in books})
        for arm in ARMS}
    recipient = {arm: sorted({book['record_count']*{
        'earned': 24, 'pooled': 8, 'permuted': 24, 'flat3': 24, 'cold': 0}[arm]
        for book in books}) for arm in ARMS}
    prefix_identity = all(lookup[w, regime]['arms'][arm]['horizons'][str(MOVE)] ==
        lookup[w, 'recombined']['arms'][arm]['horizons'][str(MOVE)]
        for w in WORLDS for regime in ('partial', 'switched', 'moved_mid') for arm in ARMS)
    admission_identity = all(len({state['activation'] for state in life['arms'].values()}) == 1
                             for life in lives)
    partial = comparisons['partial']['pooled']
    correspondence = dict(recombined=comparisons['recombined']['permuted']['gain'],
                          partial_tail=comparisons['partial']['permuted']['tail'])
    quantities = dict(comparisons=comparisons, retention=retention,
        added_case_bytes=extras, priced_margin_bits=margin, correspondence=correspondence,
        portable_bytes=portable, recipient_router_bytes=recipient,
        residual_activity={regime: {key: sum(lookup[w, regime]['residual'][key] for w in WORLDS)
            for key in ('matched', 'first', 'silent', 'active',
                        'became_active', 'fell_silent')}
            for regime in REGIMES}, shared_prefix_identity=prefix_identity,
        recombined_positive=sum(value > 0 for value in values('recombined', 'earned', 'gain')),
        unrelated_admissions=[dict(world=w, arm=arm,
            activation=lookup[w, 'unrelated']['arms'][arm]['activation'],
            gain=lookup[w, 'unrelated']['arms'][arm]['gain'])
            for w in WORLDS for arm in ARMS
            if lookup[w, 'unrelated']['arms'][arm]['activation'] is not None])
    validity = dict(archive=all(book['bytes']['pooled_full.bin'] <= 528 and
        book['bytes']['bank_full.bin'] <= 1056 and
        book['bytes']['permuted_full.bin'] == book['bytes']['bank_full.bin'] for book in books),
        source_counts=True, normalization=all(life['max_norm_error'] <= 1e-8 for life in lives),
        exact_quotes=all(all(life['exactness'].values()) for life in lives),
        bounds=all(state['minimum'] >= -1-1e-7 and state['drawdown'] <= 16+1e-7
                   for life in lives for state in life['arms'].values()),
        identical_admissions=admission_identity, shared_prefix=prefix_identity,
        unrelated_disclosed=True)
    conditions = dict(
        E1=partial['tail']['mean'] > margin and partial['tail']['wins'] >= 5 and
           partial['gain']['mean'] >= 0,
        E2=all(tables['recombined']['pooled'][key] > 0 for key in ('early', 'gain')) and
           min(retention.values()) >= .99 and quantities['recombined_positive'] == 8,
        E3=all(correspondence[key]['mean'] > margin and correspondence[key]['wins'] >= 5
               for key in ('recombined', 'partial_tail')),
        E4=False)
    material = all(conditions[key] for key in ('E1', 'E2', 'E3')) and all(validity.values())
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        quantities=quantities, conditions=conditions, validity=validity,
        material_pass=material, gate_pass=False, independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json')
    check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w, regime) for w in WORLDS for regime in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives)
    save(HERE/'RESULT.json', result)
    print(json.dumps({key: result[key] for key in
        ('conditions', 'validity', 'material_pass', 'gate_pass', 'tables')},
        indent=2, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'learn', 'evaluate'))
    args = parser.parse_args()
    if args.stage != 'freeze':
        check_freeze()
    globals()[args.stage]()
