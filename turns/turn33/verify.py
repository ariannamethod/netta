#!/usr/bin/env python3
"""Independent turn33 source, archive, residual, authority and gate reader.

The reader imports no turn33 writer or C implementation.  It recounts source
continuations, rebuilds all three archives byte-for-byte, and replays every
forecast with Decimal state.  HEAD256 P0/bindings, source selection and the
inherited outer law are explicitly pinned supplied boundaries.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal, localcontext
import gzip
import importlib.util
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NAMESPACE = 'netta-earned-case-residual-v1'
WORLDS = tuple(range(296, 304))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('earned', 'pooled', 'permuted', 'flat3', 'cold')
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024, EARLY, MOVE, N)
TOL = 1e-7
U0 = Decimal(7) / 8
RHO = Decimal(1) / 1024
PROTOCOL_SHA = 'aacfe4680944a967a3a2b54b7e3a2e11702a97809337f5cef6ba653008579d68'

spec = importlib.util.spec_from_file_location(
    'turn33_frozen_reader30', ROOT/'turns/turn30/verify.py')
p30 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p30
spec.loader.exec_module(p30)
p28 = p30.p28
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES
need, near, digest, compare = p28.need, p28.near, p28.digest, p28.compare
power2, mass_log2 = p28.power2, p28.mass_log2
SharedOuter, Flat = p30.SharedOuter, p30.Router3

COMMON = (
    't', 'k', 'heads', 'cold_heads', 'truth', 'rank', 'history', 'logcold',
    'matched', 'record', 'record_visits_before', 'source_pooled', 'source_a',
    'source_b', 'source_permuted_a', 'source_permuted_b', 'corrected_earned',
    'corrected_permuted', 'max_norm_error', 'new_exact',
    'admission_shadow_before', 'admitted_before', 'permission_u_before',
    'permission_u_after', 'earned_ha_before', 'earned_hb_before',
    'earned_ha_after', 'earned_hb_after', 'permuted_ha_before',
    'permuted_hb_before', 'permuted_ha_after', 'permuted_hb_after',
    'flat3_w_before', 'flat3_w_after')
OUTER_FIELDS = ('candidate', 'live', 'shadow_before', 'odds_before',
                'active_before', 'activated_after', 'gain_after')
FIELDS = COMMON + tuple(arm+'_'+field for arm in ARMS for field in OUTER_FIELDS)


def log_mix(weight, first, second):
    """Normalized log2 mixture, with exact passthrough for equal forecasts."""
    if first == second:
        return first
    scale = max(first, second)
    mass = weight*power2(first-scale) + (1-weight)*power2(second-scale)
    return scale + mass_log2(mass)


class Permission:
    """The inherited pooled permission; its update never sees the residual."""
    def __init__(self):
        self.u = U0

    def quote(self, cold, source):
        return log_mix(self.u, source, cold)

    def observe(self, source, pooled_quote, matched=True):
        if not matched:
            return
        posterior = self.u * power2(source-pooled_quote)
        self.u = (1-RHO)*posterior + RHO*U0
        need(RHO*U0 <= self.u <= 1-RHO*(1-U0),
             'pooled permission fixed-share support')


class Wealth:
    """Two log2 wealth ratios whose nonpositive portion has exactly zero voice."""
    def __init__(self):
        self.a = Decimal(0)
        self.b = Decimal(0)

    @staticmethod
    def excess(value):
        return max(power2(value)-1, Decimal(0))

    def source(self, pooled, a, b):
        if self.a <= 0 and self.b <= 0:
            return pooled
        ea, eb = self.excess(self.a), self.excess(self.b)
        scale = max(pooled, a, b)
        mass = (power2(pooled-scale) + ea*power2(a-scale) +
                eb*power2(b-scale)) / (1+ea+eb)
        return scale + mass_log2(mass)

    def support(self, pooled, a, b):
        if self.a <= 0 and self.b <= 0:
            return list(pooled)
        ea, eb = self.excess(self.a), self.excess(self.b)
        denominator = 1+ea+eb
        return [float((Decimal.from_float(p) + ea*Decimal.from_float(x) +
                       eb*Decimal.from_float(y))/denominator)
                for p, x, y in zip(pooled, a, b)]

    def observe(self, pooled, a, b, matched=True):
        if not matched:
            return
        ma = (1-RHO)*power2(self.a)*power2(a-pooled) + RHO
        mb = (1-RHO)*power2(self.b)*power2(b-pooled) + RHO
        self.a, self.b = Decimal.from_float(mass_log2(ma)), Decimal.from_float(mass_log2(mb))
        need(self.a >= -10-Decimal('1e-10') and self.b >= -10-Decimal('1e-10'),
             'wealth fixed-share floor')


def distribution(log_heads, counts):
    repeats = [math.exp2(value) for value in log_heads]
    total = math.fsum(repeats)
    k = len(repeats)
    valid = sum(counts[1:k+1]) if counts is not None else 0
    changed = ([total*(counts[r]+.5)/(valid+.5*k) for r in range(1, k+1)]
               if k and valid else repeats)
    need(all(value > 0 for value in changed) and 0 <= total < 1,
         'positive source support and residual')
    need(abs(math.fsum([1-total]+changed)-1) <= 1e-8,
         'source distribution normalization')
    return repeats, changed


def verify_books(world):
    """Recount four source lives and rebuild the full24 case archives."""
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    need(meta.get('source_split') ==
         [['sourceAB', 'sourceBA'], ['sourceCD', 'sourceDC']],
         str(folder)+' fixed source split')
    pairs, expanded = p28.compose(meta['rules'], str(folder))
    selected = meta['selected']
    capacity = (528-16-4*len(pairs))//16
    need(isinstance(selected, list) and len(selected) == 24 and len(selected) <= capacity,
         str(folder)+' full24 pooled selection')
    prefixes, owners = [], []
    for index, record in enumerate(selected):
        owner, length = record['rule_id'], record['prefix_len']
        need(type(owner) is int and 0 <= owner < len(pairs) and
             type(length) is int and 1 <= length < len(expanded[7+owner]),
             str(folder)+f' selected addressing {index}')
        prefix = expanded[7+owner][:length]
        need(record['prefix'] == list(prefix) and prefix not in prefixes,
             str(folder)+f' unique selected prefix {index}')
        p28.counter_list(record['counts'], str(folder)+f' pooled counts {index}')
        need(isinstance(record.get('book_counts'), list) and
             len(record['book_counts']) == 2,
             str(folder)+f' two case books {index}')
        prefixes.append(prefix)
        owners.append(owner)

    tapes = [p28.load_source(world, name) for name in p28.SOURCES]
    counts = p28.recount(tapes, prefixes)
    bank, pooled, permuted = [], [], []
    for index, (prefix, owner) in enumerate(zip(prefixes, owners)):
        a = [counts[0][prefix][rank]+counts[1][prefix][rank] for rank in range(7)]
        b = [counts[2][prefix][rank]+counts[3][prefix][rank] for rank in range(7)]
        total = [x+y for x, y in zip(a, b)]
        p28.counter_list(total, str(folder)+f' recounted pooled {index}')
        need(selected[index]['counts'] == total and
             selected[index]['total'] == sum(total) and
             selected[index]['book_counts'] == [a, b],
             str(folder)+f' exact source continuation counts {index}')
        rotate = lambda values: [values[0]]+values[2:]+values[1:2]
        bank.append((owner, prefix, [a, b]))
        pooled.append((owner, prefix, total))
        permuted.append((owner, prefix, [rotate(a), rotate(b)]))

    expected = {
        'bank_full.bin': p28.encode(pairs, bank, True),
        'pooled_full.bin': p28.encode(pairs, pooled),
        'permuted_full.bin': p28.encode(pairs, permuted, True)}
    for name, data in expected.items():
        cap = 528 if name == 'pooled_full.bin' else 1056
        need(len(data) <= cap and (folder/name).read_bytes() == data,
             str(folder/name)+' independently rebuilt archive bytes')
        need(meta['bytes'][name] == len(data), str(folder/name)+' declared byte size')
    need(len(expected['bank_full.bin']) - len(expected['pooled_full.bin']) == 384 and
         meta['added_case_bytes'] == 384 and meta['record_count'] == 24,
         str(folder)+' exact case-memory price')
    tables = {
        'bank': {prefix: (index, rows)
                 for index, (_, prefix, rows) in enumerate(bank)},
        'pooled': {prefix: (index, rows)
                   for index, (_, prefix, rows) in enumerate(pooled)},
        'perm': {prefix: (index, rows)
                 for index, (_, prefix, rows) in enumerate(permuted)}}
    receipt = dict(
        world=world, source_bytes=4*N, source_lives=4, rules=len(pairs),
        records=len(bank), counts_checked=len(pooled)*7,
        book_counts_checked=len(bank)*14, added_case_bytes=384,
        bytes={name: len(data) for name, data in expected.items()},
        hashes={name: digest(folder/name) for name in expected},
        source_role_histograms=[dict(sorted(Counter(tape).items())) for tape in tapes])
    return tables, receipt


def normalized(label, residual, heads):
    need(all(value > 0 for value in heads) and
         abs(math.fsum([residual]+heads)-1) <= 1e-8,
         label+' positive normalized support')


def check_life(world, regime, tables, saved):
    path = HERE/'results'/f'world{world}'/(regime+'.turn33.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw) == N, str(path)+' raw-byte horizon')
    if regime in ('partial', 'switched', 'moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:MOVE] == base[:MOVE], str(path)+' shared raw prefix')
    nrecords = len(tables['bank'])
    earned = [Wealth() for _ in range(nrecords)]
    permuted = [Wealth() for _ in range(nrecords)]
    permissions = [Permission() for _ in range(nrecords)]
    flats = [Flat() for _ in range(nrecords)]
    visits = [0]*nrecords
    states = {arm: SharedOuter() for arm in ARMS}
    history, admitted, admission_shadow = [], False, 0.0
    max_error = max_norm = 0.0
    best = worst = first_divergence = None
    residual_counts = dict(matched=0, first=0, silent=0, active=0,
                           became_active=0, fell_silent=0)
    exact = dict(new=True, equal=True, inactive=True, history=True,
                 shared_admission=True, router_state=True, silent=True, first=True)

    def check(actual, expected, label, tolerance=TOL):
        nonlocal max_error
        max_error = max(max_error, near(actual, expected, label, tolerance))

    with gzip.open(path, 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        need(tuple(rows.fieldnames or ()) == FIELDS, str(path)+' exact trace header')
        for t in range(N):
            row = next(rows, None)
            label = f'{path}:{t}'
            need(row is not None and int(row['t']) == t and int(row['truth']) == raw[t],
                 label+' complete raw chronology')
            k, heads = int(row['k']), p28.ints(row['heads'])
            rank = p28.bindings(k, heads, raw[t], label)
            need(int(row['rank']) == rank, label+' observed HEAD256 rank')
            wanted_history = ''.join(map(str, history)) if history else '-'
            need(row['history'] == wanted_history, label+' pretruth history')
            cold, head_logs = float(row['logcold']), p28.floats(row['cold_heads'])
            need(math.isfinite(cold) and cold <= 0 and len(head_logs) == k and
                 all(math.isfinite(value) and value <= 0 for value in head_logs),
                 label+' supplied P0')
            if rank:
                check(cold, head_logs[rank-1], label+' truth-head price', 1e-10)
            else:
                need(math.exp2(cold) <=
                     1-math.fsum(math.exp2(value) for value in head_logs)+1e-12,
                     label+' NEW inside residual mass')
            need(int(row['new_exact']) == 1, label+' C protected NEW declaration')
            norm = float(row['max_norm_error'])
            need(math.isfinite(norm) and 0 <= norm <= 1e-8,
                 label+' C full-vector normalization')
            max_norm = max(max_norm, norm)

            match = [p28.suffix_match(tables[name], history)
                     for name in ('bank', 'pooled', 'perm')]
            need(match[0][:2] == match[1][:2] == match[2][:2],
                 label+' identical address in three archives')
            length, record, books = match[0]
            pooled_counts, perm_books = match[1][2], match[2][2]
            need(int(row['matched']) == length and int(row['record']) == record,
                 label+' longest selected address')
            a_counts, b_counts = books if length else (None, None)
            pa_counts, pb_counts = perm_books if length else (None, None)
            prices = [p28.source_candidate(cold, head_logs, rank, counts)
                      for counts in (pooled_counts, a_counts, b_counts,
                                     pa_counts, pb_counts)]
            pool_price, a_price, b_price, pa_price, pb_price = prices
            for field, value in zip(
                    ('source_pooled', 'source_a', 'source_b',
                     'source_permuted_a', 'source_permuted_b'), prices):
                check(row[field], value, label+'/'+field)
                if not rank or not length:
                    need(float(row[field]) == cold, label+' exact source fallback '+field)

            local_support, pooled_support = distribution(head_logs, pooled_counts)
            _, a_support = distribution(head_logs, a_counts)
            _, b_support = distribution(head_logs, b_counts)
            _, pa_support = distribution(head_logs, pa_counts)
            _, pb_support = distribution(head_logs, pb_counts)
            residual_mass = 1-math.fsum(local_support)

            w = earned[record] if length else Wealth()
            pw = permuted[record] if length else Wealth()
            permission = permissions[record] if length else Permission()
            flat = flats[record] if length else Flat()
            before_visits = visits[record] if length else 0
            need(int(row['record_visits_before']) == before_visits,
                 label+' causal record visits')

            def state_fields(stage):
                check(row['permission_u_'+stage], permission.u,
                      label+'/permission/'+stage)
                for prefix, state in (('earned', w), ('permuted', pw)):
                    check(row[prefix+'_ha_'+stage], state.a,
                          label+'/'+prefix+'/ha/'+stage)
                    check(row[prefix+'_hb_'+stage], state.b,
                          label+'/'+prefix+'/hb/'+stage)
                weights = p28.floats(row['flat3_w_'+stage])
                need(len(weights) == 3 and all(value > 0 for value in weights) and
                     abs(math.fsum(weights)-1) <= 1e-10,
                     label+' flat3 weights '+stage)
                for index, (actual, expected) in enumerate(zip(weights, flat.weights)):
                    check(actual, expected, label+f'/flat3/{stage}/{index}')

            state_fields('before')
            corrected = w.source(pool_price, a_price, b_price)
            corrected_perm = pw.source(pool_price, pa_price, pb_price)
            check(row['corrected_earned'], corrected, label+'/corrected_earned')
            check(row['corrected_permuted'], corrected_perm,
                  label+'/corrected_permuted')
            candidates = dict(
                earned=permission.quote(cold, corrected),
                pooled=permission.quote(cold, pool_price),
                permuted=permission.quote(cold, corrected_perm),
                flat3=flat.quote(cold, a_price, b_price),
                cold=cold)

            corrected_support = w.support(pooled_support, a_support, b_support)
            corrected_perm_support = pw.support(
                pooled_support, pa_support, pb_support)
            u = float(permission.u)
            candidate_supports = dict(
                earned=[(1-u)*x+u*y for x, y in
                        zip(local_support, corrected_support)],
                pooled=[(1-u)*x+u*y for x, y in
                        zip(local_support, pooled_support)],
                permuted=[(1-u)*x+u*y for x, y in
                          zip(local_support, corrected_perm_support)],
                flat3=[float(flat.weights[0])*x + float(flat.weights[1])*y +
                       float(flat.weights[2])*z
                       for x, y, z in zip(local_support, a_support, b_support)],
                cold=local_support)
            for arm, support in candidate_supports.items():
                normalized(label+'/'+arm+'/candidate', residual_mass, support)
                if states[arm].active and candidates[arm] != cold:
                    source_weight = float(states[arm].source)
                    cold_weight = float(states[arm].cold)
                    live_support = [cold_weight*x+source_weight*y
                                    for x, y in zip(local_support, support)]
                else:
                    live_support = local_support
                normalized(label+'/'+arm+'/live', residual_mass, live_support)

            if length:
                residual_counts['matched'] += 1
                now = max(w.a, w.b) > 0
                if before_visits == 0:
                    residual_counts['first'] += 1
                    exact['first'] &= candidates['earned'] == candidates['pooled']
                if not now:
                    residual_counts['silent'] += 1
                    exact['silent'] &= candidates['earned'] == candidates['pooled']
                else:
                    residual_counts['active'] += 1

            need(int(row['admitted_before']) == int(admitted),
                 label+' shared admission state')
            check(row['admission_shadow_before'], admission_shadow,
                  label+'/admission_shadow_before')
            admission_shadow += candidates['pooled']-cold
            admit = not admitted and admission_shadow >= 32
            if admit:
                admitted = True

            wanted = {}
            for arm in ARMS:
                wanted[arm] = states[arm].step(t, cold, candidates[arm], admit)
                for field, value in wanted[arm].items():
                    actual = row[arm+'_'+field]
                    if type(value) is int:
                        need(int(actual) == value, label+'/'+arm+'/'+field)
                    else:
                        check(actual, value, label+'/'+arm+'/'+field)
                if not rank:
                    exact['new'] &= (float(row[arm+'_candidate']) == cold and
                                     float(row[arm+'_live']) == cold)
                if candidates[arm] == cold:
                    exact['equal'] &= (float(row[arm+'_candidate']) == cold and
                                       float(row[arm+'_live']) == cold)
                if not wanted[arm]['active_before']:
                    exact['inactive'] &= float(row[arm+'_live']) == cold
            exact['shared_admission'] &= (
                len({value['active_before'] for value in wanted.values()}) == 1 and
                len({value['activated_after'] for value in wanted.values()}) == 1)

            if length:
                w.observe(pool_price, a_price, b_price)
                pw.observe(pool_price, pa_price, pb_price)
                permission.observe(pool_price, candidates['pooled'])
                flat.observe(cold, a_price, b_price)
                visits[record] += 1
                after_now = max(w.a, w.b) > 0
                if not now and after_now:
                    residual_counts['became_active'] += 1
                if now and not after_now:
                    residual_counts['fell_silent'] += 1
            state_fields('after')

            difference = float(row['earned_live'])-float(row['pooled_live'])
            sample = dict(row, difference=difference,
                          raw_hex=raw[max(0, t-16):t+17].hex())
            if t >= MOVE:
                if best is None or difference > best['difference']:
                    best = sample
                if worst is None or difference < worst['difference']:
                    worst = sample
            if first_divergence is None and (
                    float(row['earned_candidate']) != float(row['pooled_candidate'])):
                first_divergence = sample
            history = (history+[rank])[-32:]
        need(next(rows, None) is None, str(path)+' excess trace rows')

    exact['router_state'] = True
    life = dict(
        world=world, regime=regime,
        arms={arm: states[arm].result() for arm in ARMS},
        max_norm_error=max_norm, exactness=exact, residual=residual_counts,
        raw_help=best, raw_harm=worst, first_divergence=first_divergence)
    max_error = max(max_error, compare(saved, life, f'RESULT/{world}/{regime}'))
    return life, N*len(ARMS), max_error


def mean(values):
    return math.fsum(values)/len(values)


def rebuild(lives, books):
    lookup = {(life['world'], life['regime']): life for life in lives}
    values = lambda regime, arm, key: [
        lookup[world, regime]['arms'][arm][key] for world in WORLDS]
    diffs = lambda regime, other, key: [
        a-b for a, b in zip(values(regime, 'earned', key),
                            values(regime, other, key))]
    tables = {
        regime: {
            arm: {key: mean(values(regime, arm, key))
                  for key in ('early', 'gain', 'tail')}
            for arm in ARMS}
        for regime in REGIMES}
    comparisons = {}
    for regime in REGIMES:
        comparisons[regime] = {}
        for other in ('pooled', 'permuted', 'flat3'):
            comparisons[regime][other] = {}
            for key in ('early', 'gain', 'tail'):
                delta = diffs(regime, other, key)
                comparisons[regime][other][key] = dict(
                    mean=mean(delta), wins=sum(value > 0 for value in delta),
                    per_world=delta)
    retention = dict(
        recombined_early=tables['recombined']['earned']['early']/
                         max(tables['recombined']['pooled']['early'], 1e-300),
        recombined_full=tables['recombined']['earned']['gain']/
                        max(tables['recombined']['pooled']['gain'], 1e-300))
    extras = sorted({book['added_case_bytes'] for book in books})
    need(extras == [384], 'uniform exact case price')
    margin = .01*extras[0]
    portable = {
        arm: sorted({0 if arm == 'cold' else book['bytes'][
            'pooled_full.bin' if arm == 'pooled' else 'bank_full.bin']
            for book in books})
        for arm in ARMS}
    recipient = {
        arm: sorted({book['records']*{
            'earned': 24, 'pooled': 8, 'permuted': 24,
            'flat3': 24, 'cold': 0}[arm] for book in books})
        for arm in ARMS}
    prefix_identity = all(
        lookup[world, regime]['arms'][arm]['horizons'][str(MOVE)] ==
        lookup[world, 'recombined']['arms'][arm]['horizons'][str(MOVE)]
        for world in WORLDS
        for regime in ('partial', 'switched', 'moved_mid')
        for arm in ARMS)
    admission_identity = all(
        len({state['activation'] for state in life['arms'].values()}) == 1
        for life in lives)
    partial = comparisons['partial']['pooled']
    correspondence = dict(
        recombined=comparisons['recombined']['permuted']['gain'],
        partial_tail=comparisons['partial']['permuted']['tail'])
    quantities = dict(
        comparisons=comparisons, retention=retention,
        added_case_bytes=extras, priced_margin_bits=margin,
        correspondence=correspondence, portable_bytes=portable,
        recipient_router_bytes=recipient,
        residual_activity={
            regime: {
                key: sum(lookup[world, regime]['residual'][key]
                         for world in WORLDS)
                for key in ('matched', 'first', 'silent', 'active',
                            'became_active', 'fell_silent')}
            for regime in REGIMES},
        shared_prefix_identity=prefix_identity,
        recombined_positive=sum(
            value > 0 for value in values('recombined', 'earned', 'gain')),
        unrelated_admissions=[
            dict(world=world, arm=arm,
                 activation=lookup[world, 'unrelated']['arms'][arm]['activation'],
                 gain=lookup[world, 'unrelated']['arms'][arm]['gain'])
            for world in WORLDS for arm in ARMS
            if lookup[world, 'unrelated']['arms'][arm]['activation'] is not None])
    validity = dict(
        archive=all(
            book['bytes']['pooled_full.bin'] <= 528 and
            book['bytes']['bank_full.bin'] <= 1056 and
            book['bytes']['permuted_full.bin'] == book['bytes']['bank_full.bin']
            for book in books),
        source_counts=True,
        normalization=all(life['max_norm_error'] <= 1e-8 for life in lives),
        exact_quotes=all(all(life['exactness'].values()) for life in lives),
        bounds=all(
            state['minimum'] >= -1-TOL and state['drawdown'] <= 16+TOL
            for life in lives for state in life['arms'].values()),
        identical_admissions=admission_identity, shared_prefix=prefix_identity,
        unrelated_disclosed=True)
    conditions = dict(
        E1=partial['tail']['mean'] > margin and partial['tail']['wins'] >= 5 and
           partial['gain']['mean'] >= 0,
        E2=all(tables['recombined']['pooled'][key] > 0
               for key in ('early', 'gain')) and
           min(retention.values()) >= .99 and
           quantities['recombined_positive'] == 8,
        E3=all(correspondence[key]['mean'] > margin and
               correspondence[key]['wins'] >= 5
               for key in ('recombined', 'partial_tail')),
        E4=all(validity.values()))
    material = all(conditions[key] for key in ('E1', 'E2', 'E3')) and all(validity.values())
    return dict(tables=tables, quantities=quantities, validity=validity,
                conditions=conditions, material_pass=material,
                gate_pass=material and conditions['E4'])


def check_schema(result):
    for key in ('namespace', 'worlds', 'regimes', 'arms', 'protocol_sha256',
                'lives', 'tables', 'quantities', 'conditions', 'validity',
                'material_pass', 'independent_reader_pending', 'gate_pass'):
        need(key in result, 'RESULT.json missing '+key)
    need(set(result['conditions']) == {'E1', 'E2', 'E3', 'E4'},
         'RESULT condition keys')
    need(set(result['validity']) == {
        'archive', 'source_counts', 'normalization', 'exact_quotes', 'bounds',
        'identical_admissions', 'shared_prefix', 'unrelated_disclosed'},
        'RESULT validity keys')
    need(set(result['quantities']) == {
        'comparisons', 'retention', 'added_case_bytes', 'priced_margin_bits',
        'correspondence', 'portable_bytes', 'recipient_router_bytes',
        'residual_activity', 'shared_prefix_identity', 'recombined_positive',
        'unrelated_admissions'}, 'RESULT quantity keys')


def identity(result):
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, wanted in dict(
            namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
            arms=list(ARMS)).items():
        need(result[key] == wanted and frozen[key] == wanted, 'identity '+key)
    p28.check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    need(result['protocol_sha256'] == PROTOCOL_SHA, 'RESULT protocol identity')
    need(result['independent_reader_pending'] is True and
         result['conditions']['E4'] is False and result['gate_pass'] is False,
         'writer cannot certify independent reader')
    required = (
        'turns/turn33/verify.py', 'turns/turn33/earned_router.c',
        'turns/turn33/PROTOCOL.md', 'turns/turn33/INTERFACE.md',
        'turns/turn30/verify.py', 'turns/turn28/verify.py')
    need(all(name in frozen['files'] for name in required),
         'frozen direct reader dependencies')
    for name, wanted in frozen['files'].items():
        p28.check_artifact(ROOT/name, wanted)
    counts = {}
    for name in ('DATA_MANIFEST.json', 'EXTRACT_MANIFEST.json',
                 'MEMORY_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for relative, wanted in manifest.items():
            p28.check_artifact(HERE/relative, wanted)
        counts[name] = len(manifest)
    return dict(
        reader_sha256=digest(Path(__file__)),
        freeze_sha256=digest(HERE/'FREEZE.json'),
        result_sha256=digest(HERE/'RESULT.json'),
        freeze_files=len(frozen['files']), manifest_entries=counts,
        protocol_sha256=PROTOCOL_SHA)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'new output path '+str(args.output))
    result = json.loads((HERE/'RESULT.json').read_text())
    check_schema(result)
    pins = identity(result)
    recorded = {(life['world'], life['regime']): life for life in result['lives']}
    need(len(recorded) == len(result['lives']) and
         set(recorded) == {(world, regime) for world in WORLDS for regime in REGIMES},
         'complete unique life grid')
    lives, books, count, max_error = [], [], 0, 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            tables, source = verify_books(world)
            books.append(source)
            for regime in REGIMES:
                life, forecasts, error = check_life(
                    world, regime, tables, recorded[world, regime])
                lives.append(life)
                count += forecasts
                max_error = max(max_error, error)
                print('verified', world, regime, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(ARMS),
         'complete forecast count')
    rebuilt = rebuild(lives, books)
    for key in ('tables', 'quantities', 'validity', 'material_pass'):
        max_error = max(max_error, compare(result[key], rebuilt[key], 'RESULT/'+key))
    for key in ('E1', 'E2', 'E3'):
        need(result['conditions'][key] == rebuilt['conditions'][key],
             'RESULT/conditions/'+key)
    need(rebuilt['conditions']['E4'], 'independent reconstruction validity')
    receipt = dict(
        verification_pass=True, material_pass=rebuilt['material_pass'],
        gate_pass=rebuilt['gate_pass'], forecasts_checked=count,
        max_error=max_error,
        source_counts_checked=sum(
            book['counts_checked']+book['book_counts_checked'] for book in books),
        conditions=rebuilt['conditions'], quantities=rebuilt['quantities'],
        validity=rebuilt['validity'], tables=rebuilt['tables'], pins=pins,
        source_books=books, lives=lives,
        scope='Independent source continuation recount and exact archive '
              'reconstruction; earned/permuted wealth, pooled permission, flat '
              'router, common admission, outer trajectories, metrics and gates. '
              'Inherited greedy selection and HEAD256 P0/bindings remain supplied '
              'pinned inputs; no independent frontend or generator reconstruction.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(
        {key: receipt[key] for key in
         ('verification_pass', 'material_pass', 'gate_pass',
          'forecasts_checked', 'max_error', 'source_counts_checked', 'conditions')},
        sort_keys=True))


if __name__ == '__main__':
    main()
