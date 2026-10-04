#!/usr/bin/env python3
"""One frozen trial of source-selected conditional episode granularity."""
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
WORLDS = tuple(range(312, 320))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('dense10', 'sparse10', 'sel4', 'earned2', 'pooled', 'cold')
NAMESPACE = 'netta-conditional-granularity-v1'
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024, EARLY, MOVE, N)
FINE = 10

spec = importlib.util.spec_from_file_location('turn35_source28', ROOT/'turns/turn28/experiment.py')
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
    own = ('PROTOCOL.md', 'DIAGNOSIS34.md', 'INTERFACE.md', 'experiment.py',
           'verify.py', 'fixture_check.py', 'hybrid_router.c', 'hybrid_router',
           'episode', 'Makefile')
    inherited = (
        'turns/turn34/sel4_router.c', 'turns/turn34/verify.py',
        'turns/turn34/PROTOCOL.md', 'turns/turn33/earned_router.c',
        'turns/turn33/verify.py', 'turns/turn33/PROTOCOL.md',
        'turns/turn31/case_router.c', 'turns/turn30/verify.py',
        'turns/turn30/router3.c', 'turns/turn28/experiment.py',
        'turns/turn28/verify.py', 'turns/turn13/experiment.py',
        'turns/turn13/episode.c', 'turns/turn22/experiment.py',
        'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
        'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
        'court4/transfer4_confirm_core.c')
    return ['turns/turn35/'+name for name in own] + list(inherited)


def freeze():
    for name in ('data', 'memory', 'results'):
        assert not (HERE/name).exists(), name
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS,
        regimes=REGIMES, arms=ARMS, fine_records=FINE,
        files={name: sha(ROOT/name) for name in frozen_names()}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES) and frozen['arms'] == list(ARMS)
    assert frozen['fine_records'] == FINE
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


def split4_counts(prefix, tapes):
    context = bytes(prefix)
    assert context and len(tapes) == 4
    books = [[0]*7 for _ in range(4)]
    for i, tape in enumerate(tapes):
        start = 0
        while (at := tape.find(context, start)) >= 0:
            start = at + 1
            after = at + len(context)
            if after < len(tape):
                books[i][tape[after]] += 1
    return books


def information_density(episodes):
    """Frozen Jeffreys-smoothed episode-vs-parent source score."""
    parents = [[a+b for a, b in zip(episodes[0], episodes[1])],
               [a+b for a, b in zip(episodes[2], episodes[3])]]
    gain = 0.0
    for episode, parent in zip(episodes, (parents[0], parents[0], parents[1], parents[1])):
        ne, np = sum(episode), sum(parent)
        for n, pn in zip(episode, parent):
            if n:
                pe = (n + .5)/(ne + 3.5)
                pp = (pn + .5)/(np + 3.5)
                gain += n*math.log2(pe/pp)
    return gain/(1 + sum(map(sum, episodes)))


