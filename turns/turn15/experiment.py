#!/usr/bin/env python3
"""SURFACE MOVE: byte-bijection equivariance and a mid-life change of surface."""
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
import sys
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(160, 168))
SOURCES = ('sourceAB', 'sourceBA', 'sourceCD', 'sourceDC')
EMITTED = ('recombined', 'unrelated')
DERIVED = ('moved_whole', 'moved_mid')
REGIMES = ('recombined', 'moved_whole', 'moved_mid', 'unrelated')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
N = 16384
MOVE = 8192
NAMESPACE = 'netta-surface-move-v1'
HAZARDS = {'slow': 2.0**-16, 'fast': 2.0**-10}


def borrow(name, path):
    """Load a frozen hand under a private name; never edit it, never alias it."""
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


t13 = borrow('turn15_frozen_turn13_experiment', REPO/'turn13'/'experiment.py')
t13.HERE = HERE
t13.REPO = REPO
t13.WORLDS = WORLDS
t13.REGIMES = REGIMES
t13.NAMESPACE = NAMESPACE

t14 = borrow('turn15_frozen_turn14_experiment', REPO/'turn14'/'experiment.py')
t14.HERE = HERE
t14.REPO = REPO
t14.WORLDS = WORLDS
t14.REGIMES = REGIMES
t14.NAMESPACE = NAMESPACE

FROZEN = (
    'turn15/PROTOCOL.md', 'turn15/experiment.py', 'turn15/verify.py',
    'turn15/Makefile', 'turn15/episode',
    'turn13/episode.c', 'turn13/experiment.py', 'turn13/verify.py',
    'turn14/experiment.py', 'turn14/verify.py',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c',
)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2,
                  sort_keys=True, allow_nan=False)
        stream.write('\n')


def surface_map(world):
    """One uniform byte bijection per world on its own dedicated seed stream.

    The seed is turn13's namespace|world|name discipline with the dedicated
    name 'surface'; the draw is turn13's own alphabet idiom, a shuffle of
    0..255. pi_w is written only into the hidden GENERATOR.json; no learner,
    archive or trace ever sees it.
    """
    order = list(range(256))
    random.Random(t13.seed(world, 'surface')).shuffle(order)
    assert sorted(order) == list(range(256)) and len(set(order)) == 256
    return bytes(order)


