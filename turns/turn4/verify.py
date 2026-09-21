#!/usr/bin/env python3
"""Independent reader for BANK2-COLD-ROW: absolute capitals and HMM paths."""

import argparse
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(REPO / 'turn3'))
import verify as old  # noqa: E402: frozen independent HEAD256 trace reader

WORLDS = range(40, 48)
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
N = 16384
PRIOR = (1/8, 7/16, 7/16)
MAX_ERROR = 1e-8


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def check_receipts(root):
    name = ('CODE_FREEZE_VERIFY.json' if (root/'CODE_FREEZE_VERIFY.json').exists()
            else 'CODE_FREEZE_REPAIR.json' if (root/'CODE_FREEZE_REPAIR.json').exists()
            else 'CODE_FREEZE.json')
    freeze = json.loads((root/name).read_text())
    if name == 'CODE_FREEZE_REPAIR.json':
        old.need(freeze['repaired_from_sha256'] == sha(root/'CODE_FREEZE.json'),
                 'original frozen failure retained')
    if name == 'CODE_FREEZE_VERIFY.json':
        old.need(freeze['repaired_from_sha256'] == sha(root/'CODE_FREEZE_REPAIR.json'),
                 'numerical repair freeze retained')
    old.need(freeze['worlds'] == list(WORLDS) and freeze['namespace'] == 'netta-cold-v1',
             'fresh-world freeze identity')
    old.need(freeze['protocol_sha256'] == sha(root/'PROTOCOL.md'), 'protocol hash')
    for name, expected in freeze['files'].items():
        old.need(sha(REPO/name) == expected, f'frozen code: {name}')
    manifests = json.loads((root/'ARTIFACT_MANIFESTS.json').read_text())
    for kind, entries in manifests.items():
        for name, expected in entries.items():
            old.need(sha(root/name) == expected, f'artifact changed: {kind}/{name}')


def independent_weights(capitals):
    # Absolute sequence capitals rather than the evaluator's P0-relative
    # log-odds. Common evidence cancels only at quote time.
    scores = [math.log2(prior) + capital for prior, capital in zip(PRIOR, capitals)]
    top = max(scores)
    scaled = [2.0 ** (score-top) for score in scores]
    total = math.fsum(scaled)
    weights = tuple(x/total for x in scaled)
    # A vanishing diagnostic weight may underflow to zero; the selected
    # mixture remains a normalized convex combination with live support.
    old.need(all(x >= 0 and math.isfinite(x) for x in weights), 'posterior support')
    old.near(math.fsum(weights), 1.0, 'posterior normalization', 1e-12)
    return weights


def two_weights(capitals):
    top = max(capitals)
    scaled = [2.0 ** (x-top) for x in capitals]
    total = math.fsum(scaled)
    return tuple(x/total for x in scaled)


def check_event(record, tag, actual, tolerance=MAX_ERROR):
    error = old.near(record[tag], actual, tag, tolerance)
    return error