def build_memory(tapes):
    rules = t13.grow(tapes)
    children = [(rule['left'], rule['right']) for rule in rules]
    capacity = (528 - 16 - 4*len(rules)) // 16
    selected, steps = t13.joint_select(t13.branch_candidates(children, tapes), tapes, capacity)
    records = []
    for record in selected:
        episodes = split4_counts(record['prefix'], tapes)
        cases = p28.split_counts(record['prefix'], tapes)
        assert [a+b for a, b in zip(episodes[0], episodes[1])] == cases[0]
        assert [a+b for a, b in zip(episodes[2], episodes[3])] == cases[1]
        assert [a+b for a, b in zip(*cases)] == record['counts']
        assert max(sum(episodes, []), default=0) <= 65535
        records.append(dict(record, book_counts=cases, episode_counts=episodes,
                            information_density=information_density(episodes)))
    assert len(records) == 24
    scores = [record['information_density'] for record in records]
    dense = sorted(range(len(records)), key=lambda i: (-scores[i], i))[:FINE]
    sparse = sorted(range(len(records)), key=lambda i: (scores[i], i))[:FINE]

    def encode2():
        data = struct.pack('<8sII', b'NETEB001', len(rules), len(records))
        data += b''.join(struct.pack('<HH', *rule) for rule in children)
        for record in records:
            data += struct.pack('<BBH14H', record['rule_id'], record['prefix_len'], 0,
                                *sum(record['book_counts'], []))
        return data

    def encode4():
        data = struct.pack('<8sII', b'NETEB004', len(rules), len(records))
        data += b''.join(struct.pack('<HH', *rule) for rule in children)
        for record in records:
            data += struct.pack('<BBH28H', record['rule_id'], record['prefix_len'], 0,
                                *sum(record['episode_counts'], []))
        return data

    def encode_hybrid(indices):
        chosen = set(indices)
        data = struct.pack('<8sII', b'NETEH010', len(rules), len(records))
        data += b''.join(struct.pack('<HH', *rule) for rule in children)
        for i, record in enumerate(records):
            data += struct.pack('<BBH14H', record['rule_id'], record['prefix_len'],
                                int(i in chosen), *sum(record['book_counts'], []))
        for i, record in enumerate(records):
            if i in chosen:
                data += struct.pack('<14H', *(record['episode_counts'][0] +
                                              record['episode_counts'][2]))
        return data

    archives = {
        'dense10.bin': encode_hybrid(dense),
        'sparse10.bin': encode_hybrid(sparse),
        'bank4_full.bin': encode4(),
        'bank2_full.bin': encode2(),
        'pooled_full.bin': t13.branch_archive(children, selected)}
    assert len(archives['dense10.bin']) == len(archives['sparse10.bin'])
    assert len(archives['dense10.bin']) - len(archives['bank2_full.bin']) == 28*FINE
    meta = dict(
        rules=rules, selected=records, source_split=[[name] for name in p28.SOURCES],
        case_split=[list(p28.SOURCES[:2]), list(p28.SOURCES[2:])],
        joint_steps=steps,
        source_role_sha256=[hashlib.sha256(tape).hexdigest() for tape in tapes],
        score='Jeffreys episode-vs-paired-parent self-description gain / (1+successors)',
        dense_indices=dense, sparse_indices=sparse, bytes={k: len(v) for k, v in archives.items()},
        record_count=len(records), fine_records=FINE,
        added_hybrid_bytes=len(archives['dense10.bin'])-len(archives['bank2_full.bin']),
        added_full_bytes=len(archives['bank4_full.bin'])-len(archives['bank2_full.bin']))
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
    command = [str(HERE/'hybrid_router'), 'predict'] + [str(memory/name) for name in
        ('dense10.bin', 'sparse10.bin', 'bank4_full.bin', 'bank2_full.bin', 'pooled_full.bin')]
    target = folder/(regime+'.turn35.tsv.gz')
    with (HERE/'data'/f'world{world}'/(regime+'.bin')).open('rb') as source, \
         gzip.open(target, 'xb') as output:
        proc = subprocess.Popen(command, stdin=source, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        while chunk := proc.stdout.read(1 << 20):
            output.write(chunk)
        stderr = proc.stderr.read(); rc = proc.wait()
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
                 shared_admission=True, router_state=True, first=True,
                 dense10_silent=True, sparse10_silent=True,
                 sel4_silent=True, earned2_silent=True, source_tags=True)
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    assert len(raw) == N
    if regime in ('partial', 'switched', 'moved_mid'):
        assert raw[:MOVE] == (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()[:MOVE]
    meta = json.loads((HERE/'memory'/f'world{world}'/'BOOKS.json').read_text())
    dense_set, sparse_set = set(meta['dense_indices']), set(meta['sparse_indices'])
    history, max_norm = [], 0.0
    residual = {arm: dict(matched=0, first=0, silent=0, active=0,
                          became_active=0, fell_silent=0)
                for arm in ('dense10', 'sparse10', 'sel4', 'earned2')}
    fine_visits = dict(dense10=0, sparse10=0)
    best = worst = selector_help = selector_harm = None
    path = HERE/'results'/f'world{world}'/(regime+'.turn35.tsv.gz')
    with gzip.open(path, 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        for t in range(N):
            row = next(rows, None)
            assert row is not None and int(row['t']) == t and int(row['truth']) == raw[t]
            exact['history'] &= row['history'] == (''.join(map(str, history)) or '-')
            rank, cold = int(row['rank']), float(row['logcold'])
            record, visits = int(row['record']), int(row['record_visits_before'])
            df = int(row['dense_fine']); sf = int(row['sparse_fine'])
            if record >= 0:
                exact['source_tags'] &= df == int(record in dense_set) and sf == int(record in sparse_set)
            else:
                exact['source_tags'] &= df == sf == 0
            max_norm = max(max_norm, float(row['max_norm_error']))
            for side in ('before', 'after'):
                u = float(row[f'permission_u_{side}'])
                exact['router_state'] &= 0 < u < 1
                for prefix in ('dense10', 'sparse10', 'sel4'):
                    values = [float(row[f'{prefix}_h{i}_{side}']) for i in range(1, 5)]
                    exact['router_state'] &= all(math.isfinite(v) and v >= -10-1e-10 for v in values)
                    if ((prefix == 'dense10' and not df) or
                            (prefix == 'sparse10' and not sf)):
                        exact['router_state'] &= values[2:] == [0.0, 0.0]
                for case in ('ha', 'hb'):
                    value = float(row[f'earned2_{case}_{side}'])
                    exact['router_state'] &= math.isfinite(value) and value >= -10-1e-10
            if record >= 0:
                if df: fine_visits['dense10'] += 1
                if sf: fine_visits['sparse10'] += 1
                for arm, parts in (('dense10', 4 if df else 2),
                                   ('sparse10', 4 if sf else 2), ('sel4', 4), ('earned2', 2)):
                    if arm == 'earned2':
                        before = [float(row['earned2_ha_before']), float(row['earned2_hb_before'])]
                        after = [float(row['earned2_ha_after']), float(row['earned2_hb_after'])]
                    else:
                        before = [float(row[f'{arm}_h{i}_before']) for i in range(1, parts+1)]
                        after = [float(row[f'{arm}_h{i}_after']) for i in range(1, parts+1)]
                    r = residual[arm]; r['matched'] += 1
                    silent, after_silent = max(before) <= 0, max(after) <= 0
                    if visits == 0:
                        r['first'] += 1
                        exact['first'] &= float(row[arm+'_candidate']) == float(row['pooled_candidate'])
                    if silent:
                        r['silent'] += 1
                        exact[arm+'_silent'] &= float(row[arm+'_candidate']) == float(row['pooled_candidate'])
                    else: r['active'] += 1
                    r['became_active'] += silent and not after_silent
                    r['fell_silent'] += not silent and after_silent
            active = {int(row[arm+'_active_before']) for arm in ARMS}
            fired = {int(row[arm+'_activated_after']) for arm in ARMS}
            exact['shared_admission'] &= (len(active) == 1 and len(fired) == 1 and
                                           active == {int(row['admitted_before'])})
            for arm in ARMS:
                candidate, live = float(row[arm+'_candidate']), float(row[arm+'_live'])
                state = stats[arm]; state['gain'] += live-cold
                state['minimum'] = min(state['minimum'], state['gain'])
                state['peak'] = max(state['peak'], state['gain'])
                state['drawdown'] = max(state['drawdown'], state['peak']-state['gain'])
                assert abs(state['gain']-float(row[arm+'_gain_after'])) <= 1e-7
                if int(row[arm+'_activated_after']):
                    assert state['activation'] is None; state['activation'] = t+1
                if t+1 in HORIZONS: state['horizons'][str(t+1)] = state['gain']
                if not rank: exact['new'] &= candidate == cold and live == cold
                if candidate == cold: exact['equal'] &= live == cold
                if not int(row[arm+'_active_before']): exact['inactive'] &= live == cold
            d = float(row['dense10_live'])-float(row['earned2_live'])
            s = float(row['dense10_live'])-float(row['sparse10_live'])
            sample = dict(row, dense_over_coarse=d, dense_over_sparse=s,
                          raw_hex=raw[max(0,t-16):t+17].hex())
            if t >= MOVE:
                if best is None or d > best['dense_over_coarse']: best = sample
                if worst is None or d < worst['dense_over_coarse']: worst = sample
                if selector_help is None or s > selector_help['dense_over_sparse']: selector_help = sample
                if selector_harm is None or s < selector_harm['dense_over_sparse']: selector_harm = sample
            history = (history+[rank])[-32:]
        assert next(rows, None) is None
    for state in stats.values():
        state['early'] = state['horizons'][str(EARLY)]
        state['tail'] = state['gain']-state['horizons'][str(MOVE)]
    return dict(world=world, regime=regime, arms=stats, max_norm_error=max_norm,
                exactness=exact, residual=residual, fine_visits=fine_visits,
                raw_help=best, raw_harm=worst, selector_help=selector_help,
                selector_harm=selector_harm)


def mean(values):
    return math.fsum(values)/len(values)


def verdict(lives):
    lookup = {(life['world'], life['regime']): life for life in lives}
    values = lambda regime, arm, key: [lookup[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime, left, right, key: [a-b for a,b in zip(values(regime,left,key), values(regime,right,key))]
    tables = {regime: {arm: {key: mean(values(regime,arm,key)) for key in ('early','gain','tail')}
                       for arm in ARMS} for regime in REGIMES}
    comparisons = {}
    for regime in REGIMES:
        comparisons[regime] = {}
        for left, right in (('dense10','earned2'), ('dense10','sparse10'),
                            ('sel4','earned2'), ('dense10','pooled')):
            name = left+'_minus_'+right; comparisons[regime][name] = {}
            for key in ('early','gain','tail'):
                delta = diffs(regime,left,right,key)
                comparisons[regime][name][key] = dict(mean=mean(delta),
                    wins=sum(x>0 for x in delta), per_world=delta)
    books = [json.loads((HERE/'memory'/f'world{w}'/'BOOKS.json').read_text()) for w in WORLDS]
    hbytes = sorted({b['added_hybrid_bytes'] for b in books})
    fbytes = sorted({b['added_full_bytes'] for b in books})
    assert hbytes == [280] and fbytes == [672]
    m10, m24 = .01*hbytes[0], .01*fbytes[0]
    portable = {arm: sorted({0 if arm=='cold' else b['bytes'][{
        'dense10':'dense10.bin','sparse10':'sparse10.bin','sel4':'bank4_full.bin',
        'earned2':'bank2_full.bin','pooled':'pooled_full.bin'}[arm]] for b in books}) for arm in ARMS}
    recipient = {arm: sorted({b['record_count']*{
        'dense10':40,'sparse10':40,'sel4':40,'earned2':24,'pooled':8,'cold':0}[arm]
        for b in books}) for arm in ARMS}
    prefix_identity = all(lookup[w,r]['arms'][a]['horizons'][str(MOVE)] ==
                          lookup[w,'recombined']['arms'][a]['horizons'][str(MOVE)]
                          for w in WORLDS for r in ('partial','switched','moved_mid') for a in ARMS)
    admission_identity = all(len({s['activation'] for s in life['arms'].values()}) == 1 for life in lives)
    d = comparisons['switched']['dense10_minus_earned2']
    s = comparisons['switched']['dense10_minus_sparse10']
    f = comparisons['switched']['sel4_minus_earned2']
    partial_d = comparisons['partial']['dense10_minus_earned2']['tail']
    dense_positive = sum(x>0 for x in values('recombined','dense10','gain'))
    retention = {key: tables['recombined']['dense10'][key]/max(tables['recombined']['earned2'][key],1e-300)
                 for key in ('early','gain')}
    quantities = dict(comparisons=comparisons, added_hybrid_bytes=hbytes,
        added_full_bytes=fbytes, priced_margin_m10_bits=m10, priced_margin_m24_bits=m24,
        fine_fraction=FINE/24, retention=retention, portable_bytes=portable,
        recipient_router_bytes=recipient, shared_prefix_identity=prefix_identity,
        recombined_dense_positive=dense_positive,
        fine_visits={regime:{arm:sum(lookup[w,regime]['fine_visits'][arm] for w in WORLDS)
                             for arm in ('dense10','sparse10')} for regime in REGIMES},
        unrelated_admissions=[dict(world=w,arm=a,activation=lookup[w,'unrelated']['arms'][a]['activation'],
                                   gain=lookup[w,'unrelated']['arms'][a]['gain'])
                              for w in WORLDS for a in ARMS
                              if lookup[w,'unrelated']['arms'][a]['activation'] is not None])
    validity = dict(
        archive=all(b['bytes']['pooled_full.bin']<=528 and b['bytes']['bank2_full.bin']<=1056 and
                    b['bytes']['bank4_full.bin']<=2112 and b['bytes']['dense10.bin']==b['bytes']['sparse10.bin'] and
                    b['fine_records']==FINE for b in books),
        source_counts=True, source_only_selection=True,
        normalization=all(l['max_norm_error']<=1e-8 for l in lives),
        exact_quotes=all(all(l['exactness'].values()) for l in lives),
        bounds=all(st['minimum']>=-1-1e-7 and st['drawdown']<=16+1e-7
                   for l in lives for st in l['arms'].values()),
        identical_admissions=admission_identity, shared_prefix=prefix_identity,
        unrelated_disclosed=True)
    conditions = dict(
        G1=d['tail']['mean']>m10 and d['tail']['wins']>=5 and d['gain']['mean']>=0,
        G2=s['tail']['mean']>0 and s['tail']['wins']>=5 and portable['dense10']==portable['sparse10'],
        G3=f['tail']['mean']>0 and d['tail']['mean']>=.5*f['tail']['mean'] and
           hbytes[0]/fbytes[0] <= FINE/24+1e-15,
        G4=dense_positive==8 and min(retention.values())>=.99 and partial_d['mean']>=0,
        G5=False)
    material = all(conditions[k] for k in ('G1','G2','G3','G4')) and all(validity.values())
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        protocol_sha256=sha(HERE/'PROTOCOL.md'), lives=lives, tables=tables,
        quantities=quantities, conditions=conditions, validity=validity,
        material_pass=material, gate_pass=False, independent_reader_pending=True)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json'); check_manifest('MEMORY_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        lives = list(pool.map(run_target, [(w,r) for w in WORLDS for r in REGIMES]))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    result = verdict(lives); save(HERE/'RESULT.json', result)
    print(json.dumps({k:result[k] for k in ('conditions','validity','material_pass','gate_pass','tables')},
                     indent=2,sort_keys=True))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(); parser.add_argument('stage',choices=('freeze','generate','extract','learn','evaluate'))
    args=parser.parse_args()
    if args.stage!='freeze': check_freeze()
    globals()[args.stage]()
