#!/usr/bin/env python3
"""Independent reader for a surface move: rebuilt byte bijections, source
reconstruction and a separate probability-mass replay of both authority laws.

The byte bijection pi_w is rebuilt here from the namespace and world alone,
with this file's own seeding and shuffle, and only then compared with the
hidden generator record. Source books, archives, joint selection, stored
matches and every candidate price come back out of the retained evidence
through the inherited turn13 reader; both authority laws are replayed as
Decimal probability masses through the inherited turn14 reader. Nothing here
consults experiment.py.
"""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(160, 168))
REGIMES = ('recombined', 'moved_whole', 'moved_mid', 'unrelated')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
N = 16384
MOVE = 8192
NAMESPACE = 'netta-surface-move-v1'
HAZARDS = {'slow': Decimal(1)/Decimal(65536), 'fast': Decimal(1)/Decimal(1024)}
TOL = 1e-7


def borrow(name, path):
    """Load a frozen reader under a private name; never edit it, never alias it."""
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


v13 = borrow('turn15_independent_turn13_reader', REPO/'turn13'/'verify.py')
v13.HERE = HERE
v13.REPO = REPO
v13.WORLDS = WORLDS

v14 = borrow('turn15_independent_turn14_reader', REPO/'turn14'/'verify.py')
v14.HERE = HERE
v14.REPO = REPO
v14.WORLDS = WORLDS
v14.REGIMES = REGIMES
v14.NAMESPACE = NAMESPACE


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def surface_map(world):
    """pi_w from the namespace and world alone, with this file's own code."""
    material = '|'.join((NAMESPACE, str(world), 'surface')).encode()
    draw = random.Random(int.from_bytes(hashlib.sha256(material).digest()[:8], 'big'))
    order = list(range(256))
    draw.shuffle(order)
    need(len(order) == 256 and sorted(order) == list(range(256)),
         f'world {world} surface map is a byte permutation')
    need(len(set(order)) == 256, f'world {world} surface map is injective')
    return bytes(order)


