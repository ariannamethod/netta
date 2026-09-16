#!/usr/bin/env python3
"""Independent BANK2 reader: source counts, capital identities, HMM paths.

No experiment module is imported. The reader does not reproduce BPE training;
it checks the public chronological HEAD256 trace contract. The separate C
replay and frontend causality checks belong to the root's incoming audit.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor
import csv
import gzip
import hashlib
import itertools
import json
import math
from pathlib import Path
import random
import struct

N = 16384
WORLDS = tuple(range(32, 40))
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
ARMS = ('row', 'global', 'pool', 'null0', 'null1', 'null2')
HAZARD = 2.0 ** -16
TOL = 1e-7

# Independent enumeration: retain restricted-growth strings from a Cartesian
# product, rather than sharing the generator's recursive routine.
PATTERNS = tuple(''.join(map(str, (0,) + tail))
                 for tail in itertools.product(range(6), repeat=5)
                 if all(tail[j] <= 1 + max((0,) + tail[:j]) for j in range(5)))
WIDTH = {p: 2 + max(map(int, p)) for p in PATTERNS}
assert len(PATTERNS) == 203 and sum(WIDTH.values()) == 877


def need(test, label):
    if not test:
        raise AssertionError(label)


def near(actual, expected, label, tolerance=1e-8):
    actual = float(actual)
    need(math.isfinite(actual) and math.isfinite(expected), label + ': finite')
    error = abs(actual - expected)
    need(error <= tolerance, f'{label}: {actual:.17g} != {expected:.17g}')
    return error


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def fair_capital(a, b):
    """log2 of the equal prior mixture of two sequence capitals."""
    return max(a, b) + math.log1p(2.0 ** -abs(a-b)) / math.log(2.0) - 1.0


def weights(a, b):
    """Probability weights from independent cumulative component prices."""
    top = max(a, b)
    a, b = 2.0 ** (a-top), 2.0 ** (b-top)
    total = a+b
    return a/total, b/total


def traced(root, world, name):
    folder = f'world{world:02d}'
    raw = (root/'data'/folder/(name+'.bin')).read_bytes()
    need(len(raw) == N, f'{world}/{name}: raw length')
    path = root/'traces'/folder/(name+'.tsv.gz')
    known = {}
    previous = None
    count = 0
    with gzip.open(path, 'rt', newline='') as stream:
        for t, row in enumerate(csv.DictReader(stream, delimiter='\t')):
            tag = f'{world}/{name}/{t}'
            need(t < N and int(row['t']) == t, tag+': order')
            truth = int(row['truth'])
            need(truth == raw[t], tag+': chronological raw truth')
            trained = int(row['trained_bytes'])
            need(trained == (t//1024)*1024, tag+': prefix rebuild')
            if trained != previous:
                known.clear()
                previous = trained
            inventory = int(row['nunits'])
            need(256 <= inventory <= 2304, tag+': inventory')
            size = int(row['context_len'])
            need(0 <= size <= 6, tag+': context size')
            units = [] if row['unit_hex'] == '-' else [bytes.fromhex(s) for s in row['unit_hex'].split(',')]
            ids = [] if row['unit_ids'] == '-' else [int(s) for s in row['unit_ids'].split(',')]
            need(len(units) == len(ids) == size, tag+': context records')
            need(all(0 < len(u) <= 256 for u in units), tag+': expansions')
            suffix = b''.join(units)
            need(len(suffix) <= min(t, 256), tag+': history window')
            need(not suffix or suffix == raw[t-len(suffix):t], tag+': actual observed suffix')
            for uid, unit in zip(ids, units):
                need(0 <= uid < inventory, tag+': unit ID')
                need(uid not in known or known[uid] == unit, tag+': stable expansion')
                known[uid] = unit
            if size == 6:
                names = list(dict.fromkeys(u[0] for u in units))
                pattern = ''.join(str(names.index(u[0])) for u in units)
                group = names.index(truth) if truth in names else len(names)
                need(pattern == row['pattern'] and pattern in WIDTH, tag+': equality pattern')
                need(names == list(map(int, row['heads'].split(','))), tag+': context heads')
                need(group == int(row['group']), tag+': outcome group')
            else:
                names, pattern, group = [], '-', -1
                need(row['pattern'] == row['heads'] == '-' and int(row['group']) == -1,
                     tag+': incomplete context')
            logs = [float(row[f'logp{c}']) for c in range(256)]
            need(all(math.isfinite(x) and x < 0 for x in logs), tag+': finite base prices')
            probs = [2.0 ** x for x in logs]
            need(all(x > 0 for x in probs), tag+': positive 256-byte support')
            norm_error = abs(math.fsum(probs)-1.0)
            need(norm_error <= 1e-8, tag+': base normalization')
            near(row['logp_base_truth'], logs[truth], tag+': indexed truth', 1e-12)
            if names:
                named = set(names)
                masses = [probs[c] for c in names] + [math.fsum(probs[c] for c in range(256) if c not in named)]
            else:
                masses = [1.0]
            need(all(m > 0 for m in masses), tag+': group support')
            count += 1
            yield t, truth, pattern, group, masses, logs[truth], norm_error
    need(count == N, f'{path}: trace length')


def decode_book(blob, tag):
    need(len(blob) == 7032 and blob[:8] == b'NETHD256', tag+': book law/length')
    need(struct.unpack_from('<II', blob, 8) == (6, 877), tag+': book header')
    cells = struct.unpack_from('<877Q', blob, 16)
    counts, cursor = {}, 0
    for p in PATTERNS:
        counts[p] = tuple(cells[cursor:cursor+WIDTH[p]])
        need(sum(counts[p]) <= 2**53, tag+': exact-double source range')
        cursor += WIDTH[p]
    return counts


def read_memory(root, world):
    directory = root/'memory'/f'world{world:02d}'
    standalone = []
    for book in ('A', 'B'):
        rebuilt = {p: [0]*WIDTH[p] for p in PATTERNS}
        for source in range(2):
            for t, truth, p, g, mass, logbase, norm in traced(root, world, f'source{book}{source}'):
                if p != '-':
                    rebuilt[p][g] += 1
        blob = (directory/f'{book}.bin').read_bytes()
        parsed = decode_book(blob, f'{world}/{book}')
        need(parsed == {p: tuple(a) for p, a in rebuilt.items()},
             f'{world}/{book}: independently recounted source observations')
        standalone.append(blob)
    bank_blob = (directory/'bank.bin').read_bytes()
    need(len(bank_blob) == 14080 and bank_blob[:8] == b'NETBANK1',
         f'{world}: bank exact law/length')
    need(struct.unpack_from('<II', bank_blob, 8) == (2, 0), f'{world}: bank header')
    embedded = [bank_blob[16:7048], bank_blob[7048:]]
    need(embedded == standalone, f'{world}: bank embedded bytes differ from collected books')
    # Actual evaluation below consumes these decoded serialized bank bytes.
    books = tuple(decode_book(b, f'{world}/bank/{j}') for j, b in enumerate(embedded))
    return books, dict(bank=sha(directory/'bank.bin'), A=sha(directory/'A.bin'), B=sha(directory/'B.bin'))


def priors(world, books):
    original = tuple({p: tuple((2*x+1)/(2*sum(a)+len(a)) for x in a)
                      for p, a in book.items()} for book in books)
    pooled_counts = {p: tuple(a+b for a, b in zip(books[0][p], books[1][p])) for p in PATTERNS}
    pooled = {p: tuple((2*x+1)/(2*sum(a)+len(a)) for x in a)
              for p, a in pooled_counts.items()}
    laws = {'row': original, 'global': original}
    donors = {}
    for index in range(3):
        arm = f'null{index}'
        message = f'netta-bank-v1|{world}|head-bank-null-{index}'.encode()
        rng = random.Random(int.from_bytes(hashlib.sha256(message).digest()[:8], 'big'))
        mapping = {}
        for width in range(2, 8):
            members = tuple(p for p in PATTERNS if WIDTH[p] == width)
            shift = rng.randrange(1, len(members)) if len(members) > 1 else 0
            mapping.update(zip(members, members[shift:]+members[:shift]))
        donors[arm] = mapping
        shifted = []
        for j, book in enumerate(books):
            result = {}
            for p, donor in mapping.items():
                old_new = original[j][p][-1]
                a = book[donor]
                denominator = 2*sum(a[:-1])+len(a)-1
                result[p] = tuple((1-old_new)*(2*x+1)/denominator for x in a[:-1]) + (old_new,)
                near(math.fsum(result[p]), 1.0, f'{world}/{arm}/{j}/{p}: null normalization', 1e-12)
                need(result[p][-1] == original[j][p][-1], f'{world}/{arm}: null NEW')
            shifted.append(result)
        laws[arm] = tuple(shifted)
    return laws, pooled, pooled_counts, donors


def cold_groups(local, masses):
    denominator = 2*sum(local)+len(local)
    return tuple((m+(2*n+1)/denominator)/2 for m, n in zip(masses, local))


def source_groups(source_law, support, local, masses, cold):
    if support < 32 or sum(local) < 1 or len(local) <= 2:
        return cold, False
    numerator = tuple(m+(n+32*a)/(sum(local)+32)
                      for m, n, a in zip(masses, local, source_law))
    # Direct conditional normalization over repeat classes, without taking
    # the writer's log complement of the intermediate NEW probability.
    repeat_total = math.fsum(numerator[:-1])
    projected = tuple((1-cold[-1])*x/repeat_total for x in numerator[:-1]) + (cold[-1],)
    near(math.fsum(projected), 1.0, 'projected source normalization', 1e-8)
    need(all(x > 0 for x in projected), 'positive projected group probabilities')
    need(projected[-1] == cold[-1], 'projected source NEW equality')
    return projected, True


class OuterPaths:
    """Independent normalized probability forward-pass of absorbing C/S paths."""
    def __init__(self):
        self.active = False
        self.activation = None
        self.shadow = self.gain = 0.0
        self.cold = self.source = 0.5
        self.minimum = self.peak = self.drawdown = 0.0
        self.at8192 = 0.0
        self.active_events = 0
        self.horizons = {}

    def odds(self):
        return math.log2(self.source)-math.log2(self.cold) if self.active else 0.0

    def observe(self, t, candidate_gain):
        before_active = self.active
        activated = False
        increment = 0.0
        if before_active:
            # Bayes update by the actual candidate/cold price ratio, then
            # move h of surviving source mass to absorbing cold.
            source_joint = self.source * (2.0 ** candidate_gain)
            normalizer = self.cold + source_joint
            increment = math.log2(normalizer)
            self.gain += increment
            self.cold = (self.cold + HAZARD*source_joint)/normalizer
            self.source = ((1-HAZARD)*source_joint)/normalizer
            need(self.source > 0 and self.cold > 0, 'HMM forward underflow')
            self.active_events += 1
        self.shadow += candidate_gain
        if not before_active and self.shadow >= 32:
            self.active = True
            self.activation = t+1
            self.cold = self.source = 0.5
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        if t == 8191:
            self.at8192 = self.gain
        if t+1 in (1024, 4096, 8192, 16384):
            self.horizons[str(t+1)] = self.gain
        need(self.gain >= -1-TOL, f'event {t}: one-bit lifetime bound')
        need(self.drawdown <= 16+TOL, f'event {t}: sixteen-bit interval bound')
        return increment, activated


def verify_world(job):
    root_string, world = job
    root = Path(root_string)
    books, hashes = read_memory(root, world)
    laws, pooled, pooled_counts, donors = priors(world, books)
    metadata = json.loads((root/'results'/f'world{world:02d}-memory.json').read_text())
    need(metadata['world'] == world and metadata['observation_law'] == 'HEAD256-v1',
         f'{world}: memory metadata identity')
    need(metadata['counts'] == [{p: list(a) for p, a in book.items()} for book in books],
         f'{world}: memory metadata counts')
    need(metadata['null_donors'] == donors, f'{world}: joint null row permutation')
    claimed = json.loads((root/'results'/f'world{world:02d}.json').read_text())
    need(claimed['world'] == world and claimed['bank_bytes'] == 14080 and
         claimed['source_bytes'] == 4*N, f'{world}: claimed resource budget')
    directory = root/'data'/f'world{world:02d}'
    need((directory/'mosaic.bin').read_bytes()[:8192] ==
         (directory/'mosaic_then_unrelated.bin').read_bytes()[:8192],
         f'{world}: changed-life common raw prefix')
    supports = tuple({p: sum(a) for p, a in book.items()} for book in books)
    need(metadata['support'] == list(supports), f'{world}: source support metadata')
    need(claimed['source_events'] == [sum(s.values()) for s in supports], f'{world}: source event census')
    need(claimed['supported_rows'] == [sum(n >= 32 for n in s.values()) for s in supports],
         f'{world}: source coverage census')
    results = {}
    maximum_error = maximum_norm_error = maximum_identity_error = 0.0
    prediction_count = 0
    for regime in REGIMES:
        local = {p: [0]*WIDTH[p] for p in PATTERNS}
        states = {arm: OuterPaths() for arm in ARMS}
        # These are accumulated component capitals, not an implementation of
        # the writer's row/global log-odds recurrence.
        capitals = {arm: ({'*': [0.0, 0.0]} if arm == 'global'
                          else {p: [0.0, 0.0] for p in PATTERNS})
                    for arm in ARMS if arm != 'pool'}
        visits = {p: 0 for p in PATTERNS}
        source_contacts = [0, 0]
        row_gains = {p: 0.0 for p in PATTERNS}
        cold_loss = base_loss = 0.0
        event_path = root/'results'/'events'/f'world{world:02d}'/(regime+'.tsv.gz')
        with gzip.open(event_path, 'rt', newline='') as stream:
            records = iter(csv.DictReader(stream, delimiter='\t'))
            for t, truth, pattern, group, masses, logbase, norm_error in traced(root, world, regime):
                maximum_norm_error = max(maximum_norm_error, norm_error)
                if pattern == '-':
                    cold = (1.0,)
                    profiles = {arm: ((1.0,), (1.0,)) for arm in ARMS if arm != 'pool'}
                    pool = (1.0,)
                    selected = (False, False)
                    actual_group = 0
                else:
                    actual_group = group
                    b = local[pattern]
                    cold = cold_groups(b, masses)
                    near(math.fsum(cold), 1.0, 'cold normalization', 1e-8)
                    need(all(p > 0 for p in cold), 'positive cold support')
                    real = tuple(source_groups(laws['row'][j][pattern], supports[j][pattern],
                                               b, masses, cold) for j in range(2))
                    selected = tuple(x[1] for x in real)
                    profiles = {'row': tuple(x[0] for x in real),
                                'global': tuple(x[0] for x in real)}
                    pool = source_groups(pooled[pattern], sum(pooled_counts[pattern]),
                                         b, masses, cold)[0]
                    for arm in ('null0', 'null1', 'null2'):
                        profiles[arm] = tuple(source_groups(laws[arm][j][pattern], supports[j][pattern],
                                                            b, masses, cold)[0] for j in range(2))
                    visits[pattern] += 1
                    for j in range(2):
                        source_contacts[j] += int(selected[j])
                c_truth = cold[actual_group]
                m_truth = masses[actual_group]
                logcold = logbase + math.log2(c_truth/m_truth)
                cold_loss -= logcold
                base_loss -= logbase
                for arm in ARMS:
                    record = next(records, None)
                    need(record is not None, f'{world}/{regime}: missing event')
                    label = f'{world}/{regime}/{arm}/{t}'
                    need(int(record['t']) == t and record['arm'] == arm, label+': event order')
                    need(record['pattern'] == pattern and int(record['truth']) == truth and
                         int(record['group']) == group, label+': event identity')
                    maximum_error = max(maximum_error,
                        near(record['logbase'], logbase, label+': logbase', 1e-10),
                        near(record['logcold'], logcold, label+': logcold', 1e-9))
                    component = profiles['row'] if arm == 'pool' else profiles[arm]
                    log_components = tuple(logbase+math.log2(s[actual_group]/m_truth) for s in component)
                    deltas = tuple(math.log2(s[actual_group]/c_truth) for s in component)
                    maximum_error = max(maximum_error,
                        near(record['logA'], log_components[0], label+': component A', 1e-9),
                        near(record['logB'], log_components[1], label+': component B', 1e-9))
                    inner_before = inner_after = 0.0
                    if arm == 'pool':
                        bank = pool
                        candidate_gain = math.log2(pool[actual_group]/c_truth)
                    elif pattern == '-':
                        bank = cold
                        candidate_gain = 0.0
                    else:
                        pair = capitals[arm]['*' if arm == 'global' else pattern]
                        inner_before = pair[1]-pair[0]
                        w_a, w_b = weights(*pair)
                        bank = tuple(w_a*a+w_b*b for a, b in zip(*component))
                        old_capital = fair_capital(*pair)
                        after = [a+d for a, d in zip(pair, deltas)]
                        new_capital = fair_capital(*after)
                        candidate_gain = new_capital-old_capital
                        instantaneous_gain = math.log2(bank[actual_group]/c_truth)
                        maximum_identity_error = max(maximum_identity_error,
                            near(instantaneous_gain, candidate_gain, label+': sequence-capital price', 1e-9))
                        pair[:] = after
                        inner_after = pair[1]-pair[0]
                    need(all(x > 0 for x in bank), label+': full-byte positivity from group factors')
                    near(math.fsum(bank), 1.0, label+': full-byte normalization', 1e-8)
                    if pattern != '-':
                        near(bank[-1], cold[-1], label+': all NEW byte equality', 1e-10)
                        if group == WIDTH[pattern]-1:
                            near(inner_after, inner_before, label+': NEW has no selection evidence', 1e-12)
                    maximum_error = max(maximum_error,
                        near(record['logbank'], logcold+candidate_gain, label+': candidate quote', 1e-8),
                        near(record['inner_odds_before'], inner_before, label+': inner state before', 1e-7),
                        near(record['inner_odds_after'], inner_after, label+': inner state after', 1e-7))
                    state = states[arm]
                    need(int(record['active_before']) == int(state.active), label+': active-before')
                    maximum_error = max(maximum_error,
                        near(record['shadow_before'], state.shadow, label+': prospective shadow', 1e-7),
                        near(record['outer_odds_before'], state.odds(), label+': HMM pretruth weights', 1e-7))
                    live_gain, activated = state.observe(t, candidate_gain)
                    if arm == 'row' and pattern != '-':
                        row_gains[pattern] += live_gain
                    need(int(record['activated_after']) == int(activated), label+': following-event admission')
                    maximum_error = max(maximum_error,
                        near(record['loglive'], logcold+live_gain, label+': forward-pass live probability', 1e-8),
                        near(record['gain_after'], state.gain, label+': cumulative live gain', 1e-7))
                    prediction_count += 1
                if pattern != '-':
                    local[pattern][group] += 1
            need(next(records, None) is None, f'{world}/{regime}: excess event records')
        components = capitals['row']
        totals = [math.fsum(pair[j] for pair in components.values()) for j in range(2)]
        row_oracle = math.fsum(max(pair) for pair in components.values())
        row_capital = math.fsum(fair_capital(*pair) for pair in components.values())
        global_capital = fair_capital(*totals)
        maximum_identity_error = max(maximum_identity_error,
            near(states['row'].shadow, row_capital, f'{world}/{regime}: row capital identity', 1e-7),
            near(states['global'].shadow, global_capital, f'{world}/{regime}: global capital identity', 1e-7))
        penalty = row_oracle-row_capital
        visited = sum(n > 0 for n in visits.values())
        need(-TOL <= penalty <= visited+TOL <= 203+TOL,
             f'{world}/{regime}: ungated fixed-per-row oracle selection bound')
        for arm in ('null0', 'null1', 'null2'):
            identity = math.fsum(fair_capital(*pair) for pair in capitals[arm].values())
            maximum_identity_error = max(maximum_identity_error,
                near(states[arm].shadow, identity, f'{world}/{regime}/{arm}: null capital identity', 1e-7))
        claim = claimed['regimes'][regime]
        for p in PATTERNS:
            maximum_error = max(maximum_error,
                near(claim['row_gains'][p], row_gains[p], f'{world}/{regime}/{p}: row live gain', 1e-7))
        maximum_error = max(maximum_error,
            near(claim['cold_loss'], cold_loss, f'{world}/{regime}: cold loss', 1e-7),
            near(claim['base_loss'], base_loss, f'{world}/{regime}: base loss', 1e-7))
        for arm, state in states.items():
            arm_claim = claim['arms'][arm]
            for key, expected in dict(gain=state.gain, at8192=state.at8192,
                                     tail_gain=state.gain-state.at8192, shadow=state.shadow,
                                     inner_score=state.shadow, odds=state.odds(), minimum=state.minimum,
                                     peak=state.peak, drawdown=state.drawdown).items():
                maximum_error = max(maximum_error,
                    near(arm_claim[key], expected, f'{world}/{regime}/{arm}: final {key}', 1e-7))
            need(arm_claim['activation'] == state.activation and
                 arm_claim['active'] == state.active and arm_claim['active_events'] == state.active_events,
                 f'{world}/{regime}/{arm}: admission summary')
            for horizon, expected in state.horizons.items():
                maximum_error = max(maximum_error,
                    near(arm_claim['horizons'][horizon], expected,
                         f'{world}/{regime}/{arm}: horizon {horizon}', 1e-7))
            if arm == 'pool':
                continue
            # The global and row selectors use the same component expert
            # scores; only the location of their posterior differs.
            row_scores = capitals['row'] if arm == 'global' else capitals[arm]
            arm_totals = [math.fsum(v[j] for v in row_scores.values()) for j in range(2)]
            arm_row_oracle = math.fsum(max(v) for v in row_scores.values())
            arm_identity = (fair_capital(*arm_totals) if arm == 'global'
                            else math.fsum(fair_capital(*v) for v in row_scores.values()))
            oracle_claim = claim['oracle'][arm]
            for j in range(2):
                maximum_error = max(maximum_error,
                    near(oracle_claim['component_scores'][j], arm_totals[j],
                         f'{world}/{regime}/{arm}: component total {j}', 1e-7))
            for key, expected in dict(best_whole=max(arm_totals), best_per_row=arm_row_oracle,
                                     mixture_identity=arm_identity,
                                     regret=(max(arm_totals) if arm == 'global' else arm_row_oracle)-arm_identity).items():
                maximum_error = max(maximum_error,
                    near(oracle_claim[key], expected, f'{world}/{regime}/{arm}: oracle {key}', 1e-7))
            need(oracle_claim['visited_rows'] == visited, f'{world}/{regime}/{arm}: visited rows')
            for p, pair in row_scores.items():
                for j in range(2):
                    maximum_error = max(maximum_error,
                        near(oracle_claim['row_component_scores'][p][j], pair[j],
                             f'{world}/{regime}/{arm}/{p}: component price {j}', 1e-7))
                expected_ell = pair[1]-pair[0] if arm != 'global' else 0.0
                maximum_error = max(maximum_error,
                    near(arm_claim['row_ell'][p], expected_ell,
                         f'{world}/{regime}/{arm}/{p}: final row weights', 1e-7))
            maximum_error = max(maximum_error,
                near(arm_claim['global_ell'], arm_totals[1]-arm_totals[0] if arm == 'global' else 0.0,
                     f'{world}/{regime}/{arm}: final global weights', 1e-7))
        arms = {arm: dict(gain=s.gain, at8192=s.at8192, tail_gain=s.gain-s.at8192,
                          minimum=s.minimum, drawdown=s.drawdown, activation=s.activation,
                          active_events=s.active_events, shadow=s.shadow)
                for arm, s in states.items()}
        results[regime] = dict(arms=arms, oracle=dict(A=totals[0], B=totals[1],
                               best_whole=max(totals), best_per_row=row_oracle,
                               row_candidate=row_capital, global_candidate=global_capital,
                               row_selection_cost=penalty, visited_rows=visited),
                               source_contacts=source_contacts)
    return dict(world=world, book_hashes=hashes, lives=results,
                source_events=[sum(sum(a) for a in book.values()) for book in books],
                supported_rows=[sum(sum(a) >= 32 for a in book.values()) for book in books],
                predictions=prediction_count, maximum_numeric_error=maximum_error,
                maximum_base_normalization_error=maximum_norm_error,
                maximum_capital_identity_error=maximum_identity_error,
                null_donors=donors)


def check_manifests(root):
    frozen = json.loads((root/'CODE_FREEZE.json').read_text())
    need(frozen['protocol_sha256'] == sha(root/'PROTOCOL.md'), 'protocol digest')
    need(frozen['worlds'] == list(WORLDS) and frozen['namespace'] == 'netta-bank-v1',
         'frozen experiment identity')
    for name, expected in frozen['files'].items():
        need(sha(root.parent/name) == expected, f'frozen code changed: {name}')
    manifest_hashes = {}
    for folder in ('data', 'memory', 'traces', 'results'):
        manifest_path = root/folder/'MANIFEST.json'
        manifest_hashes[folder] = sha(manifest_path)
        for name, expected in json.loads(manifest_path.read_text()).items():
            need(sha(root/name) == expected, f'frozen artifact changed: {name}')
    return manifest_hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=None)
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output or root/'VERIFY.json'
    need(not output.exists(), f'verification output already exists: {output}')
    manifest_hashes = check_manifests(root)
    with ProcessPoolExecutor(max_workers=args.workers) as workers:
        checked = list(workers.map(verify_world, [(str(root), w) for w in WORLDS]))
    summary = json.loads((root/'results'/'SUMMARY.json').read_text())
    need(summary['protocol_sha256'] == sha(root/'PROTOCOL.md'), 'summary protocol hash')
    need([w['world'] for w in summary['worlds']] == list(WORLDS), 'summary world set/order')
    for claimed in summary['worlds']:
        single = json.loads((root/'results'/f"world{claimed['world']:02d}.json").read_text())
        need(claimed == single, f"world {claimed['world']}: aggregate differs from individual result")
    paired = []
    for world in checked:
        arms = world['lives']['mosaic']['arms']
        gain = arms['row']['gain']
        paired.append(dict(world=world['world'], gain=gain,
                           best_null_contrast=gain-max(arms[f'null{j}']['gain'] for j in range(3)),
                           global_contrast=gain-arms['global']['gain'],
                           pool_contrast=gain-arms['pool']['gain']))
    metrics = {key+'_bpb': math.fsum(p[key] for p in paired)/(len(WORLDS)*N)
               for key in ('gain', 'best_null_contrast', 'global_contrast', 'pool_contrast')}
    tests = dict(mean_vs_cold=metrics['gain_bpb'] > .01,
                 mean_vs_best_null=metrics['best_null_contrast_bpb'] > .01,
                 all_worlds_positive=all(p['gain'] > 0 for p in paired),
                 mean_vs_global=metrics['global_contrast_bpb'] > .002,
                 mean_vs_pool=metrics['pool_contrast_bpb'] > .002)
    for key, expected in metrics.items():
        near(summary['metrics'][key], expected, 'summary metric '+key, 1e-10)
    need(len(summary['paired_mosaic']) == len(paired), 'paired world count')
    for actual, expected in zip(summary['paired_mosaic'], paired):
        need(actual['world'] == expected['world'], 'paired world identity')
        for key in ('gain', 'best_null_contrast', 'global_contrast', 'pool_contrast'):
            near(actual[key], expected[key], f"paired {expected['world']} {key}", 1e-7)
    need(summary['tests'] == tests and summary['material_pass'] == all(tests.values()),
         'declared numeric gate differs from independent recomputation')
    report = dict(verification_pass=True, material_pass=all(tests.values()),
                  protocol_sha256=sha(root/'PROTOCOL.md'), checker_sha256=sha(Path(__file__)),
                  checked_artifact_manifest_hashes=manifest_hashes,
                  worlds=list(WORLDS), arm_lives=len(WORLDS)*len(REGIMES)*len(ARMS),
                  predictions=sum(w['predictions'] for w in checked),
                  source_lives=4*len(WORLDS), bank_bytes=14080,
                  maximum_numeric_error=max(w['maximum_numeric_error'] for w in checked),
                  maximum_base_normalization_error=max(w['maximum_base_normalization_error'] for w in checked),
                  maximum_capital_identity_error=max(w['maximum_capital_identity_error'] for w in checked),
                  metrics=metrics, tests=tests, paired_mosaic=paired, checked=checked,
                  scope='Trace contract, independent source recount and serialized bank, all inner/outer prices and numeric gate. Frontend BPE causality and direct C replay are checked separately.')
    with output.open('x') as stream:
        json.dump(report, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({key: report[key] for key in ('verification_pass', 'material_pass',
          'arm_lives', 'predictions', 'maximum_numeric_error', 'maximum_capital_identity_error',
          'metrics', 'tests')}, indent=2))


if __name__ == '__main__':
    main()