def verify_life(root, world, regime, books):
    old.need((root/'data'/f'world{world:02d}'/'mosaic.bin').read_bytes()[:8192] ==
             (root/'data'/f'world{world:02d}'/'mosaic_then_unrelated.bin').read_bytes()[:8192],
             f'{world}: target common prefix')
    laws = [{p: tuple((2*x+1)/(2*sum(counts)+len(counts)) for x in counts)
             for p, counts in book.items()} for book in books]
    supports = [{p: sum(counts) for p, counts in book.items()} for book in books]
    local = {p: [0]*old.WIDTH[p] for p in old.PATTERNS}
    capital3 = {'row3': {}, 'global3': {'*': [0.0, 0.0, 0.0]}}
    capital2 = {}
    outer = {'row3': old.OuterPaths(), 'global3': old.OuterPaths(),
             'row2': old.OuterPaths()}
    max_error = max_norm = 0.0
    predictions = 0
    record_path = root/'results'/'events'/f'world{world:02d}'/f'{regime}.tsv.gz'
    c_path = root/'results'/'c'/f'world{world:02d}'/f'{regime}-row.tsv.gz'
    with gzip.open(record_path, 'rt', newline='') as output, \
         gzip.open(c_path, 'rt', newline='') as c_output:
        events = iter(csv.DictReader(output, delimiter='\t'))
        c_events = iter(csv.DictReader(c_output, delimiter='\t'))
        for t, truth, pattern, group, masses, logbase, base_norm in old.traced(root, world, regime):
            max_norm = max(max_norm, base_norm)
            c = next(c_events, None)
            old.need(c is not None and int(c['t']) == t and c['pattern'] == pattern and
                     int(c['truth']) == truth and int(c['group']) == group,
                     f'{world}/{regime}/{t}: C trace identity')
            if pattern == '-':
                cold = (1.0,)
                pair = (cold, cold)
                index = 0
            else:
                index = group
                b = local[pattern]
                cold = old.cold_groups(b, masses)
                pair = tuple(old.source_groups(laws[j][pattern], supports[j][pattern],
                                               b, masses, cold)[0] for j in range(2))
            for law in (cold, *pair):
                max_norm = max(max_norm, abs(math.fsum(law)-1.0))
                old.need(all(x > 0 for x in law), f'{world}/{regime}/{t}: group support')
            if pattern != '-':
                old.near(pair[0][-1], cold[-1], 'A NEW invariant', 1e-10)
                old.near(pair[1][-1], cold[-1], 'B NEW invariant', 1e-10)
            price = tuple(logbase+math.log2(law[index]/masses[index])
                          for law in (cold, *pair))
            for key, expected in zip(('logcold', 'logA', 'logB'), price):
                max_error = max(max_error, check_event(c, key, expected))
            # Verify the original two-book arm on this fresh batch too.
            prior2 = capital2.setdefault(pattern, [0.0, 0.0])
            w2 = two_weights(prior2)
            bank2 = w2[0]*pair[0][index] + w2[1]*pair[1][index]
            delta2 = math.log2(bank2/cold[index])
            before2 = outer['row2']
            live2 = (price[0] + math.log2(before2.cold + before2.source*2.0**delta2)
                     if before2.active else price[0])
            max_error = max(max_error,
                            check_event(c, 'logbank', price[0]+delta2),
                            check_event(c, 'loglive', live2))
            before2.observe(t, delta2)
            max_error = max(max_error, check_event(c, 'gain_after', before2.gain, 1e-7))
            for arm in ('row3', 'global3'):
                record = next(events, None)
                tag = f'{world}/{regime}/{arm}/{t}'
                old.need(record is not None and record['arm'] == arm and
                         int(record['t']) == t and record['pattern'] == pattern and
                         int(record['truth']) == truth and int(record['group']) == group,
                         tag+': event identity')
                for key, expected in zip(('logcold', 'logA', 'logB'), price):
                    max_error = max(max_error, old.near(record[key], expected, tag+'/'+key))
                key = pattern if arm == 'row3' else '*'
                capitals = capital3[arm].setdefault(key, [0.0, 0.0, 0.0])
                weights = independent_weights(capitals)
                for name, expected in zip(('weight_cold', 'weight_A', 'weight_B'), weights):
                    max_error = max(max_error, old.near(record[name], expected, tag+'/'+name))
                max_error = max(max_error,
                    old.near(record['capital_A_before'], capitals[1]-capitals[0],
                             tag+'/capital A', 1e-8),
                    old.near(record['capital_B_before'], capitals[2]-capitals[0],
                             tag+'/capital B', 1e-8))
                candidate = math.fsum(w*law[index] for w, law in zip(weights, (cold, *pair)))
                full_law = [math.fsum(w*law[g] for w, law in zip(weights, (cold, *pair)))
                            for g in range(len(cold))]
                max_norm = max(max_norm, abs(math.fsum(full_law)-1.0))
                old.need(all(v > 0 for v in full_law), tag+': candidate support')
                if pattern != '-':
                    old.near(full_law[-1], cold[-1], tag+': candidate NEW', 1e-10)
                delta = math.log2(candidate/cold[index])
                state = outer[arm]
                active = state.active
                odds = state.odds()
                shadow = state.shadow
                loglive = (price[0]+math.log2(state.cold+state.source*2.0**delta)
                           if active else price[0])
                max_error = max(max_error,
                    old.near(record['logbank'], price[0]+delta, tag+'/bank'),
                    old.near(record['loglive'], loglive, tag+'/live'),
                    old.near(record['shadow_before'], shadow, tag+'/shadow', 1e-7),
                    old.near(record['odds_before'], odds, tag+'/odds', 1e-7))
                old.need(int(record['active_before']) == int(active), tag+': active before')
                increment, activated = state.observe(t, delta)
                old.need(int(record['activated_after']) == int(activated), tag+': crossing')
                max_error = max(max_error,
                    old.near(record['gain_after'], state.gain, tag+'/gain', 1e-7),
                    old.near(loglive-price[0], increment, tag+'/increment', 1e-8))
                predictions += 1
                if pattern != '-':
                    for j in range(3):
                        capitals[j] += price[j]
            if pattern != '-':
                prior2[0] += price[1]
                prior2[1] += price[2]
                local[pattern][group] += 1
        old.need(next(events, None) is None and next(c_events, None) is None,
                 f'{world}/{regime}: surplus event')
    return {arm: {'gain': state.gain, 'tail': state.gain-state.at8192,
                  'activation': state.activation, 'minimum': state.minimum,
                  'drawdown': state.drawdown, 'horizons': state.horizons}
            for arm, state in outer.items()}, max_error, max_norm, predictions


