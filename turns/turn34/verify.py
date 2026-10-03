#!/usr/bin/env python3
"""Independent turn34 source, archive, episode-residual and gate reader.

The reader imports no turn34 writer or C implementation. It recounts source
continuations per life, rebuilds all four archives byte-for-byte, and replays
every forecast with Decimal state. Descriptive discontinuous classifications
(silent/active at exactly zero wealth, exact candidate equality) follow the
recorded C double after the independent reconstruction has agreed within the
frozen tolerance — the turn33 disclosed reader-repair convention, in this
reader's law from birth. HEAD256 P0/bindings, source selection and the
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
import struct
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NAMESPACE = 'netta-earned-selection-v1'
WORLDS = tuple(range(304, 312))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('sel4', 'earned2', 'pooled', 'permuted4', 'cold')
EPISODES = 4
N, MOVE, EARLY = 16384, 8192, 4096
HORIZONS = (1024, EARLY, MOVE, N)
TOL = 1e-7
U0 = Decimal(7) / 8
RHO = Decimal(1) / 1024
PROTOCOL_SHA = '74fac529d58eaa4a47b3a400c8e783b4e9d490cc0b340e3c64e871b967dc054b'

spec = importlib.util.spec_from_file_location(
    'turn34_frozen_reader33', ROOT/'turns/turn33/verify.py')
p33 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p33
spec.loader.exec_module(p33)
p28 = p33.p28
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES
need, near, digest, compare = p28.need, p28.near, p28.digest, p28.compare
power2, mass_log2 = p28.power2, p28.mass_log2
SharedOuter = p33.SharedOuter
Permission, Wealth = p33.Permission, p33.Wealth
distribution, normalized, log_mix = p33.distribution, p33.normalized, p33.log_mix

COMMON = (
    't', 'k', 'heads', 'cold_heads', 'truth', 'rank', 'history', 'logcold',
    'matched', 'record', 'record_visits_before', 'source_pooled', 'source_e1',
    'source_e2', 'source_e3', 'source_e4', 'source_a2', 'source_b2',
    'source_pe1', 'source_pe2', 'source_pe3', 'source_pe4', 'corrected_sel4',
    'corrected_earned2', 'corrected_permuted4', 'max_norm_error', 'new_exact',
    'admission_shadow_before', 'admitted_before', 'permission_u_before',
    'permission_u_after')
WEALTH_COLUMNS = tuple(
    f'{prefix}_h{i}_{stage}'
    for prefix in ('sel4', 'permuted4') for stage in ('before', 'after')
    for i in range(1, 5))
# The writer prints sel4 before, sel4 after, permuted4 before, permuted4 after.
WEALTH_ORDER = tuple(
    f'sel4_h{i}_before' for i in range(1, 5)) + tuple(
    f'sel4_h{i}_after' for i in range(1, 5)) + tuple(
    f'permuted4_h{i}_before' for i in range(1, 5)) + tuple(
    f'permuted4_h{i}_after' for i in range(1, 5))
EARNED2_COLUMNS = ('earned2_ha_before', 'earned2_hb_before',
                   'earned2_ha_after', 'earned2_hb_after')
OUTER_FIELDS = ('candidate', 'live', 'shadow_before', 'odds_before',
                'active_before', 'activated_after', 'gain_after')
FIELDS = COMMON + WEALTH_ORDER + EARNED2_COLUMNS + tuple(
    arm+'_'+field for arm in ARMS for field in OUTER_FIELDS)


class SelWealth:
    """Four log2 wealth ratios; a nonpositive episode has exactly zero voice."""
    def __init__(self):
        self.h = [Decimal(0)]*EPISODES

    @staticmethod
    def excess(value):
        return max(power2(value)-1, Decimal(0))

    def silent(self):
        return all(value <= 0 for value in self.h)

    def source(self, pooled, episodes):
        if self.silent():
            return pooled
        ex = [self.excess(value) for value in self.h]
        scale = max([pooled]+list(episodes))
        mass = (power2(pooled-scale) +
                sum(e*power2(price-scale) for e, price in zip(ex, episodes)))
        return scale + mass_log2(mass/(1+sum(ex)))

    def support(self, pooled, episode_supports):
        if self.silent():
            return list(pooled)
        ex = [self.excess(value) for value in self.h]
        denominator = 1+sum(ex)
        return [float((Decimal.from_float(p) +
                       sum(e*Decimal.from_float(column[i])
                           for e, column in zip(ex, episode_supports)))
                      / denominator)
                for i, p in enumerate(map(float, pooled))]

    def observe(self, pooled, episodes, matched=True):
        if not matched:
            return
        updated = []
        for value, price in zip(self.h, episodes):
            mass = (1-RHO)*power2(value)*power2(price-pooled) + RHO
            updated.append(Decimal.from_float(mass_log2(mass)))
        self.h = updated
        need(all(value >= -10-Decimal('1e-10') for value in self.h),
             'episode wealth fixed-share floor')


def rotate(values):
    return [values[0]]+values[2:]+values[1:2]


def verify_books(world):
    """Recount four source lives and rebuild the four full24 archives."""
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    need(meta.get('source_split') ==
         [['sourceAB'], ['sourceBA'], ['sourceCD'], ['sourceDC']],
         str(folder)+' fixed per-life source split')
    need(meta.get('case_split') ==
         [['sourceAB', 'sourceBA'], ['sourceCD', 'sourceDC']],
         str(folder)+' fixed pairwise case split')
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
        need(isinstance(record.get('episode_counts'), list) and
             len(record['episode_counts']) == 4,
             str(folder)+f' four episode books {index}')
        need(isinstance(record.get('book_counts'), list) and
             len(record['book_counts']) == 2,
             str(folder)+f' two case books {index}')
        prefixes.append(prefix)
        owners.append(owner)

    tapes = [p28.load_source(world, name) for name in p28.SOURCES]
    counts = p28.recount(tapes, prefixes)
    bank4, bank2, pooled, permuted4 = [], [], [], []
    for index, (prefix, owner) in enumerate(zip(prefixes, owners)):
        per_life = [[counts[life][prefix][rank] for rank in range(7)]
                    for life in range(4)]
        a = [x+y for x, y in zip(per_life[0], per_life[1])]
        b = [x+y for x, y in zip(per_life[2], per_life[3])]
        total = [x+y for x, y in zip(a, b)]
        p28.counter_list(total, str(folder)+f' recounted pooled {index}')
        need(selected[index]['counts'] == total and
             selected[index]['total'] == sum(total) and
             selected[index]['episode_counts'] == per_life and
             selected[index]['book_counts'] == [a, b],
             str(folder)+f' exact per-life continuation counts {index}')
        bank4.append((owner, prefix, per_life))
        bank2.append((owner, prefix, [a, b]))
        pooled.append((owner, prefix, total))
        permuted4.append((owner, prefix, [rotate(life) for life in per_life]))

    def encode_four(records):
        data = struct.pack('<8sII', b'NETEB004', len(pairs), len(records))
        data += b''.join(struct.pack('<HH', *pair) for pair in pairs)
        for index, (owner, prefix, books) in enumerate(records):
            data += struct.pack('<BBH28H', owner, len(prefix), 0, *sum(books, []))
        need(len(data) <= 2112, str(folder)+' NETEB004 capacity')
        return data

    expected = {
        'bank4_full.bin': encode_four(bank4),
        'permuted4_full.bin': encode_four(permuted4),
        'bank2_full.bin': p28.encode(pairs, bank2, True),
        'pooled_full.bin': p28.encode(pairs, pooled)}
    for name, data in expected.items():
        cap = {'pooled_full.bin': 528, 'bank2_full.bin': 1056}.get(name, 2112)
        need(len(data) <= cap and (folder/name).read_bytes() == data,
             str(folder/name)+' independently rebuilt archive bytes')
        need(meta['bytes'][name] == len(data), str(folder/name)+' declared byte size')
    extra4 = len(expected['bank4_full.bin']) - len(expected['pooled_full.bin'])
    extra2 = len(expected['bank2_full.bin']) - len(expected['pooled_full.bin'])
    need(meta['added_episode_bytes'] == extra4 and
         meta['added_case_bytes'] == extra2 == 384 and
         meta['record_count'] == 24,
         str(folder)+' exact episode-memory price')
    tables = {
        'bank4': {prefix: (index, rows)
                  for index, (_, prefix, rows) in enumerate(bank4)},
        'bank2': {prefix: (index, rows)
                  for index, (_, prefix, rows) in enumerate(bank2)},
        'pooled': {prefix: (index, rows)
                   for index, (_, prefix, rows) in enumerate(pooled)},
        'perm4': {prefix: (index, rows)
                  for index, (_, prefix, rows) in enumerate(permuted4)}}
    receipt = dict(
        world=world, source_bytes=4*N, source_lives=4, rules=len(pairs),
        records=len(bank4), counts_checked=len(pooled)*7,
        episode_counts_checked=len(bank4)*28,
        added_episode_bytes=extra4, added_case_bytes=extra2,
        bytes={name: len(data) for name, data in expected.items()},
        hashes={name: digest(folder/name) for name in expected},
        source_role_histograms=[dict(sorted(Counter(tape).items())) for tape in tapes])
    return tables, receipt


def excess_float(logwealth):
    return max(2.0**logwealth - 1.0, 0.0)


def check_life(world, regime, tables, saved):
    path = HERE/'results'/f'world{world}'/(regime+'.turn34.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw) == N, str(path)+' raw-byte horizon')
    if regime in ('partial', 'switched', 'moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:MOVE] == base[:MOVE], str(path)+' shared raw prefix')
    nrecords = len(tables['bank4'])
    sel = [SelWealth() for _ in range(nrecords)]
    perm4 = [SelWealth() for _ in range(nrecords)]
    earned2 = [Wealth() for _ in range(nrecords)]
    permissions = [Permission() for _ in range(nrecords)]
    visits = [0]*nrecords
    states = {arm: SharedOuter() for arm in ARMS}
    history, admitted, admission_shadow = [], False, 0.0
    max_error = max_norm = 0.0
    best = worst = best2 = worst2 = None
    first_divergence = first_divergence2 = None
    residual_counts = dict(matched=0, first=0, silent=0, active=0,
                           became_active=0, fell_silent=0)
    episode_active = [0, 0, 0, 0]
    divergent = concentrated = 0
    exact = dict(new=True, equal=True, inactive=True, history=True,
                 shared_admission=True, router_state=True, silent=True, first=True,
                 earned2_silent=True, permuted4_silent=True)

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
                     for name in ('bank4', 'bank2', 'pooled', 'perm4')]
            need(match[0][:2] == match[1][:2] == match[2][:2] == match[3][:2],
                 label+' identical address in four archives')
            length, record, episode_books = match[0]
            case_books, pooled_counts, perm_books = (
                match[1][2], match[2][2], match[3][2])
            need(int(row['matched']) == length and int(row['record']) == record,
                 label+' longest selected address')
            e_counts = episode_books if length else [None]*4
            a2_counts, b2_counts = case_books if length else (None, None)
            pe_counts = perm_books if length else [None]*4
            all_counts = ([pooled_counts] + list(e_counts) +
                          [a2_counts, b2_counts] + list(pe_counts))
            prices = [p28.source_candidate(cold, head_logs, rank, counts)
                      for counts in all_counts]
            price_fields = ('source_pooled', 'source_e1', 'source_e2', 'source_e3',
                            'source_e4', 'source_a2', 'source_b2', 'source_pe1',
                            'source_pe2', 'source_pe3', 'source_pe4')
            for field, value in zip(price_fields, prices):
                check(row[field], value, label+'/'+field)
                if not rank or not length:
                    need(float(row[field]) == cold, label+' exact source fallback '+field)
            pool_price = prices[0]
            e_prices, a2_price, b2_price = prices[1:5], prices[5], prices[6]
            pe_prices = prices[7:11]

            local_support, pooled_support = distribution(head_logs, pooled_counts)
            e_supports = [distribution(head_logs, counts)[1] for counts in e_counts]
            _, a2_support = distribution(head_logs, a2_counts)
            _, b2_support = distribution(head_logs, b2_counts)
            pe_supports = [distribution(head_logs, counts)[1] for counts in pe_counts]
            residual_mass = 1-math.fsum(local_support)

            s = sel[record] if length else SelWealth()
            n4 = perm4[record] if length else SelWealth()
            w2 = earned2[record] if length else Wealth()
            permission = permissions[record] if length else Permission()
            before_visits = visits[record] if length else 0
            need(int(row['record_visits_before']) == before_visits,
                 label+' causal record visits')

            def state_fields(stage):
                check(row['permission_u_'+stage], permission.u,
                      label+'/permission/'+stage)
                for i in range(EPISODES):
                    check(row[f'sel4_h{i+1}_{stage}'], s.h[i],
                          label+f'/sel4/h{i+1}/'+stage)
                    check(row[f'permuted4_h{i+1}_{stage}'], n4.h[i],
                          label+f'/permuted4/h{i+1}/'+stage)
                check(row['earned2_ha_'+stage], w2.a, label+'/earned2/ha/'+stage)
                check(row['earned2_hb_'+stage], w2.b, label+'/earned2/hb/'+stage)

            state_fields('before')
            corrected = s.source(pool_price, e_prices)
            corrected2 = w2.source(pool_price, a2_price, b2_price)
            corrected_perm = n4.source(pool_price, pe_prices)
            check(row['corrected_sel4'], corrected, label+'/corrected_sel4')
            check(row['corrected_earned2'], corrected2, label+'/corrected_earned2')
            check(row['corrected_permuted4'], corrected_perm,
                  label+'/corrected_permuted4')
            candidates = dict(
                sel4=permission.quote(cold, corrected),
                earned2=permission.quote(cold, corrected2),
                pooled=permission.quote(cold, pool_price),
                permuted4=permission.quote(cold, corrected_perm),
                cold=cold)

            sel_support = s.support(pooled_support, e_supports)
            earned2_support = w2.support(pooled_support, a2_support, b2_support)
            perm_support = n4.support(pooled_support, pe_supports)
            u = float(permission.u)
            candidate_supports = dict(
                sel4=[(1-u)*x+u*y for x, y in zip(local_support, sel_support)],
                earned2=[(1-u)*x+u*y for x, y in
                         zip(local_support, earned2_support)],
                pooled=[(1-u)*x+u*y for x, y in
                        zip(local_support, pooled_support)],
                permuted4=[(1-u)*x+u*y for x, y in
                           zip(local_support, perm_support)],
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

            # Discontinuous labels follow the recorded doubles, every one of
            # which was already reconstructed within TOL by state_fields.
            h_before = [float(row[f'sel4_h{i}_before']) for i in range(1, 5)]
            sel_c = float(row['sel4_candidate'])
            pool_c = float(row['pooled_candidate'])
            if length:
                residual_counts['matched'] += 1
                now = max(h_before) > 0
                if before_visits == 0:
                    residual_counts['first'] += 1
                    exact['first'] &= sel_c == pool_c
                if not now:
                    residual_counts['silent'] += 1
                    exact['silent'] &= sel_c == pool_c
                else:
                    residual_counts['active'] += 1
                for i in range(4):
                    episode_active[i] += h_before[i] > 0
                if max(float(row[f'permuted4_h{i}_before'])
                       for i in range(1, 5)) <= 0:
                    exact['permuted4_silent'] &= (
                        float(row['permuted4_candidate']) == pool_c)
                if max(float(row['earned2_ha_before']),
                       float(row['earned2_hb_before'])) <= 0:
                    exact['earned2_silent'] &= (
                        float(row['earned2_candidate']) == pool_c)
                if sel_c != pool_c:
                    divergent += 1
                    shares = [excess_float(h) for h in h_before]
                    total = sum(shares)
                    if total > 0 and max(shares)/total >= 0.9:
                        concentrated += 1

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
                if float(row[arm+'_candidate']) == cold:
                    exact['equal'] &= float(row[arm+'_live']) == cold
                if not wanted[arm]['active_before']:
                    exact['inactive'] &= float(row[arm+'_live']) == cold
            exact['shared_admission'] &= (
                len({value['active_before'] for value in wanted.values()}) == 1 and
                len({value['activated_after'] for value in wanted.values()}) == 1)

            if length:
                s.observe(pool_price, e_prices)
                n4.observe(pool_price, pe_prices)
                w2.observe(pool_price, a2_price, b2_price)
                permission.observe(pool_price, candidates['pooled'])
                visits[record] += 1
            state_fields('after')
            if length:
                after_now = max(float(row[f'sel4_h{i}_after'])
                                for i in range(1, 5)) > 0
                now_rec = max(h_before) > 0
                if not now_rec and after_now:
                    residual_counts['became_active'] += 1
                if now_rec and not after_now:
                    residual_counts['fell_silent'] += 1

            difference = float(row['sel4_live'])-float(row['pooled_live'])
            difference2 = float(row['sel4_live'])-float(row['earned2_live'])
            sample = dict(row, difference=difference, difference2=difference2,
                          raw_hex=raw[max(0, t-16):t+17].hex())
            if t >= MOVE:
                if best is None or difference > best['difference']:
                    best = sample
                if worst is None or difference < worst['difference']:
                    worst = sample
                if best2 is None or difference2 > best2['difference2']:
                    best2 = sample
                if worst2 is None or difference2 < worst2['difference2']:
                    worst2 = sample
            if first_divergence is None and sel_c != pool_c:
                first_divergence = sample
            if first_divergence2 is None and (
                    sel_c != float(row['earned2_candidate'])):
                first_divergence2 = sample
            history = (history+[rank])[-32:]
        need(next(rows, None) is None, str(path)+' excess trace rows')

    exact['router_state'] = True
    life = dict(
        world=world, regime=regime,
        arms={arm: states[arm].result() for arm in ARMS},
        max_norm_error=max_norm, exactness=exact, residual=residual_counts,
        episode_active=episode_active,
        selection=dict(divergent=divergent, concentrated=concentrated),
        raw_help=best, raw_harm=worst, raw_help_vs_earned2=best2,
        raw_harm_vs_earned2=worst2, first_divergence=first_divergence,
        first_divergence_vs_earned2=first_divergence2)
    max_error = max(max_error, compare(saved, life, f'RESULT/{world}/{regime}'))
    return life, N*len(ARMS), max_error


def mean(values):
    return math.fsum(values)/len(values)


def rebuild(lives, books):
    lookup = {(life['world'], life['regime']): life for life in lives}
    values = lambda regime, arm, key: [
        lookup[world, regime]['arms'][arm][key] for world in WORLDS]
    diffs = lambda regime, other, key: [
        a-b for a, b in zip(values(regime, 'sel4', key),
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
        for other in ('pooled', 'permuted4', 'earned2'):
            comparisons[regime][other] = {}
            for key in ('early', 'gain', 'tail'):
                delta = diffs(regime, other, key)
                comparisons[regime][other][key] = dict(
                    mean=mean(delta), wins=sum(value > 0 for value in delta),
                    per_world=delta)
    retention = dict(
        recombined_early=tables['recombined']['sel4']['early']/
                         max(tables['recombined']['pooled']['early'], 1e-300),
        recombined_full=tables['recombined']['sel4']['gain']/
                        max(tables['recombined']['pooled']['gain'], 1e-300))
    extras4 = sorted({book['added_episode_bytes'] for book in books})
    extras2 = sorted({book['added_case_bytes'] for book in books})
    need(len(extras4) == 1 and extras2 == [384], 'uniform exact episode price')
    m4 = .01*extras4[0]
    m42 = .01*(extras4[0]-extras2[0])
    portable = {
        arm: sorted({0 if arm == 'cold' else book['bytes'][{
            'sel4': 'bank4_full.bin', 'earned2': 'bank2_full.bin',
            'pooled': 'pooled_full.bin', 'permuted4': 'permuted4_full.bin'}[arm]]
            for book in books})
        for arm in ARMS}
    recipient = {
        arm: sorted({book['records']*{
            'sel4': 40, 'earned2': 24, 'pooled': 8,
            'permuted4': 40, 'cold': 0}[arm] for book in books})
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
        recombined=comparisons['recombined']['permuted4']['gain'],
        partial_tail=comparisons['partial']['permuted4']['tail'])
    composition = comparisons['switched']['earned2']['tail']
    quantities = dict(
        comparisons=comparisons, retention=retention,
        added_episode_bytes=extras4, added_case_bytes=extras2,
        priced_margin_m4_bits=m4, priced_margin_m42_bits=m42,
        correspondence=correspondence, composition_switched_tail=composition,
        portable_bytes=portable, recipient_router_bytes=recipient,
        residual_activity={
            regime: {
                key: sum(lookup[world, regime]['residual'][key]
                         for world in WORLDS)
                for key in ('matched', 'first', 'silent', 'active',
                            'became_active', 'fell_silent')}
            for regime in REGIMES},
        episode_activity={
            regime: [sum(lookup[world, regime]['episode_active'][i]
                         for world in WORLDS) for i in range(4)]
            for regime in REGIMES},
        selection_concentration={
            regime: dict(
                divergent=sum(lookup[world, regime]['selection']['divergent']
                              for world in WORLDS),
                concentrated=sum(lookup[world, regime]['selection']['concentrated']
                                 for world in WORLDS))
            for regime in REGIMES},
        shared_prefix_identity=prefix_identity,
        recombined_positive=sum(
            value > 0 for value in values('recombined', 'sel4', 'gain')),
        unrelated_admissions=[
            dict(world=world, arm=arm,
                 activation=lookup[world, 'unrelated']['arms'][arm]['activation'],
                 gain=lookup[world, 'unrelated']['arms'][arm]['gain'])
            for world in WORLDS for arm in ARMS
            if lookup[world, 'unrelated']['arms'][arm]['activation'] is not None])
    validity = dict(
        archive=all(
            book['bytes']['pooled_full.bin'] <= 528 and
            book['bytes']['bank2_full.bin'] <= 1056 and
            book['bytes']['bank4_full.bin'] <= 2112 and
            book['bytes']['permuted4_full.bin'] == book['bytes']['bank4_full.bin']
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
        G1=partial['tail']['mean'] > m4 and partial['tail']['wins'] >= 5 and
           partial['gain']['mean'] >= 0,
        G2=all(tables['recombined']['pooled'][key] > 0
               for key in ('early', 'gain')) and
           min(retention.values()) >= .99 and
           quantities['recombined_positive'] == 8,
        G3=all(correspondence[key]['mean'] > m4 and
               correspondence[key]['wins'] >= 5
               for key in ('recombined', 'partial_tail')),
        G4=composition['mean'] > m42 and composition['wins'] >= 5,
        G5=all(validity.values()))
    material = (all(conditions[key] for key in ('G1', 'G2', 'G3', 'G4')) and
                all(validity.values()))
    return dict(tables=tables, quantities=quantities, validity=validity,
                conditions=conditions, material_pass=material,
                gate_pass=material and conditions['G5'])


def check_schema(result):
    for key in ('namespace', 'worlds', 'regimes', 'arms', 'protocol_sha256',
                'lives', 'tables', 'quantities', 'conditions', 'validity',
                'material_pass', 'independent_reader_pending', 'gate_pass'):
        need(key in result, 'RESULT.json missing '+key)
    need(set(result['conditions']) == {'G1', 'G2', 'G3', 'G4', 'G5'},
         'RESULT condition keys')
    need(set(result['validity']) == {
        'archive', 'source_counts', 'normalization', 'exact_quotes', 'bounds',
        'identical_admissions', 'shared_prefix', 'unrelated_disclosed'},
        'RESULT validity keys')
    need(set(result['quantities']) == {
        'comparisons', 'retention', 'added_episode_bytes', 'added_case_bytes',
        'priced_margin_m4_bits', 'priced_margin_m42_bits', 'correspondence',
        'composition_switched_tail', 'portable_bytes', 'recipient_router_bytes',
        'residual_activity', 'episode_activity', 'selection_concentration',
        'shared_prefix_identity', 'recombined_positive', 'unrelated_admissions'},
        'RESULT quantity keys')


def identity(result):
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, wanted in dict(
            namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
            arms=list(ARMS)).items():
        need(result[key] == wanted and frozen[key] == wanted, 'identity '+key)
    p28.check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    need(result['protocol_sha256'] == PROTOCOL_SHA, 'RESULT protocol identity')
    need(result['independent_reader_pending'] is True and
         result['conditions']['G5'] is False and result['gate_pass'] is False,
         'writer cannot certify independent reader')
    required = (
        'turns/turn34/verify.py', 'turns/turn34/sel4_router.c',
        'turns/turn34/PROTOCOL.md', 'turns/turn34/INTERFACE.md',
        'turns/turn33/verify.py', 'turns/turn33/earned_router.c',
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
    for key in ('G1', 'G2', 'G3', 'G4'):
        need(result['conditions'][key] == rebuilt['conditions'][key],
             'RESULT/conditions/'+key)
    need(rebuilt['conditions']['G5'], 'independent reconstruction validity')
    receipt = dict(
        verification_pass=True, material_pass=rebuilt['material_pass'],
        gate_pass=rebuilt['gate_pass'], forecasts_checked=count,
        max_error=max_error,
        source_counts_checked=sum(
            book['counts_checked']+book['episode_counts_checked'] for book in books),
        conditions=rebuilt['conditions'], quantities=rebuilt['quantities'],
        validity=rebuilt['validity'], tables=rebuilt['tables'], pins=pins,
        source_books=books, lives=lives,
        scope='Independent per-life source continuation recount and exact '
              'four-archive reconstruction; four episode wealths, the frozen '
              'two-case wealth, pooled permission, common admission, outer '
              'trajectories, metrics and gates. Discontinuous labels follow '
              'the recorded double after reconstruction within tolerance, by '
              'the turn33 repair convention adopted at birth. Inherited '
              'greedy selection and HEAD256 P0/bindings remain supplied '
              'pinned inputs.')
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
