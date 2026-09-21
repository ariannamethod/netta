#!/usr/bin/env python3
"""One frozen BANK2-ROW experiment; standard library, explicit stage boundaries."""
import argparse
import concurrent.futures
import contextlib
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(32, 40))
N = 16384
HAZARD = 2.0 ** -16
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
SOURCES = ('sourceA0', 'sourceA1', 'sourceB0', 'sourceB1')
ARMS = ('row', 'global', 'pool', 'null0', 'null1', 'null2')
REAL = ARMS[:3]


def patterns(prefix=(0,)):
    if len(prefix) == 6:
        yield ''.join(map(str, prefix))
    else:
        for j in range(max(prefix) + 2):
            yield from patterns(prefix + (j,))


PATTERNS = tuple(patterns())
K = {p: max(map(int, p)) + 1 for p in PATTERNS}
assert len(PATTERNS) == 203 and sum(k + 1 for k in K.values()) == 877


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(obj, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): digest(p)
            for p in sorted(folder.rglob('*')) if p.is_file()}


def check_manifest(path):
    for name, expected in json.loads(path.read_text()).items():
        if digest(HERE / name) != expected:
            raise RuntimeError(f'input changed: {name}')


def seed(world, component):
    return int.from_bytes(hashlib.sha256(
        f'netta-bank-v1|{world}|{component}'.encode()).digest()[:8], 'big')


def la(a, b):
    if a == -math.inf:
        return b
    if b == -math.inf:
        return a
    top = max(a, b)
    return top + math.log1p(2.0 ** (-abs(a - b))) / math.log(2.0)


def weight(ell):
    return 1.0 / (1.0 + 2.0 ** -ell) if ell >= 0 else 2.0 ** ell / (1.0 + 2.0 ** ell)


def canonical(values):
    unique, result = [], ''
    for x in values:
        if x not in unique:
            unique.append(x)
        result += str(unique.index(x))
    return result, unique


def frozen_files():
    names = ('turn3/PROTOCOL.md', 'turn3/experiment.py', 'turn3/bank.c',
             'turn3/Makefile', 'turn3/bank',
             'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
             'byte_recurrence/trace.c', 'byte_recurrence/collect.c',
             'byte_recurrence/Makefile', 'byte_recurrence/byte_trace',
             'byte_recurrence/byte_collect', 'portable_recurrence/recurrence.c',
             'portable_recurrence/recurrence.h', 'court4/transfer4_confirm_core.c')
    return [REPO / name for name in names]


def freeze():
    save(HERE / 'CODE_FREEZE.json', {
        'created_utc': time.time(), 'namespace': 'netta-bank-v1', 'worlds': WORLDS,
        'protocol_sha256': digest(HERE / 'PROTOCOL.md'),
        'files': {str(p.relative_to(REPO)): digest(p) for p in frozen_files()}})


def check_freeze():
    frozen = json.loads((HERE / 'CODE_FREEZE.json').read_text())
    for rel, expected in frozen['files'].items():
        if digest(REPO / rel) != expected:
            raise RuntimeError(f'frozen code changed: {rel}')