def run(root):
    check_receipts(root)
    result = json.loads((root/'RESULT.json').read_text())
    old.need(result['worlds'] == list(WORLDS) and result['independent_gate_pending'],
             'sealed result identity')
    lookup = {(x['world'], x['regime']): x for x in result['lives']}
    old.need(len(lookup) == 24, 'life inventory')
    maximum_error = maximum_norm = 0.0
    predictions = 0
    verified = {}
    for world in WORLDS:
        books, hashes = old.read_memory(root, world)
        for regime in REGIMES:
            expected, error, norm, count = verify_life(root, world, regime, books)
            label = (world, regime)
            claimed = lookup[label]
            for arm in ('row2', 'row3', 'global3'):
                for field in ('gain', 'tail', 'activation'):
                    if arm == 'row2' and field == 'activation':
                        continue  # Compact row2 receipt contains gain/tail only.
                    actual = expected[arm][field]
                    reference = claimed[arm][field]
                    if field == 'activation':
                        old.need(actual == reference, f'{label}/{arm}: activation')
                    else:
                        maximum_error = max(maximum_error,
                            old.near(reference, actual, f'{label}/{arm}/{field}', 1e-7))
            verified[label] = expected
            maximum_error = max(maximum_error, error)
            maximum_norm = max(maximum_norm, norm)
            predictions += count
        print('verified world', world, flush=True)
    mosaic = [verified[w, 'mosaic'] for w in WORLDS]
    switched = [verified[w, 'mosaic_then_unrelated'] for w in WORLDS]
    mean = lambda values: math.fsum(values)/len(values)
    mean3 = mean([x['row3']['gain'] for x in mosaic])
    mean2 = mean([x['row2']['gain'] for x in mosaic])
    tails = [x['row3']['tail']-x['row2']['tail'] for x in switched]
    summary = {
        'mosaic_gain_bpb': mean3/N,
        'retained_fraction': mean3/mean2,
        'mosaic_positive_worlds': sum(x['row3']['gain'] > 0 for x in mosaic),
        'changed_tail_mean_improvement': mean(tails),
        'changed_tail_improved_worlds': sum(x > 0 for x in tails),
        'changed_tail_worst_improvement': min(x['row3']['tail'] for x in switched)
                                         - min(x['row2']['tail'] for x in switched),
        'changed_tail_paired_differences': tails,
    }
    for key, value in summary.items():
        given = result['summary'][key]
        if isinstance(value, list):
            for j, item in enumerate(value):
                maximum_error = max(maximum_error,
                    old.near(given[j], item, f'{key}/{j}', 1e-7))
        elif isinstance(value, int):
            old.need(value == given, key)
        else:
            maximum_error = max(maximum_error, old.near(given, value, key, 1e-7))
    tests = {
        'material_gain': summary['mosaic_gain_bpb'] >= 0.0075,
        'all_worlds_positive': summary['mosaic_positive_worlds'] == 8,
        'retention': summary['retained_fraction'] >= 0.70,
        'changed_tail_mean': summary['changed_tail_mean_improvement'] > 1.0,
        'changed_tail_count': summary['changed_tail_improved_worlds'] >= 5,
        'changed_tail_worst': summary['changed_tail_worst_improvement'] > 1.0,
    }
    document = {'verification_pass': maximum_error <= 1e-7 and maximum_norm < 1e-8,
                'gate_pass': all(tests.values()), 'tests': tests,
                'maximum_numeric_error': maximum_error,
                'maximum_normalization_error': maximum_norm,
                'predictions': predictions, 'summary': summary,
                'result_sha256': sha(root/'RESULT.json')}
    return document


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, default=HERE)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    report = run(args.root)
    with args.output.open('x') as stream:
        json.dump(report, stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write('\n')
    print(json.dumps(report, sort_keys=True, indent=2))
