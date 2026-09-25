#!/usr/bin/env python3
"""Independent turn28 source-count, candidate, router and authority reader.

No imports of the writer or C implementation, no generated data, no writes
except a new output receipt. HEAD256 cold/bindings and inherited greedy
selection are supplied boundaries. New book counts are recounted from source
role observations using independent rolling integer contexts.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import math
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
NAMESPACE = 'netta-episode-alternatives-v1'
WORLDS = tuple(range(264, 272))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('local', 'global', 'pooled_full', 'pooled_small', 'permuted')
ROUTERS = ('local', 'global', 'permuted')
SOURCES = ('sourceAB', 'sourceBA', 'sourceCD', 'sourceDC')
N = 16384
TOL = 1e-7
ARCHIVE_CAP = 528
PRIOR = (Decimal(1)/8, Decimal(7)/16, Decimal(7)/16)
RHO = Decimal(1)/1024
PROTOCOL_SHA = '46608fc29be30f9087acefa4795d3495bb6e827f3c2f2760c37bc20c036220f2'
COMMON = ('t', 'k', 'heads', 'cold_heads', 'truth', 'rank', 'history',
          'logcold', 'matched', 'record', 'full_matched', 'full_record',
          'bank_a', 'bank_b', 'perm_a', 'perm_b', 'max_norm_error', 'new_exact')
FIELDS = (COMMON + tuple(a+'_'+s for a in ROUTERS for s in ('w_before', 'w_after')) +
          tuple(a+'_'+s for a in ARMS for s in
                ('candidate', 'live', 'shadow_before', 'odds_before',
                 'active_before', 'activated_after', 'gain_after')))
SOURCE_FIELDS = ('t', 'pattern', 'k', 'heads', 'truth', 'rank', 'logcold_truth')
HORIZONS = (1024, 4096, 8192, 16384)


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+' finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}; error={error}')
    return error


def compare(actual, expected, label):
    error = 0.0
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+' object')
        for key, value in expected.items():
            need(key in actual, label+' missing '+key)
            error = max(error, compare(actual[key], value, label+'/'+str(key)))
    elif isinstance(expected, (tuple, list)):
        need(isinstance(actual, (list, tuple)) and len(actual) == len(expected),
             label+' list length')
        for i, (a, b) in enumerate(zip(actual, expected)):
            error = max(error, compare(a, b, label+'/'+str(i)))
    elif isinstance(expected, float):
        error = near(actual, expected, label)
    else:
        need(type(actual) is type(expected) and actual == expected,
             f'{label}: {actual!r} != {expected!r}')
    return error


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            value.update(part)
    return value.hexdigest()


def check_artifact(path, wanted):
    got = digest(path)
    need(got == wanted, f'artifact {path}: sha256 {got} != {wanted}')


def mass_log2(value):
    need(value > 0, 'positive probability mass')
    simple = float(value)
    if 2.0**-1022 <= simple < math.inf:
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


def power2(value):
    # Keep underflowed Decimal authority alive without subnormal rounding.
    if -1022 <= value <= 1023:
        return Decimal.from_float(math.exp2(value))
    return Decimal(2) ** Decimal.from_float(value)


def ints(text):
    return [] if text in ('-', '') else list(map(int, text.split(',')))


def floats(text):
    return [] if text in ('-', '') else list(map(float, text.split(',')))


def bindings(k, heads, truth, label):
    need(0 <= k <= 6 and len(heads) == k and len(set(heads)) == k and
         all(0 <= x <= 255 for x in heads), label+' unique HEAD256 bindings')
    return heads.index(truth)+1 if truth in heads else 0


def load_source(world, name):
    stem = HERE/'data'/f'world{world}'/name
    raw = stem.with_suffix('.bin').read_bytes()
    need(len(raw) == N, str(stem)+' source byte budget')
    tape = []
    with gzip.open(stem.with_suffix('.tsv.gz'), 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        need(tuple(rows.fieldnames or ()) == SOURCE_FIELDS, str(stem)+' header')
        for t in range(N):
            row = next(rows, None)
            label = f'{stem}:{t}'
            need(row is not None and int(row['t']) == t and
                 int(row['truth']) == raw[t], label+' source chronology')
            k, heads = int(row['k']), ints(row['heads'])
            rank = bindings(k, heads, raw[t], label)
            need(int(row['rank']) == rank, label+' observed role')
            p = row['pattern']
            if p == '-':
                need(k == 0, label+' incomplete pattern')
            else:
                need(len(p) == 6 and p[0] == '0' and p.isdigit(), label+' RGS pattern')
                values = list(map(int, p))
                need(all(x <= 1+max(values[:i]) for i, x in enumerate(values) if i)
                     and max(values)+1 == k, label+' canonical pattern')
            cold = float(row['logcold_truth'])
            need(math.isfinite(cold) and cold <= 0, label+' positive cold price')
            tape.append(rank)
        need(next(rows, None) is None, str(stem)+' excess source events')
    return tape


def compose(rules, label):
    need(len(rules) <= 32, label+' maximum rules')
    expanded = [(i,) for i in range(7)]
    pairs = []
    for i, rule in enumerate(rules):
        left, right = rule['left'], rule['right']
        need(type(left) is int and type(right) is int and
             0 <= left < 7+i and 0 <= right < 7+i, label+' acyclic rule addressing')
        value = expanded[left]+expanded[right]
        need(len(value) <= 32, label+' rule length')
        expanded.append(value)
        pairs.append((left, right))
    return pairs, expanded


def recount(tapes, prefixes):
    """Count next roles from rolling 3-bit suffix codes; never join lives."""
    codes = {}
    for prefix in prefixes:
        code = 0
        for x in prefix:
            code = (code << 3) | x
        codes.setdefault(len(prefix), {})[code] = prefix
    result = [{p: [0]*7 for p in prefixes} for _ in tapes]
    for life, tape in enumerate(tapes):
        for length, wanted in codes.items():
            history, mask = 0, (1 << (3*length))-1
            for t, truth in enumerate(tape):
                if t >= length and history in wanted:
                    result[life][wanted[history]][truth] += 1
                history = ((history << 3) | truth) & mask
    return result


def counter_list(values, label):
    need(isinstance(values, list) and len(values) == 7 and
         all(type(x) is int and 0 <= x <= 65535 for x in values),
         label+' seven exact uint16 counters')
    return values


def encode(pairs, records, bank=False):
    data = struct.pack('<8sII', b'NETEB001' if bank else b'NETEI001',
                       len(pairs), len(records))
    data += b''.join(struct.pack('<HH', *p) for p in pairs)
    for record in records:
        owner, prefix, rows = record
        if bank:
            data += struct.pack('<BBH14H', owner, len(prefix), 0, *(rows[0]+rows[1]))
        else:
            data += struct.pack('<BB7H', owner, len(prefix), *rows)
    return data


def verify_books(world):
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    need(all(key in meta for key in ('rules', 'selected', 'bank_selected', 'source_split')),
         str(folder)+' BOOKS contract')
    # Split is a declaration of supplied source-life boundaries, never a target label.
    need(meta['source_split'] == [['sourceAB', 'sourceBA'], ['sourceCD', 'sourceDC']],
         str(folder)+' fixed source split')
    pairs, expanded = compose(meta['rules'], str(folder))
    fullcap = (ARCHIVE_CAP-16-4*len(pairs))//16
    bankcap = (ARCHIVE_CAP-16-4*len(pairs))//32
    selected = meta['selected']
    need(isinstance(selected, list) and len(selected) <= fullcap,
         str(folder)+' full selection capacity')
    prefixes, owners = [], []
    for i, record in enumerate(selected):
        owner, length = record['rule_id'], record['prefix_len']
        need(type(owner) is int and 0 <= owner < len(pairs) and type(length) is int
             and 1 <= length < len(expanded[7+owner]), str(folder)+' selected addressing')
        prefix = expanded[7+owner][:length]
        need(record['prefix'] == list(prefix) and prefix not in prefixes,
             str(folder)+' unique expanded selected prefix')
        counter_list(record['counts'], str(folder)+f' selected {i}')
        prefixes.append(prefix)
        owners.append(owner)
    projected = selected[:bankcap]
    need(len(meta['bank_selected']) == len(projected), str(folder)+' bank projection length')
    tapes = [load_source(world, name) for name in SOURCES]
    counts = recount(tapes, prefixes)
    full, bank, small, perm = [], [], [], []
    for i, (prefix, owner) in enumerate(zip(prefixes, owners)):
        a = [counts[0][prefix][r]+counts[1][prefix][r] for r in range(7)]
        b = [counts[2][prefix][r]+counts[3][prefix][r] for r in range(7)]
        pooled = [x+y for x, y in zip(a, b)]
        counter_list(pooled, str(folder)+f' recounted {i}')
        need(selected[i]['counts'] == pooled and selected[i]['total'] == sum(pooled),
             str(folder)+f' exact source continuation counts {i}')
        full.append((owner, prefix, pooled))
        if i < len(projected):
            declared = meta['bank_selected'][i]
            for key in ('rule_id', 'prefix_len', 'prefix', 'counts', 'total'):
                need(declared[key] == selected[i][key], str(folder)+' greedy projection '+key)
            need(declared['book_counts'] == [a, b], str(folder)+f' A/B source counts {i}')
            bank.append((owner, prefix, [a, b]))
            small.append((owner, prefix, pooled))
            rotate = lambda c: [c[0]]+c[2:]+[c[1]]
            perm.append((owner, prefix, [rotate(a), rotate(b)]))
    expected = {'bank.bin': encode(pairs, bank, True),
                'pooled_full.bin': encode(pairs, full),
                'pooled_small.bin': encode(pairs, small),
                'permuted.bin': encode(pairs, perm, True)}
    for name, data in expected.items():
        need(len(data) <= ARCHIVE_CAP and (folder/name).read_bytes() == data,
             str(folder/name)+' source-rebuilt exact archive bytes/cap')
    tables = {'bank': {p: (i, rows) for i, (_, p, rows) in enumerate(bank)},
              'full': {p: (i, rows) for i, (_, p, rows) in enumerate(full)},
              'small': {p: (i, rows) for i, (_, p, rows) in enumerate(small)},
              'perm': {p: (i, rows) for i, (_, p, rows) in enumerate(perm)}}
    receipt = dict(world=world, source_bytes=4*N, source_lives=4, rules=len(pairs),
                   full_records=len(full), bank_records=len(bank),
                   full_capacity=fullcap, bank_capacity=bankcap,
                   counts_checked=len(full)*7,
                   book_counts_checked=len(bank)*14,
                   bytes={name: len(data) for name, data in expected.items()},
                   hashes={name: digest(folder/name) for name in expected},
                   router_payload_bytes={'local': 24*len(bank), 'global': 24,
                                         'permuted': 24*len(bank)},
                   source_role_histograms=[dict(sorted(Counter(t).items())) for t in tapes])
    return tables, receipt


def suffix_match(table, history):
    for length in range(min(len(history), 31), 0, -1):
        entry = table.get(tuple(history[-length:]))
        if entry is not None:
            return length, entry[0], entry[1]
    return 0, -1, None


def source_candidate(cold, head_logs, rank, counts):
    k = len(head_logs)
    if not rank or counts is None or not k:
        return cold
    valid = sum(counts[1:k+1])
    if not valid:
        return cold
    scale = max(head_logs)
    repeat_log = scale+math.log2(math.fsum(math.exp2(p-scale) for p in head_logs))
    return repeat_log+math.log2((counts[rank]+.5)/(valid+.5*k))


class Router:
    """Three explicit probability masses; update after the current quote."""
    def __init__(self):
        self.weights = list(PRIOR)

    def quote(self, cold, a, b):
        if cold == a == b:
            return cold
        scale = max(cold, a, b)
        mass = sum(w*power2(p-scale) for w, p in zip(self.weights, (cold, a, b)))
        return scale+mass_log2(mass)

    def observe(self, cold, a, b, matched=True):
        if not matched:
            return
        scale = max(cold, a, b)
        masses = [w*power2(p-scale) for w, p in zip(self.weights, (cold, a, b))]
        total = sum(masses)
        self.weights = [(1-RHO)*(value/total)+RHO*p for value, p in zip(masses, PRIOR)]
        need(all(w >= RHO*p for w, p in zip(self.weights, PRIOR)), 'router share floor')
        near(sum(self.weights), 1, 'router probability normalization', 1e-12)

    def step(self, cold, a, b, matched=True):
        before = list(map(float, self.weights))
        price = self.quote(cold, a, b)
        self.observe(cold, a, b, matched)
        return dict(candidate=price, before=before, after=list(map(float, self.weights)))


class Outer:
    """Independent probability-space replay of prospective fixed slow HMM."""
    def __init__(self):
        self.source = self.cold = Decimal('.5')
        self.hazard = Decimal(1)/65536
        self.shadow = self.gain = self.minimum = self.peak = self.drawdown = 0.0
        self.active = False
        self.activation = None
        self.horizons = {}

    def step(self, t, cold, candidate):
        before = self.active
        shadow = self.shadow
        odds = mass_log2(self.source/self.cold) if before else 0.0
        increment = 0.0
        if before:
            joint = self.source*power2(candidate-cold)
            total = self.cold+joint
            if candidate != cold:
                increment = mass_log2(total)
            self.source = (1-self.hazard)*joint/total
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive outer probability masses')
        live = cold if candidate == cold or not before else cold+increment
        self.gain += live-cold
        self.shadow += candidate-cold
        activated = False
        if not self.active and self.shadow >= 32:
            self.active = True
            self.activation = t+1
            self.source = self.cold = Decimal('.5')
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        need(self.minimum >= -1-TOL and self.drawdown <= 16+TOL,
             'outer prefix/drawdown bounds')
        if t+1 in HORIZONS:
            self.horizons[str(t+1)] = self.gain
        return dict(candidate=candidate, live=live, shadow_before=shadow,
                    odds_before=odds, active_before=int(before),
                    activated_after=int(activated), gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, early=self.horizons['4096'],
                    tail=self.gain-self.horizons['8192'], minimum=self.minimum,
                    peak=self.peak, drawdown=self.drawdown, activation=self.activation,
                    horizons=self.horizons)


def check_life(world, regime, tables, saved):
    path = HERE/'results'/f'world{world}'/(regime+'.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw) == N, str(path)+' target byte budget')
    if regime in ('partial', 'switched', 'moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:8192] == base[:8192], str(path)+' preserved raw prefix')
    local = [Router() for _ in tables['bank']]
    permuted = [Router() for _ in tables['bank']]
    global_router = Router()
    states = {a: Outer() for a in ARMS}
    history, max_error, max_norm = [], 0.0, 0.0
    raw_help = raw_harm = None
    exactness = dict(new=True, equal=True, inactive=True, history=True)
    visits = Counter()

    def check(actual, wanted, label, tolerance=TOL):
        nonlocal max_error
        max_error = max(max_error, near(actual, wanted, label, tolerance))

    with gzip.open(path, 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        need(tuple(rows.fieldnames or ()) == FIELDS, str(path)+' exact target header')
        for t in range(N):
            row = next(rows, None)
            label = f'{path}:{t}'
            need(row is not None and int(row['t']) == t and int(row['truth']) == raw[t],
                 label+' complete causal byte chronology')
            heads, k = ints(row['heads']), int(row['k'])
            rank = bindings(k, heads, raw[t], label)
            need(int(row['rank']) == rank, label+' raw byte observed role')
            expected_history = ''.join(map(str, history)) if history else '-'
            need(row['history'] == expected_history, label+' pretruth role history')
            cold = float(row['logcold'])
            logs = floats(row['cold_heads'])
            need(math.isfinite(cold) and cold <= 0 and len(logs) == k and
                 all(math.isfinite(p) and p <= 0 for p in logs), label+' positive supplied P0')
            repeats = [math.exp2(x) for x in logs]
            mass = math.fsum(repeats)
            need(all(p > 0 for p in repeats) and 0 <= mass < 1,
                 label+' positive P0 role masses')
            if rank:
                check(cold, logs[rank-1], label+' truth bound to cold head', 1e-10)
            else:
                need(math.exp2(cold) <= 1-mass+1e-12, label+' NEW inside residual mass')
            need(int(row['new_exact']) == 1, label+' C full-vector protected NEW')
            norm = float(row['max_norm_error'])
            need(math.isfinite(norm) and 0 <= norm <= 1e-8, label+' C full-vector normalization')
            max_norm = max(max_norm, norm)
            length, record, books = suffix_match(tables['bank'], history)
            full_length, full_record, full_counts = suffix_match(tables['full'], history)
            for key, value in (('matched', length), ('record', record),
                               ('full_matched', full_length), ('full_record', full_record)):
                need(int(row[key]) == value, label+' pretruth '+key)
            if length:
                visits[record] += 1
                a, b = books
                perm_a, perm_b = tables['perm'][tuple(history[-length:])][1]
                small_counts = [x+y for x, y in zip(a, b)]
            else:
                a = b = perm_a = perm_b = small_counts = None
            component_counts = (a, b, perm_a, perm_b)
            components = [source_candidate(cold, logs, rank, counts)
                          for counts in component_counts]
            for field, value in zip(('bank_a', 'bank_b', 'perm_a', 'perm_b'), components):
                check(row[field], value, label+'/'+field)
                if not rank or not length:
                    need(float(row[field]) == cold, label+' exact unchanged '+field)
            # Independently normalize the complete changed support. Every other
            # byte stays P0, so one residual mass suffices to check all 256.
            distributions = []
            for counts in component_counts+(full_counts, small_counts):
                valid = sum(counts[1:k+1]) if counts is not None else 0
                changed = ([mass*(counts[r]+.5)/(valid+.5*k) for r in range(1, k+1)]
                           if valid and k else repeats)
                need(all(p > 0 for p in changed), label+' positive source distribution')
                independent_norm = abs(math.fsum([1-mass]+changed)-1)
                need(independent_norm <= 1e-8, label+' source normalization')
                distributions.append(changed)
            prices = {}
            routers = {'local': local[record] if length else Router(),
                       'global': global_router,
                       'permuted': permuted[record] if length else Router()}
            for arm, router in routers.items():
                pair = components[:2] if arm != 'permuted' else components[2:]
                before = floats(row[arm+'_w_before'])
                need(len(before) == 3, label+' router weight width')
                for i, (value, wanted) in enumerate(zip(before, router.weights)):
                    check(value, wanted, label+f'/{arm}/weight-before-{i}')
                price = router.quote(cold, *pair)
                prices[arm] = price
                support = distributions[:2] if arm != 'permuted' else distributions[2:4]
                full_mass = [1-mass]+[
                    math.fsum((float(router.weights[0])*repeats[r],
                               float(router.weights[1])*support[0][r],
                               float(router.weights[2])*support[1][r])) for r in range(k)]
                need(abs(math.fsum(full_mass)-1) <= 1e-8 and all(x > 0 for x in full_mass),
                     label+' full router normalization/positivity')
                if cold == pair[0] == pair[1]:
                    need(float(row[arm+'_candidate']) == cold,
                         label+' exact equal-source candidate '+arm)
            prices['pooled_full'] = source_candidate(cold, logs, rank, full_counts)
            prices['pooled_small'] = source_candidate(cold, logs, rank, small_counts)
            for arm in ARMS:
                wanted = states[arm].step(t, cold, prices[arm])
                for key, value in wanted.items():
                    if type(value) is int:
                        need(int(row[arm+'_'+key]) == value, label+'/'+arm+'/'+key)
                    else:
                        check(row[arm+'_'+key], value, label+'/'+arm+'/'+key)
                if not rank:
                    need(float(row[arm+'_candidate']) == cold,
                         label+' exact realized NEW candidate '+arm)
                if not wanted['active_before']:
                    need(float(row[arm+'_live']) == cold, label+' cold before admission '+arm)
            # Posterior/share only after every current forecast has been formed.
            for arm, router in routers.items():
                pair = components[:2] if arm != 'permuted' else components[2:]
                router.observe(cold, *pair, matched=bool(length))
                after = floats(row[arm+'_w_after'])
                need(len(after) == 3, label+' router weight width after')
                for i, (value, wanted) in enumerate(zip(after, router.weights)):
                    check(value, wanted, label+f'/{arm}/weight-after-{i}')
            if t >= 8192:
                difference = float(row['local_live'])-float(row['pooled_full_live'])
                sample = dict(row, difference=difference,
                              raw_hex=raw[max(0, t-16):t+17].hex())
                if raw_help is None or difference > raw_help['difference']:
                    raw_help = sample
                if raw_harm is None or difference < raw_harm['difference']:
                    raw_harm = sample
            history.append(rank)
            history = history[-32:]
        need(next(rows, None) is None, str(path)+' excess recipient rows')
    life = dict(world=world, regime=regime,
                arms={a: states[a].result() for a in ARMS}, max_norm_error=max_norm,
                exactness=exactness, raw_help=raw_help, raw_harm=raw_harm)
    max_error = max(max_error, compare(saved, life, 'RESULT.json/'+str(world)+'/'+regime))
    return life, N*len(ARMS), max_error, dict(visits)


def mean(values):
    return math.fsum(values)/len(values)


def rebuild(lives):
    indexed = {(x['world'], x['regime']): x for x in lives}
    tables = {}
    for regime in REGIMES:
        tables[regime] = {a: {key: mean([indexed[w, regime]['arms'][a][key] for w in WORLDS])
                              for key in ('gain', 'early', 'tail')} for a in ARMS}
    intact = [indexed[w, 'recombined']['arms'] for w in WORLDS]
    partial = [indexed[w, 'partial']['arms'] for w in WORLDS]
    e_full = [x['local']['early']-x['pooled_full']['early'] for x in intact]
    e_global = [x['local']['early']-x['global']['early'] for x in intact]
    e_perm = [x['local']['early']-x['permuted']['early'] for x in intact]
    p_tail = [x['local']['tail']-x['pooled_full']['tail'] for x in partial]
    p_whole = [x['local']['gain']-x['pooled_full']['gain'] for x in partial]
    full_denominator = tables['recombined']['pooled_full']['gain']
    quantities = dict(
        early_gain_per_byte=tables['recombined']['local']['early']/4096,
        early_positive=sum(x['local']['early'] > 0 for x in intact),
        early_vs_full=mean(e_full), early_wins_full=sum(x > 0 for x in e_full),
        full_retention=tables['recombined']['local']['gain']/max(full_denominator, 1e-300),
        full_positive=sum(x['local']['gain'] > 0 for x in intact),
        partial_tail_vs_full=mean(p_tail), partial_tail_wins_full=sum(x > 0 for x in p_tail),
        partial_whole_vs_full=mean(p_whole),
        early_vs_global=mean(e_global), early_wins_global=sum(x > 0 for x in e_global),
        early_vs_permuted=mean(e_perm), early_wins_permuted=sum(x > 0 for x in e_perm))
    q = quantities
    conditions = dict(
        T1=q['early_gain_per_byte'] >= .005 and q['early_positive'] >= 6,
        T2=q['early_vs_full'] > 1 and q['early_wins_full'] >= 5,
        T3=full_denominator > 0 and q['full_retention'] >= .95 and q['full_positive'] == 8,
        T4=q['partial_tail_vs_full'] > 1 and q['partial_tail_wins_full'] >= 5 and
           q['partial_whole_vs_full'] >= 0,
        T5=q['early_vs_global'] > 1 and q['early_wins_global'] >= 5 and
           q['early_vs_permuted'] > 1 and q['early_wins_permuted'] >= 5)
    validity = {key: True for key in ('archive', 'normalization', 'exact_quotes',
                                    'bounds', 'source_counts')}
    return dict(tables=tables, quantities=quantities, conditions=conditions,
                material_pass=all(conditions.values()), validity=validity)


def check_schema(result):
    for key in ('namespace', 'worlds', 'regimes', 'arms', 'protocol_sha256', 'lives',
                'tables', 'conditions', 'quantities', 'material_pass', 'validity',
                'independent_reader_pending'):
        need(key in result, 'RESULT.json missing '+key)
    need(set(result['conditions']) == {'T1', 'T2', 'T3', 'T4', 'T5'},
         'RESULT.json condition keys')
    need(set(result['validity']) == {'archive', 'normalization', 'exact_quotes',
                                    'bounds', 'source_counts'}, 'RESULT.json validity keys')
    return True


def identity(result):
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, wanted in dict(namespace=NAMESPACE, worlds=list(WORLDS),
                            regimes=list(REGIMES), arms=list(ARMS)).items():
        need(result[key] == wanted, 'RESULT.json/'+key)
        need(frozen[key] == wanted, 'FREEZE.json/'+key)
    check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    need(result['protocol_sha256'] == PROTOCOL_SHA, 'RESULT.json protocol SHA')
    need(result['independent_reader_pending'] is True, 'writer cannot certify independent reader')
    for name in ('verify.py', 'PROTOCOL.md', 'INTERFACE.md'):
        need(str((HERE/name).relative_to(ROOT)) in frozen['files'], 'FREEZE missing '+name)
    for name, wanted in frozen['files'].items():
        check_artifact(ROOT/name, wanted)
    counts = {}
    for name in ('DATA_MANIFEST.json', 'EXTRACT_MANIFEST.json',
                 'MEMORY_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for relative, wanted in manifest.items():
            check_artifact(HERE/relative, wanted)
        counts[name] = len(manifest)
    return dict(reader_sha256=digest(Path(__file__)), freeze_sha256=digest(HERE/'FREEZE.json'),
                freeze_files=len(frozen['files']), manifest_entries=counts,
                protocol_sha256=PROTOCOL_SHA)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), f'output {args.output} already exists')
    result = json.loads((HERE/'RESULT.json').read_text())
    check_schema(result)
    pins = identity(result)
    recorded = {(x['world'], x['regime']): x for x in result['lives']}
    need(len(recorded) == len(result['lives']) and
         set(recorded) == {(w, r) for w in WORLDS for r in REGIMES},
         'RESULT.json complete unique life grid')
    lives, source_books, count, max_error, visits = [], [], 0, 0.0, {}
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            tables, source = verify_books(world)
            source_books.append(source)
            for regime in REGIMES:
                life, forecasts, error, used = check_life(world, regime, tables, recorded[world, regime])
                lives.append(life)
                count += forecasts
                max_error = max(max_error, error)
                visits[f'{world}/{regime}'] = used
                print('verified', world, regime, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(ARMS), 'complete forecast count')
    rebuilt = rebuild(lives)
    for name, expected in rebuilt.items():
        max_error = max(max_error, compare(result[name], expected, 'RESULT.json/'+name))
    receipt = dict(verification_pass=True, material_pass=rebuilt['material_pass'],
                   forecasts_checked=count, max_error=max_error,
                   source_counts_checked=sum(x['counts_checked']+x['book_counts_checked']
                                             for x in source_books),
                   conditions=rebuilt['conditions'], validity=rebuilt['validity'],
                   quantities=rebuilt['quantities'], result_sha256=digest(HERE/'RESULT.json'),
                   pins=pins, source_books=source_books, bank_record_visits=visits,
                   tables=rebuilt['tables'], lives=lives,
                   scope='Independent source-role continuation recount, exact four archives, '
                         'matches/candidates/routers/outer laws and metrics. Inherited greedy '
                         'selection and HEAD256 cold/bindings are supplied, hash-pinned inputs; '
                         'no independent source grammar mining or full frontend reconstruction.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, ensure_ascii=False, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({key: receipt[key] for key in
                      ('verification_pass', 'material_pass', 'forecasts_checked', 'max_error',
                       'source_counts_checked', 'conditions')}, sort_keys=True))


if __name__ == '__main__':
    main()