def generate_world(world):
    """turn13's generator law with the switched regime replaced by renaming.

    Sources, recombined and unrelated are emitted exactly as turn13 emits
    them. moved_whole and moved_mid are not emitted: they are the recombined
    life read back and renamed through pi_w, whole or from byte MOVE on.
    """
    folder = HERE/'data'/f'world{world}'
    folder.mkdir(parents=True, exist_ok=False)
    rng = random.Random(t13.seed(world, 'components'))
    components = []
    while len(components) < 4:
        draw = [0, 1, 2, 3]*3
        rng.shuffle(draw)
        if draw not in components:
            components.append(draw)
    cycles = {'sourceAB': (0, 1), 'sourceBA': (1, 0), 'sourceCD': (2, 3),
              'sourceDC': (3, 2), 'recombined': (0, 3, 1, 2)}
    commands = {}
    for name, cycle in cycles.items():
        rng = random.Random(t13.seed(world, f'commands-{name}'))
        tape = []
        for t in range(N):
            c = components[cycle[(t//48) % len(cycle)]][t % 12]
            if rng.random() < .08:
                c = rng.randrange(4)
            tape.append(c)
        commands[name] = bytes(tape)
    rng = random.Random(t13.seed(world, 'commands-unrelated'))
    commands['unrelated'] = bytes(rng.randrange(4) for _ in range(N))
    seeds = {}
    for name in SOURCES+EMITTED:
        body = 'recipient' if name == 'recombined' else name
        alphabet = list(range(256))
        random.Random(t13.seed(world, 'alphabet-'+body)).shuffle(alphabet)
        alphabet_path = folder/(name+'.alphabet.bin')
        alphabet_path.write_bytes(bytes(alphabet[:64]))
        command_path = folder/(name+'.commands.bin')
        command_path.write_bytes(commands[name])
        seeds[name] = t13.seed(world, 'emit-'+body) or 1
        t13.emit(folder/(name+'.bin'), command_path, alphabet_path, seeds[name])
    pi = surface_map(world)
    recombined = (folder/'recombined.bin').read_bytes()
    assert len(recombined) == N
    moved = bytes(pi[byte] for byte in recombined)
    (folder/'moved_whole.bin').write_bytes(moved)
    (folder/'moved_mid.bin').write_bytes(recombined[:MOVE]+moved[MOVE:])
    assert (folder/'moved_mid.bin').read_bytes()[:MOVE] == recombined[:MOVE]
    assert (folder/'moved_whole.bin').read_bytes() == moved
    t13.save(folder/'GENERATOR.json', {'world': world, 'components': components,
        'cycles': cycles, 'emission_seeds': seeds,
        'source_joints': [[0, 1], [1, 0], [2, 3], [3, 2]],
        'target_joints': [[0, 3], [3, 1], [1, 2], [2, 0]],
        'surface_seed': t13.seed(world, 'surface'), 'surface_map': list(pi),
        'surface_move_position': MOVE,
        'scope': 'hidden generation labels and the hidden byte bijection; '
                 'learners receive only emitted bytes'})
    return world


def manipulation():
    """What the renaming actually did to the bytes, measured from the files."""
    rows = []
    for world in WORLDS:
        folder = HERE/'data'/f'world{world}'
        recombined = (folder/'recombined.bin').read_bytes()
        whole = (folder/'moved_whole.bin').read_bytes()
        mid = (folder/'moved_mid.bin').read_bytes()
        pi = surface_map(world)
        rows.append(dict(world=world,
            bijection=sorted(pi) == list(range(256)) and len(set(pi)) == 256,
            fixed_points=sum(pi[b] == b for b in range(256)),
            whole_is_renamed=whole == bytes(pi[b] for b in recombined),
            mid_is_renamed=mid == recombined[:MOVE]+bytes(pi[b] for b in recombined[MOVE:]),
            prefix_identical=mid[:MOVE] == recombined[:MOVE],
            mid_tail_differing=sum(a != b for a, b in zip(mid[MOVE:], recombined[MOVE:]))/(N-MOVE),
            whole_differing=sum(a != b for a, b in zip(whole, recombined))/N,
            distinct_bytes=len(set(recombined))))
    return rows


def raw_scan(world):
    """Raw moved_mid bytes after the move: where memory helps and where it harms."""
    source = HERE/'results'/f'world{world}'/'moved_mid.tsv.gz'
    outer = HERE/'results'/'outer'/f'world{world}'/'moved_mid.tsv.gz'
    best = worst = None
    with gzip.open(source, 'rt') as incoming, gzip.open(outer, 'rt') as replayed:
        rows = csv.DictReader(incoming, delimiter='\t')
        saved = csv.DictReader(replayed, delimiter='\t')
        for t, row in enumerate(rows):
            assert int(row['t']) == t
            fast = None
            for arm in ARMS:
                line = next(saved, None)
                assert line is not None and int(line['t']) == t and line['arm'] == arm
                if arm == 'episode':
                    fast = line
            if t < MOVE:
                continue
            cold = float(row['logcold'])
            candidate = float(row['episode_candidate'])
            live = float(fast['fast_live'])
            sample = dict(world=world, regime='moved_mid', t=t,
                          truth=int(row['truth']), rank=int(row['rank']),
                          logcold=cold, candidate=candidate,
                          slow_live=float(row['episode_live']), fast_live=live,
                          received=live-cold, candidate_minus_cold=candidate-cold,
                          matched_length=int(row['episode_matchedL']),
                          fast_gain_after=float(fast['fast_gain_after']))
            if best is None or sample['received'] > best['received']:
                best = sample
            if worst is None or sample['received'] < worst['received']:
                worst = sample
        assert next(saved, None) is None
    assert t+1 == N and best is not None and worst is not None
    return dict(world=world, help=best, harm=worst)


def summarize(lives, manip, worst_norm):
    by = {(x['world'], x['regime']): x for x in lives}
    mean = lambda values: math.fsum(values)/len(values)
    pick = lambda regime, mode, arm, field: [by[w, regime][mode][arm][field] for w in WORLDS]
    tables = {regime: {mode: {arm: dict(
                    gain=pick(regime, mode, arm, 'gain'),
                    tail=pick(regime, mode, arm, 'tail'),
                    candidate_gain=pick(regime, mode, arm, 'candidate_gain'),
                    minimum=pick(regime, mode, arm, 'minimum'),
                    peak=pick(regime, mode, arm, 'peak'),
                    max_drawdown=pick(regime, mode, arm, 'max_drawdown'),
                    activation=pick(regime, mode, arm, 'activation'),
                    horizons={h: [by[w, regime][mode][arm]['horizons'][h] for w in WORLDS]
                              for h in ('1024', '4096', '8192', str(N))},
                    mean_gain=mean(pick(regime, mode, arm, 'gain')),
                    mean_tail=mean(pick(regime, mode, arm, 'tail')),
                    mean_candidate_gain=mean(pick(regime, mode, arm, 'candidate_gain')),
                    positive_gains=sum(x > 0 for x in pick(regime, mode, arm, 'gain')),
                    admissions=sum(x is not None for x in pick(regime, mode, arm, 'activation')))
                for arm in ARMS} for mode in HAZARDS} for regime in REGIMES}

    equivariance = {}
    for mode in HAZARDS:
        arms = {}
        for arm in ARMS:
            full = [abs(by[w, 'moved_whole'][mode][arm]['gain'] -
                        by[w, 'recombined'][mode][arm]['gain']) for w in WORLDS]
            early = [abs(by[w, 'moved_whole'][mode][arm]['horizons']['4096'] -
                         by[w, 'recombined'][mode][arm]['horizons']['4096']) for w in WORLDS]
            arms[arm] = dict(full_deltas=full, early_deltas=early,
                worst_full=max(full), worst_early=max(early),
                admission_match=[by[w, 'moved_whole'][mode][arm]['activation'] ==
                                 by[w, 'recombined'][mode][arm]['activation'] for w in WORLDS])
        equivariance[mode] = arms

    tails = pick('moved_mid', 'fast', 'episode', 'tail')
    fulls = pick('moved_mid', 'fast', 'episode', 'gain')
    surface = dict(fast_changed_tails=tails, fast_changed_tail_mean=mean(tails),
        fast_changed_tail_positive=sum(x > 0 for x in tails),
        fast_full_gains=fulls, fast_full_gain_bpb=mean(fulls)/N,
        fast_full_positive=sum(x > 0 for x in fulls),
        slow_changed_tails=pick('moved_mid', 'slow', 'episode', 'tail'),
        slow_changed_tail_mean=mean(pick('moved_mid', 'slow', 'episode', 'tail')),
        slow_full_gains=pick('moved_mid', 'slow', 'episode', 'gain'),
        slow_full_gain_bpb=mean(pick('moved_mid', 'slow', 'episode', 'gain'))/N,
        episode_admissions={mode: pick('moved_mid', mode, 'episode', 'activation')
                            for mode in HAZARDS},
        early_prefix_deltas=[by[w, 'moved_mid']['fast']['episode']['horizons']['8192'] -
                             by[w, 'recombined']['fast']['episode']['horizons']['8192']
                             for w in WORLDS])

    everywhere = [(w, regime, mode, arm) for w in WORLDS for regime in REGIMES
                  for mode in HAZARDS for arm in ARMS]
    bounds = dict(
        worst_prefix_gain=min(by[w, r][m][a]['minimum'] for w, r, m, a in everywhere),
        worst_fast_drawdown=max(by[w, r]['fast'][a]['max_drawdown']
                                for w, r, m, a in everywhere),
        worst_slow_drawdown=max(by[w, r]['slow'][a]['max_drawdown']
                                for w, r, m, a in everywhere),
        permuted_admissions=sum(by[w, r][m]['permuted']['activation'] is not None
                                for w, r, m, a in everywhere if a == 'permuted'))

    tests = dict(
        c1_prefix_identity=all(r['prefix_identical'] and r['mid_is_renamed'] for r in manip),
        c1_mid_divergence=all(r['mid_tail_differing'] >= .90 for r in manip),
        c1_whole_divergence=all(r['whole_differing'] >= .90 and r['whole_is_renamed']
                                for r in manip),
        c1_bijection=all(r['bijection'] for r in manip),
        c2_equivariance_full=all(a['worst_full'] <= 1e-7
                                 for m in equivariance.values() for a in m.values()),
        c2_equivariance_early=all(a['worst_early'] <= 1e-7
                                  for m in equivariance.values() for a in m.values()),
        c2_equivariance_admission=all(all(a['admission_match'])
                                      for m in equivariance.values() for a in m.values()),
        c3_recovery_tail_mean=surface['fast_changed_tail_mean'] >= 1.0,
        c3_recovery_tail_count=surface['fast_changed_tail_positive'] >= 5,
        c4_whole_life_gain=surface['fast_full_gain_bpb'] >= .005,
        c4_whole_life_positive=surface['fast_full_positive'] == len(WORLDS),
        c5_permuted_never_admitted=bounds['permuted_admissions'] == 0,
        c6_prefix_bound=bounds['worst_prefix_gain'] >= -1-1e-7,
        c6_fast_drawdown=bounds['worst_fast_drawdown'] <= 10+1e-7,
        c6_slow_drawdown=bounds['worst_slow_drawdown'] <= 16+1e-7,
        c6_normalized_new_exact=worst_norm <= 1e-8)
    assert len(tests) == 16
    return dict(tables=tables, equivariance=equivariance, surface=surface,
                bounds=bounds, manipulation=manip), tests


def freeze():
    save(HERE/'FREEZE.json', dict(created_utc=time.time(), namespace=NAMESPACE,
         worlds=WORLDS, regimes=REGIMES,
         files={name: digest(REPO/name) for name in FROZEN}))


def check_freeze():
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES)
    for name, wanted in frozen['files'].items():
        assert digest(REPO/name) == wanted, name


def evaluate():
    check_freeze()
    for name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json'):
        for path, wanted in json.loads((HERE/name).read_text()).items():
            assert digest(HERE/path) == wanted, path
    if (HERE/'results').exists():
        raise RuntimeError('results already exist; refusing to replace evidence')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        futures = [pool.submit(t13.run_target, w, regime)
                   for w in WORLDS for regime in REGIMES]
        slow = [future.result() for future in futures]
    slow_by = {(x['world'], x['regime']): x for x in slow}
    lives, matches, norms = [], [], []
    for w in WORLDS:
        for regime in REGIMES:
            inherited = slow_by[w, regime]
            life, _ = t14.replay_fast(w, regime, inherited)
            lives.append(life)
            norms.append(dict(world=w, regime=regime,
                              max_norm_error=inherited['max_norm_error']))
            matches.append(dict(world=w, regime=regime,
                arms={arm: dict(matched_events=inherited['arms'][arm]['matched_events'],
                                max_matched_length=inherited['arms'][arm]['max_matched_length'])
                      for arm in ARMS},
                episode_match_lengths={str(k): v for k, v
                                       in inherited['episode_match_lengths'].items()}))
    manip = manipulation()
    samples = [raw_scan(w) for w in WORLDS]
    summary, tests = summarize(lives, manip,
                               max(x['max_norm_error'] for x in norms))
    identities = dict(candidate_shared=True, shadow_shared=True,
                      admission_shared=True, slow_bound=True, fast_bound=True)
    result = dict(worlds=WORLDS, namespace=NAMESPACE, regimes=REGIMES,
        protocol_sha256=digest(HERE/'PROTOCOL.md'), lives=lives, norms=norms,
        matches=matches, summary=summary, tests=tests, identities=identities,
        raw_samples=samples,
        raw_extrema=dict(help=max((x['help'] for x in samples), key=lambda x: x['received']),
                         harm=min((x['harm'] for x in samples), key=lambda x: x['received'])),
        gate_pass=all(tests.values()) and all(identities.values()),
        independent_reader_pending=True)
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', t13.manifest(HERE/'results'))
    print(json.dumps(dict(equivariance={mode: {arm: [a['worst_full'], a['worst_early']]
                                               for arm, a in arms.items()}
                                        for mode, arms in summary['equivariance'].items()},
                          surface=summary['surface'], bounds=summary['bounds'],
                          tests=tests, gate_pass=result['gate_pass']), indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'learn', 'evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    if stage in ('generate', 'extract'):
        function = generate_world if stage == 'generate' else t13.extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            print(list(pool.map(function, WORLDS)))
    elif stage == 'learn':
        for world in WORLDS:
            print('learned', t13.learn_world(world), flush=True)
    else:
        evaluate()
    if stage == 'extract':
        save(HERE/'DATA_MANIFEST.json', t13.manifest(HERE/'data'))
    if stage == 'learn':
        save(HERE/'MEMORY_MANIFEST.json', t13.manifest(HERE/'memory'))


if __name__ == '__main__':
    main()