def laws(world):
    a, b, mosaic, route = {'000000': 0}, {'000000': 0}, {'000000': 0}, {'000000': 'shared'}
    for k in range(2, 7):
        ordered = [p for p in PATTERNS if K[p] == k]
        shuffled = ordered[:]
        random.Random(seed(world, f'shared-rows-k{k}')).shuffle(shuffled)
        shared = set(shuffled[:len(shuffled) // 2])
        ra = random.Random(seed(world, f'law-A-k{k}'))
        rb = random.Random(seed(world, f'law-B-k{k}'))
        for p in ordered:
            a[p] = ra.randrange(k)
            b[p] = a[p] if p in shared else [j for j in range(k) if j != a[p]][rb.randrange(k-1)]
        different = [p for p in ordered if a[p] != b[p]]
        random.Random(seed(world, f'mosaic-routing-k{k}')).shuffle(different)
        first = random.Random(seed(world, f'mosaic-first-k{k}')).randrange(2)
        for i, p in enumerate(different):
            route[p] = ('A', 'B')[(i + first) % 2]
        for p in ordered:
            if p in shared:
                route[p] = 'shared'
            mosaic[p] = b[p] if route[p] == 'B' else a[p]
    rng = random.Random(seed(world, 'unrelated-grammar'))
    unrelated = {p: rng.randrange(K[p]) for p in PATTERNS}
    assert set(a) == set(b) == set(mosaic) == set(PATTERNS)
    assert mosaic != a and mosaic != b
    return a, b, mosaic, unrelated, route


def stream(law, other, mode, trajectory_seed, rename_seed):
    rng, data = random.Random(trajectory_seed), []
    for t in range(N):
        if t < 6:
            data.append(rng.randrange(256))
            continue
        p, names = canonical(data[-6:])
        current = other if mode == 'unrelated' or (mode == 'mosaic_then_unrelated' and t >= N//2) else law
        z = rng.random()
        if z < .35:
            options = [x for x in range(256) if x not in names]
            nxt = options[rng.randrange(len(options))]
        elif len(names) == 1 or z < .90:
            nxt = names[current[p]]
        else:
            options = [j for j in range(len(names)) if j != current[p]]
            nxt = names[options[rng.randrange(len(options))]]
        data.append(nxt)
    permutation = list(range(256))
    random.Random(rename_seed).shuffle(permutation)
    return bytes(permutation[x] for x in data), permutation


def generate():
    check_freeze()
    if (HERE / 'data').exists():
        raise RuntimeError('data already exists; no redraw or overwrite')
    for world in WORLDS:
        folder = HERE / 'data' / f'world{world:02d}'
        folder.mkdir(parents=True)
        a, b, target, other, route = laws(world)
        meta = {'world': world, 'law_A': a, 'law_B': b, 'mosaic_law': target,
                'unrelated_law': other, 'hidden_raw_route': route, 'files': {}}
        for name in SOURCES + REGIMES:
            source = name in SOURCES
            trajectory = f'{name}-trajectory' if source else 'target-trajectory'
            rename = f'{name}-rename' if source else 'target-rename'
            law = (a if name.startswith('sourceA') else b) if source else target
            raw, permutation = stream(law, other, 'mosaic' if source else name,
                                      seed(world, trajectory), seed(world, rename))
            path = folder / f'{name}.bin'
            with path.open('xb') as output:
                output.write(raw)
            meta['files'][name] = {'sha256': digest(path), 'bytes': len(raw),
                'trajectory_seed': seed(world, trajectory), 'rename_seed': seed(world, rename),
                'permutation': permutation}
        assert (folder/'mosaic.bin').read_bytes()[:8192] == (folder/'mosaic_then_unrelated.bin').read_bytes()[:8192]
        save(folder/'generation.json', meta)
        print('generated', world, flush=True)
    save(HERE/'data/MANIFEST.json', manifest(HERE/'data'))


def run_gzip(command, source, destination, stderr):
    destination.parent.mkdir(parents=True, exist_ok=True)
    with source.open('rb') as stdin, destination.open('xb') as rawout, stderr.open('xb') as err:
        process = subprocess.Popen(command, stdin=stdin, stdout=subprocess.PIPE, stderr=err)
        with gzip.GzipFile(fileobj=rawout, mode='wb', mtime=0, compresslevel=1) as zipped:
            assert process.stdout is not None
            for block in iter(lambda: process.stdout.read(1 << 20), b''):
                zipped.write(block)
        process.stdout.close()
        if process.wait() != 0:
            raise RuntimeError(f'command failed: {command}; see {stderr}')


def extract_one(job):
    world, name = job
    folder = HERE/'traces'/f'world{world:02d}'
    run_gzip([str(REPO/'byte_recurrence/byte_trace')],
             HERE/'data'/f'world{world:02d}'/f'{name}.bin',
             folder/f'{name}.tsv.gz', folder/f'{name}.stderr')
    return job


def unpack_book(raw):
    if len(raw) != 7032 or raw[:8] != b'NETHD256' or struct.unpack('<II', raw[8:16]) != (6, 877):
        raise ValueError('invalid HEAD256 book')
    flat, offset, counts = struct.unpack('<877Q', raw[16:]), 0, {}
    for p in PATTERNS:
        counts[p] = list(flat[offset:offset + K[p] + 1])
        offset += K[p] + 1
        if sum(counts[p]) > 2**53:
            raise ValueError('book support outside exact integer range')
    return counts


def load_bank(path):
    raw = path.read_bytes()
    if len(raw) != 14080 or raw[:8] != b'NETBANK1' or struct.unpack('<II', raw[8:16]) != (2, 0):
        raise ValueError('invalid BANK2 archive')
    return [unpack_book(raw[16:7048]), unpack_book(raw[7048:])]


def collect(world):
    folder = HERE/'memory'/f'world{world:02d}'
    folder.mkdir(parents=True)
    payloads = []
    for book in ('A', 'B'):
        path = folder/f'{book}.bin'
        command = [str(REPO/'byte_recurrence/byte_collect'), str(path)] + [
            str(HERE/'data'/f'world{world:02d}'/f'source{book}{i}.bin') for i in range(2)]
        with (folder/f'collect-{book}.stdout').open('xb') as out, (folder/f'collect-{book}.stderr').open('xb') as err:
            subprocess.run(command, stdout=out, stderr=err, check=True)
        payloads.append(path.read_bytes())
        unpack_book(payloads[-1])
    packed = b'NETBANK1' + struct.pack('<II', 2, 0) + b''.join(payloads)
    with (folder/'bank.bin').open('xb') as out:
        out.write(packed)
    assert load_bank(folder/'bank.bin') == [unpack_book(raw) for raw in payloads]
    return world


def extract():
    check_freeze()
    check_manifest(HERE/'data/MANIFEST.json')
    if (HERE/'traces').exists() or (HERE/'memory').exists():
        raise RuntimeError('extraction outputs already exist')
    jobs = [(w, name) for w in WORLDS for name in SOURCES + REGIMES]
    with concurrent.futures.ProcessPoolExecutor(max_workers=4) as workers:
        for job in workers.map(extract_one, jobs):
            print('extracted', *job, flush=True)
        for world in workers.map(collect, WORLDS):
            print('collected', world, flush=True)
    save(HERE/'traces/MANIFEST.json', manifest(HERE/'traces'))
    save(HERE/'memory/MANIFEST.json', manifest(HERE/'memory'))


def rows(world, name):
    with gzip.open(HERE/'traces'/f'world{world:02d}'/f'{name}.tsv.gz', 'rt') as stream:
        yield from csv.DictReader(stream, delimiter='\t')


def normalized_book(counts):
    return {p: [(x+.5)/(sum(c)+.5*(K[p]+1)) for x in c]
            for p, c in counts.items()}


def cargos(world, books):
    original = [normalized_book(book) for book in books]
    pooled = {p: [a+b for a,b in zip(books[0][p], books[1][p])] for p in PATTERNS}
    distributions, donors = {'real': original}, {}
    for j in range(3):
        rng, mapping = random.Random(seed(world, f'head-bank-null-{j}')), {}
        for k in range(1, 7):
            ordered = [p for p in PATTERNS if K[p] == k]
            shift = rng.randrange(1, len(ordered)) if len(ordered) > 1 else 0
            mapping.update({p: ordered[(i+shift) % len(ordered)] for i,p in enumerate(ordered)})
        shifted = []
        for book in original:
            shifted.append({p: [x*(1-book[p][-1])/(1-book[mapping[p]][-1])
                                for x in book[mapping[p]][:-1]] + [book[p][-1]]
                            for p in PATTERNS})
        distributions[f'null{j}'], donors[f'null{j}'] = shifted, mapping
    return distributions, donors, pooled, normalized_book(pooled)


def component(q0, mass, local, imported, support):
    k, total = len(mass)-1, sum(local)
    if k <= 1 or total < 1 or support < 32:
        return q0[:]
    r1 = [(b+32*a)/(total+32) for b,a in zip(local, imported)]
    q1 = [.5*m+.5*r for m,r in zip(mass, r1)]
    result = [x*(1-q0[-1])/(1-q1[-1]) for x in q1[:-1]] + [q0[-1]]
    assert all(math.isfinite(x) and x > 0 for x in result)
    assert abs(math.fsum(result)-1) < 1e-8
    return result


def mixed(a, b, ell):
    wb = weight(ell)
    return [(1-wb)*x + wb*y for x,y in zip(a,b)]


def initial():
    return dict(shadow=0.0, odds=0.0, active=False, gain=0.0, minimum=0.0,
                peak=0.0, drawdown=0.0, activation=None, at8192=0.0,
                inner_score=0.0, active_events=0, horizons={}, global_ell=0.0,
                row_ell={p: 0.0 for p in PATTERNS})


def c_run(world, regime, mode):
    folder = HERE/'results/c'/f'world{world:02d}'
    destination = folder/f'{regime}-{mode}.tsv.gz'
    run_gzip([str(HERE/'bank'), str(HERE/'memory'/f'world{world:02d}'/'bank.bin'), mode],
             HERE/'data'/f'world{world:02d}'/f'{regime}.bin', destination,
             folder/f'{regime}-{mode}.stderr')
    return destination


def raw_routing_diagnostic(world, regime):
    # Called only after the complete recipient replay. Never reaches any quote.
    folder = HERE/'data'/f'world{world:02d}'
    route = json.loads((folder/'generation.json').read_text())['hidden_raw_route']
    raw = (folder/f'{regime}.bin').read_bytes()
    visits = {part: {slot: 0 for slot in ('A', 'B', 'shared')} for part in ('first8192', 'last8192')}
    for t in range(6, len(raw)):
        pattern, _ = canonical(raw[t-6:t])
        visits['first8192' if t < 8192 else 'last8192'][route[pattern]] += 1
    return visits


def evaluate_one(world):
    books = load_bank(HERE/'memory'/f'world{world:02d}'/'bank.bin')
    distributions, donors, pooled, pool_prior = cargos(world, books)
    support = [{p: sum(book[p]) for p in PATTERNS} for book in books]
    source_meta = {'observation_law': 'HEAD256-v1', 'world': world, 'counts': books,
                   'support': support, 'null_donors': donors,
                   'null_seeds': [seed(world, f'head-bank-null-{j}') for j in range(3)]}
    save(HERE/'results'/f'world{world:02d}-memory.json', source_meta)
    result = {'world': world, 'bank_bytes': 14080, 'source_bytes': 4*N,
              'source_events': [sum(s.values()) for s in support],
              'supported_rows': [sum(n>=32 for n in s.values()) for s in support], 'regimes': {}}
    event_folder = HERE/'results/events'/f'world{world:02d}'
    event_folder.mkdir(parents=True)
    for regime in REGIMES:
        c_paths = {mode: c_run(world, regime, mode) for mode in REAL}
        local = {p: [0]*(K[p]+1) for p in PATTERNS}
        states = {arm: initial() for arm in ARMS}
        scores = {arm: {p: [0.0, 0.0] for p in PATTERNS} for arm in ARMS if arm != 'pool'}
        row_gains = {p: 0.0 for p in PATTERNS}
        raw = (HERE/'data'/f'world{world:02d}'/f'{regime}.bin').read_bytes()
        norm_max, c_max, cold_loss, base_loss = 0.0, 0.0, 0.0, 0.0
        fields = ('t','pattern','truth','group','logbase','logcold','arm','logA','logB',
                  'logbank','loglive','inner_odds_before','inner_odds_after',
                  'shadow_before','outer_odds_before','active_before','activated_after','gain_after')
        with contextlib.ExitStack() as stack:
            readers = {mode: csv.DictReader(stack.enter_context(gzip.open(path, 'rt')), delimiter='\t')
                       for mode,path in c_paths.items()}
            out = stack.enter_context(gzip.open(event_folder/f'{regime}.tsv.gz', 'xt'))
            writer = csv.writer(out, delimiter='\t')
            writer.writerow(fields)
            for t, trace in enumerate(rows(world, regime)):
                assert int(trace['t']) == t and t < N and int(trace['truth']) == raw[t]
                assert int(trace['trained_bytes']) == (t//1024)*1024
                base = [2.0 ** float(trace[f'logp{i}']) for i in range(256)]
                assert all(math.isfinite(x) and x > 0 for x in base)
                norm_max = max(norm_max, abs(math.fsum(base)-1.0))
                assert norm_max < 1e-8
                logbase, p, g = float(trace['logp_base_truth']), trace['pattern'], int(trace['group'])
                assert abs(logbase-math.log2(base[raw[t]])) < 1e-10
                if p == '-':
                    assert g == -1 and int(trace['context_len']) < 6
                    mass, q0, b, group = [1.0], [1.0], [], 0
                else:
                    expansions = [bytes.fromhex(x) for x in trace['unit_hex'].split(',')]
                    computed, heads = canonical([x[0] for x in expansions])
                    assert len(expansions) == 6 and computed == p
                    assert heads == list(map(int, trace['heads'].split(',')))
                    group = heads.index(raw[t]) if raw[t] in heads else K[p]
                    assert g == group
                    mass = [base[h] for h in heads] + [math.fsum(x for c,x in enumerate(base) if c not in heads)]
                    b, total = local[p], sum(local[p])
                    q0 = [.5*m+.5*(v+.5)/(total+.5*len(mass)) for m,v in zip(mass,b)]
                assert abs(math.fsum(q0)-1.0) < 1e-8
                logcold = logbase + math.log2(q0[group]/mass[group])
                cold_loss -= logcold
                base_loss -= logbase
                components = {}
                for name, pair in distributions.items():
                    components[name] = [component(q0, mass, b, book[p], support[j][p])
                        for j,book in enumerate(pair)] if p != '-' else [q0[:], q0[:]]
                pool_quote = component(q0, mass, b, pool_prior[p], sum(pooled[p])) if p != '-' else q0[:]
                for arm in ARMS:
                    state = states[arm]
                    pair = components[arm if arm.startswith('null') else 'real']
                    ell = (state['global_ell'] if arm == 'global' else state['row_ell'][p]) if p != '-' and arm != 'pool' else 0.0
                    qbank = pool_quote if arm == 'pool' else mixed(pair[0], pair[1], ell)
                    if p != '-':
                        # Exact identity on NEW, including when likelihood odds are extreme.
                        qbank[-1] = q0[-1]
                    assert all(math.isfinite(x) and x > 0 for x in qbank)
                    norm_max = max(norm_max, abs(math.fsum(qbank)-1.0))
                    assert norm_max < 1e-8
                    if p != '-':
                        assert abs(qbank[-1]-q0[-1]) < 1e-10
                    loga, logb = [logbase+math.log2(q[group]/mass[group]) for q in pair]
                    logbank = logbase+math.log2(qbank[group]/mass[group])
                    active, outer, shadow = state['active'], state['odds'], state['shadow']
                    qlive = mixed(q0, qbank, outer) if active else q0
                    if p != '-':
                        qlive[-1] = q0[-1]
                    assert all(math.isfinite(x) and x > 0 for x in qlive)
                    norm_max = max(norm_max, abs(math.fsum(qlive)-1.0))
                    assert norm_max < 1e-8
                    loglive = logbase+math.log2(qlive[group]/mass[group])
                    delta = logbank-logcold
                    state['inner_score'] += delta
                    state['gain'] += loglive-logcold
                    state['minimum'] = min(state['minimum'], state['gain'])
                    state['peak'] = max(state['peak'], state['gain'])
                    state['drawdown'] = max(state['drawdown'], state['peak']-state['gain'])
                    assert state['minimum'] >= -1.0-1e-7 and state['drawdown'] <= 16.0+1e-7
                    state['shadow'] += delta
                    activated = False
                    if active:
                        state['active_events'] += 1
                        z = outer+delta
                        state['odds'] = math.log1p(-HAZARD)/math.log(2.0)-la(-z, math.log2(HAZARD))
                    elif state['shadow'] >= 32.0:
                        state['active'], state['odds'], state['activation'] = True, 0.0, t+1
                        activated = True
                    update = 0.0 if p == '-' or group == K[p] else logb-loga
                    after = ell
                    if p != '-' and arm != 'pool':
                        scores[arm][p][0] += loga-logcold
                        scores[arm][p][1] += logb-logcold
                        after += update
                        if arm == 'global':
                            state['global_ell'] = after
                        else:
                            state['row_ell'][p] = after
                    if arm == 'row' and p != '-':
                        row_gains[p] += loglive-logcold
                    if t+1 in (1024, 4096, 8192, 16384):
                        state['horizons'][str(t+1)] = state['gain']
                    if t == 8191:
                        state['at8192'] = state['gain']
                    if arm in REAL:
                        c = next(readers[arm])
                        assert int(c['t']) == t and c['pattern'] == p
                        assert int(c['truth']) == raw[t] and int(c['group']) == g
                        assert int(c['active_before']) == int(active) and int(c['activated_after']) == int(activated)
                        expected = dict(logbase=logbase, logcold=logcold, logA=loga, logB=logb,
                            logbank=logbank, loglive=loglive, inner_odds_before=ell,
                            shadow_before=shadow, outer_odds_before=outer, gain_after=state['gain'])
                        for key,value in expected.items():
                            error = abs(float(c[key])-value)
                            c_max = max(c_max,error)
                            assert error < 1e-7, (world,regime,arm,t,key,error)
                    writer.writerow((t,p,raw[t],g,logbase,logcold,arm,loga,logb,logbank,loglive,
                                     ell,after,shadow,outer,int(active),int(activated),state['gain']))
                if p != '-':
                    local[p][group] += 1
            assert t+1 == N
            for reader in readers.values():
                assert next(reader, None) is None
        oracle = {}
        for arm in ARMS:
            state = states[arm]
            state['tail_gain'] = state['gain']-state['at8192']
            if arm != 'pool':
                totals = [math.fsum(v[j] for v in scores[arm].values()) for j in (0,1)]
                row_best = math.fsum(max(v) for v in scores[arm].values())
                identity = (la(*totals)-1.0 if arm == 'global' else
                            math.fsum(la(*v)-1.0 for v in scores[arm].values()))
                error = abs(identity-state['inner_score'])
                assert error < 1e-7
                visited = sum(sum(local[p]) > 0 for p in PATTERNS)
                regret = (max(totals) if arm == 'global' else row_best)-state['inner_score']
                assert -1e-7 <= regret <= (1 if arm == 'global' else visited)+1e-7
                oracle[arm] = {'component_scores': totals, 'best_whole': max(totals),
                    'best_per_row': row_best, 'mixture_identity': identity,
                    'identity_error': error, 'regret': regret, 'visited_rows': visited,
                    'row_component_scores': scores[arm]}
        result['regimes'][regime] = {'arms': states, 'oracle': oracle, 'row_gains': row_gains,
            'cold_loss': cold_loss, 'base_loss': base_loss, 'max_norm_error': norm_max,
            'max_c_difference': c_max, 'raw_routing_visits': raw_routing_diagnostic(world,regime)}
        print('evaluated', world, regime, states['row']['gain'], flush=True)
    save(HERE/'results'/f'world{world:02d}.json', result)
    return result


def evaluate():
    check_freeze()
    for name in ('data', 'traces', 'memory'):
        check_manifest(HERE/name/'MANIFEST.json')
    if (HERE/'results').exists():
        raise RuntimeError('results already exist; no overwrite')
    with concurrent.futures.ProcessPoolExecutor(max_workers=4) as workers:
        worlds = list(workers.map(evaluate_one, WORLDS))
    paired = []
    for result in worlds:
        arms = result['regimes']['mosaic']['arms']
        row = arms['row']['gain']
        paired.append({'world': result['world'], 'gain': row,
            'best_null_contrast': row-max(arms[f'null{j}']['gain'] for j in range(3)),
            'global_contrast': row-arms['global']['gain'], 'pool_contrast': row-arms['pool']['gain']})
    means = {name+'_bpb': math.fsum(p[name] for p in paired)/len(WORLDS)/N
             for name in ('gain', 'best_null_contrast', 'global_contrast', 'pool_contrast')}
    tests = {'mean_vs_cold': means['gain_bpb'] > .01,
             'mean_vs_best_null': means['best_null_contrast_bpb'] > .01,
             'all_worlds_positive': all(p['gain'] > 0 for p in paired),
             'mean_vs_global': means['global_contrast_bpb'] > .002,
             'mean_vs_pool': means['pool_contrast_bpb'] > .002}
    save(HERE/'results/SUMMARY.json', {'protocol_sha256': digest(HERE/'PROTOCOL.md'),
        'worlds': worlds, 'paired_mosaic': paired, 'metrics': means, 'tests': tests,
        'material_pass': all(tests.values()), 'independent_gate_pending': True})
    save(HERE/'results/MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps({'metrics': means, 'tests': tests, 'material_pass': all(tests.values())}, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'evaluate'))
    args = parser.parse_args()
    globals()[args.stage]()