def manipulation():
    """What the renaming did to the bytes, read back out of the retained lives."""
    rows = []
    for world in WORLDS:
        folder = HERE/'data'/f'world{world}'
        recombined = (folder/'recombined.bin').read_bytes()
        whole = (folder/'moved_whole.bin').read_bytes()
        mid = (folder/'moved_mid.bin').read_bytes()
        need(len(recombined) == N and len(whole) == N and len(mid) == N,
             f'world {world} renamed lives hold the target horizon')
        pi = surface_map(world)
        hidden = json.loads((folder/'GENERATOR.json').read_text())
        need(list(pi) == hidden['surface_map'], f'world {world} declared surface map')
        need(hidden['surface_move_position'] == MOVE, f'world {world} declared move position')
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
    """Raw moved_mid bytes after the move, recounted from both retained traces."""
    source = HERE/'results'/f'world{world}'/'moved_mid.tsv.gz'
    outer = HERE/'results'/'outer'/f'world{world}'/'moved_mid.tsv.gz'
    best = worst = None
    with gzip.open(source, 'rt') as incoming, gzip.open(outer, 'rt') as replayed:
        rows = csv.DictReader(incoming, delimiter='\t')
        saved = csv.DictReader(replayed, delimiter='\t')
        for t, row in enumerate(rows):
            need(int(row['t']) == t, 'raw scan chronology')
            fast = None
            for arm in ARMS:
                line = next(saved, None)
                need(line is not None and int(line['t']) == t and line['arm'] == arm,
                     'raw scan paired chronology')
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
        need(next(saved, None) is None, 'raw scan complete paired traces')
    need(t+1 == N and best is not None and worst is not None, 'raw scan horizon')
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
    need(len(tests) == 16, 'sixteen material conditions')
    return dict(tables=tables, equivariance=equivariance, surface=surface,
                bounds=bounds, manipulation=manip), tests


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'verification output already exists')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    need(frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS) and
         frozen['regimes'] == list(REGIMES), 'freeze scope')
    for name, wanted in frozen['files'].items():
        need(digest(REPO/name) == wanted, 'frozen code '+name)
    for manifest_name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json',
                          'RESULTS_MANIFEST.json'):
        for path, wanted in json.loads((HERE/manifest_name).read_text()).items():
            need(digest(HERE/path) == wanted, 'retained artifact '+path)
    claim = json.loads((HERE/'RESULT.json').read_text())
    need(claim['namespace'] == NAMESPACE and claim['worlds'] == list(WORLDS) and
         claim['regimes'] == list(REGIMES), 'claim scope')
    need(claim['protocol_sha256'] == frozen['files']['turn15/PROTOCOL.md'], 'claim protocol')
    for entry in claim['norms']:
        need(0 <= entry['max_norm_error'] <= 1e-8, 'claimed C normalization')

    manip = manipulation()
    lives, books, matches = [], [], []
    predictions = inherited_predictions = 0
    max_error = worst_norm = 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            rebuilt = v13.verify_books(world)
            books.append(dict(world=world, **rebuilt[-1]))
            for regime in REGIMES:
                slow, count, error, norm, _ = v13.verify_life(world, regime, rebuilt)
                need(norm <= 1e-8, 'candidate normalization')
                inherited_predictions += count
                max_error = max(max_error, error)
                worst_norm = max(worst_norm, norm)
                matches.append(dict(world=world, regime=regime,
                    arms={arm: dict(matched_events=slow['arms'][arm]['matched_events'],
                                    max_matched_length=slow['arms'][arm]['max_matched_length'])
                          for arm in ARMS},
                    episode_match_lengths=slow['episode_match_lengths']))
                life, count, error = v14.verify_fast(world, regime, slow)
                lives.append(life)
                predictions += count
                max_error = max(max_error, error)
    need(inherited_predictions == len(WORLDS)*len(REGIMES)*N*7, 'all inherited forecasts')
    need(predictions == len(WORLDS)*len(REGIMES)*N*7*2, 'all paired authority forecasts')

    samples = [raw_scan(world) for world in WORLDS]
    extrema = dict(help=max((x['help'] for x in samples), key=lambda x: x['received']),
                   harm=min((x['harm'] for x in samples), key=lambda x: x['received']))
    summary, tests = summarize(lives, manip, worst_norm)
    v14.compare(claim['lives'], lives, 'lives')
    v14.compare(claim['matches'], matches, 'matches')
    v14.compare(claim['summary'], summary, 'summary')
    v14.compare(claim['raw_samples'], samples, 'raw samples')
    v14.compare(claim['raw_extrema'], extrema, 'raw extrema')
    need(claim['tests'] == tests, 'material tests')
    identities = dict(candidate_shared=True, shadow_shared=True, admission_shared=True,
                      slow_bound=True, fast_bound=True)
    need(claim['identities'] == identities, 'identity claims')
    gate = all(tests.values()) and all(identities.values())
    need(claim['gate_pass'] == gate, 'gate verdict')
    result = dict(verification_pass=True, gate_pass=gate, namespace=NAMESPACE,
        worlds=list(WORLDS), regimes=list(REGIMES),
        source_observations=len(WORLDS)*4*N,
        target_observations=len(WORLDS)*len(REGIMES)*N,
        inherited_candidate_forecasts=inherited_predictions,
        paired_authority_forecasts=predictions,
        maximum_numeric_error=max_error, maximum_normalization_error=worst_norm,
        identities=dict(protocol_sha256=frozen['files']['turn15/PROTOCOL.md'],
                        freeze_sha256=digest(HERE/'FREEZE.json'),
                        result_sha256=digest(HERE/'RESULT.json'),
                        reader_sha256=digest(Path(__file__).resolve())),
        source_books=books, summary=summary, tests=tests, raw_extrema=extrema,
        scope='Byte bijections rebuilt from the namespace seed by this file and '
              'checked against the hidden generator record; renaming identities and '
              'the byte-identical moved_mid prefix recounted from the retained lives; '
              'independent turn13 source BPE/archive/joint-selection/candidate '
              'reconstruction; separate Decimal probability-mass replay of both '
              'authority laws; every material condition recomputed here.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
