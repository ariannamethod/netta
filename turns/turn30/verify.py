#!/usr/bin/env python3
"""Independent turn30 source, router, trace, authority and gate reader."""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NAMESPACE = 'netta-distinct-histories-v1'
WORLDS = tuple(range(280, 288))
REGIMES = ('recombined','partial','switched','moved_mid','unrelated')
ARMS = ('bank3','pooled2','permuted2','cold','full24')
GATED = ('bank3','pooled2','permuted2','cold')
N = 16384
MOVE = 8192
EARLY = 4096
HORIZONS = (1024, EARLY, MOVE, N)
TOL = 1e-7
RHO = Decimal(1)/1024
PRIOR2 = Decimal(7)/8
PRIOR3 = (Decimal(1)/8, Decimal(7)/16, Decimal(7)/16)
PROTOCOL_SHA = 'fab5c1dc717c4ecbe76fab06d934852f999f187758b77a31a7e5f068edc2a47c'

spec = importlib.util.spec_from_file_location('turn30_reader_turn28', ROOT/'turns/turn28/verify.py')
p28 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p28
spec.loader.exec_module(p28)
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES

TURN30_COMMON = ('t','k','heads','cold_heads','truth','rank','history','logcold',
                 'matched','record','source_pooled','source_a','source_b','source_permuted',
                 'max_norm_error','new_exact','admission_shadow_before','admitted_before')
TURN30_FIELDS = (TURN30_COMMON +
    tuple(a+'_'+s for a in ('bank3','pooled2','permuted2') for s in ('w_before','w_after')) +
    tuple(a+'_'+s for a in GATED for s in
          ('candidate','live','shadow_before','odds_before','active_before',
           'activated_after','gain_after')))
FULL24_ARMS = ('local_full','global_full','pooled_full','permuted_full')
FULL24_FIELDS = (('t','k','heads','cold_heads','truth','rank','history','logcold',
                  'matched','record','source','perm_source','max_norm_error','new_exact') +
    tuple(a+'_'+s for a in ('local','global','permuted') for s in ('w_before','w_after')) +
    tuple(a+'_'+s for a in FULL24_ARMS for s in
          ('candidate','live','shadow_before','odds_before','active_before',
           'activated_after','gain_after')))


def need(value, label):
    if not value: raise AssertionError(label)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            h.update(part)
    return h.hexdigest()


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+' finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}; error={error}')
    return error


def compare(actual, expected, label):
    return p28.compare(actual, expected, label)


def rotate(counts):
    return [counts[0]]+counts[2:]+counts[1:2]


class BinaryRouter:
    """Turn29 law: one weight on the single stored continuation."""
    def __init__(self): self.memory = PRIOR2

    def quote(self, cold, source):
        if cold == source: return cold
        scale = max(cold, source)
        mass = (1-self.memory)*p28.power2(cold-scale)+self.memory*p28.power2(source-scale)
        return scale+p28.mass_log2(mass)

    def observe(self, source, quoted, matched=True):
        if not matched: return
        posterior = self.memory*p28.power2(source-quoted)
        self.memory = (1-RHO)*posterior+RHO*PRIOR2
        need(RHO*PRIOR2 <= self.memory <= 1-RHO*(1-PRIOR2), 'binary share bounds')


class Router3:
    """Three masses over P0 and the two stored histories, renormalized."""
    def __init__(self): self.weights = list(PRIOR3)

    def quote(self, cold, a, b):
        if a == cold and b == cold: return cold
        scale = max(cold, a, b)
        mass = sum(w*p28.power2(p-scale) for w, p in zip(self.weights, (cold, a, b)))
        return scale+p28.mass_log2(mass)

    def observe(self, cold, a, b, matched=True):
        if not matched: return
        scale = max(cold, a, b)
        masses = [w*p28.power2(p-scale) for w, p in zip(self.weights, (cold, a, b))]
        total = sum(masses)
        need(total > 0, 'positive three-way posterior mass')
        self.weights = [(1-RHO)*(value/total)+RHO*p for value, p in zip(masses, PRIOR3)]
        need(all(w > 0 for w in self.weights), 'three-way share floor')
        near(sum(self.weights), 1, 'three-way probability normalization', 1e-12)


class SharedOuter:
    """Inherited outer law; admission is the life's single shared event."""
    def __init__(self):
        self.source = self.cold = Decimal('.5')
        self.hazard = Decimal(1)/65536
        self.shadow = self.gain = self.minimum = self.peak = self.drawdown = 0.0
        self.active = False
        self.activation = None
        self.horizons = {}

    def step(self, t, cold, candidate, admit):
        before = self.active
        shadow = self.shadow
        odds = p28.mass_log2(self.source/self.cold) if before else 0.0
        increment = 0.0
        if before:
            joint = self.source*p28.power2(candidate-cold)
            total = self.cold+joint
            if candidate != cold:
                increment = p28.mass_log2(total)
            self.source = (1-self.hazard)*joint/total
            self.cold = 1-self.source
            need(self.source > 0 and self.cold > 0, 'positive outer probability masses')
        live = cold if candidate == cold or not before else cold+increment
        self.gain += live-cold
        self.shadow += candidate-cold
        activated = False
        if not self.active and admit:
            self.active = True
            self.activation = t+1
            self.source = self.cold = Decimal('.5')
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        need(self.minimum >= -1-TOL and self.drawdown <= 16+TOL, 'outer prefix/drawdown bounds')
        if t+1 in HORIZONS: self.horizons[str(t+1)] = self.gain
        return dict(candidate=candidate, live=live, shadow_before=shadow, odds_before=odds,
                    active_before=int(before), activated_after=int(activated),
                    gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, early=self.horizons[str(EARLY)],
                    tail=self.gain-self.horizons[str(MOVE)], minimum=self.minimum,
                    peak=self.peak, drawdown=self.drawdown, activation=self.activation,
                    horizons=self.horizons)


def verify_memory(world):
    tables, receipt = p28.verify_books(world)
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    pairs, _ = p28.compose(meta['rules'], str(folder))
    for name, source in (('permuted_full', 'full'), ('permuted_small', 'small')):
        ordered = sorted((index, prefix, counts)
                         for prefix, (index, counts) in tables[source].items())
        records = [(meta['selected'][index]['rule_id'], prefix, rotate(counts))
                   for index, prefix, counts in ordered]
        encoded = p28.encode(pairs, records)
        path = folder/(name+'.bin')
        need(len(encoded) <= 528 and path.read_bytes() == encoded,
             str(path)+' independently rebuilt pooled permutation')
        tables[name] = {prefix:(index, rotate(counts)) for index, prefix, counts in ordered}
        receipt['bytes'][name+'.bin'] = len(encoded)
        receipt['hashes'][name+'.bin'] = digest(path)
    need(receipt['bytes']['pooled_small.bin'] == receipt['bytes']['permuted_small.bin'] and
         receipt['bytes']['pooled_full.bin'] == receipt['bytes']['permuted_full.bin'],
         str(folder)+' null archives cost what they mirror')
    receipt['portable_bytes'] = dict(bank3=receipt['bytes']['bank.bin'],
        pooled2=receipt['bytes']['pooled_small.bin'],
        permuted2=receipt['bytes']['permuted_small.bin'], cold=0,
        full24=receipt['bytes']['pooled_full.bin'])
    receipt['extra_bytes'] = receipt['portable_bytes']['bank3']-receipt['portable_bytes']['pooled2']
    return tables, receipt


def distribution(cold_heads, counts):
    k = len(cold_heads)
    repeat = [math.exp2(x) for x in cold_heads]
    mass = math.fsum(repeat)
    valid = sum(counts[1:k+1]) if counts is not None else 0
    changed = ([mass*(counts[r]+.5)/(valid+.5*k) for r in range(1,k+1)]
               if valid and k else repeat)
    need(all(x > 0 for x in changed) and abs(math.fsum([1-mass]+changed)-1) <= 1e-8,
         'positive normalized source distribution')
    return repeat, changed


def check_life(world, regime, tables, saved):
    folder = HERE/'results'/f'world{world}'
    turn30_path = folder/(regime+'.turn30.tsv.gz')
    full24_path = folder/(regime+'.full24.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw) == N, str(turn30_path)+' raw budget')
    if regime in ('partial','switched','moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:MOVE] == base[:MOVE], str(turn30_path)+' shared prefix')

    bank3 = [Router3() for _ in tables['small']]
    pooled2 = [BinaryRouter() for _ in tables['small']]
    permuted2 = [BinaryRouter() for _ in tables['permuted_small']]
    full24 = [BinaryRouter() for _ in tables['full']]
    states = {arm:SharedOuter() for arm in GATED}
    states['full24'] = p28.Outer()
    history = []
    admission_shadow = 0.0
    admitted = False
    max_error = max_norm = 0.0
    exact = dict(new=True, equal=True, inactive=True, history=True, cross_trace=True,
                 shared_admission=True, router_weights=True)
    best = worst = None

    def check(actual, wanted, label, tolerance=TOL):
        nonlocal max_error
        max_error = max(max_error, near(actual, wanted, label, tolerance))

    with gzip.open(turn30_path,'rt') as fa, gzip.open(full24_path,'rt') as fb:
        rows, fulls = csv.DictReader(fa,delimiter='\t'), csv.DictReader(fb,delimiter='\t')
        need(tuple(rows.fieldnames or ()) == TURN30_FIELDS, str(turn30_path)+' exact header')
        need(tuple(fulls.fieldnames or ()) == FULL24_FIELDS,
             str(full24_path)+' frozen turn29 header')
        for t in range(N):
            row, full = next(rows,None), next(fulls,None)
            label = f'{world}/{regime}/{t}'
            need(row is not None and full is not None, label+' complete traces')
            for key in ('t','k','heads','cold_heads','truth','rank','history','logcold'):
                need(row[key] == full[key], label+' one price tape '+key)
            need(int(row['t']) == t and int(row['truth']) == raw[t], label+' causal chronology')
            heads, k = p28.ints(row['heads']), int(row['k'])
            rank = p28.bindings(k, heads, raw[t], label)
            need(int(row['rank']) == rank, label+' rank')
            need(row['history'] == (''.join(map(str,history)) if history else '-'),
                 label+' pretruth history')
            need(int(row['new_exact']) == 1 and int(full['new_exact']) == 1,
                 label+' C protected NEW')
            cold = float(row['logcold']); logs = p28.floats(row['cold_heads'])
            need(math.isfinite(cold) and cold <= 0 and len(logs) == k, label+' P0')
            if rank: check(cold, logs[rank-1], label+' bound truth', 1e-10)

            slen, srecord, pooled_counts = p28.suffix_match(tables['small'], history)
            blen, brecord, books = p28.suffix_match(tables['bank'], history)
            plen, precord, perm_counts = p28.suffix_match(tables['permuted_small'], history)
            flen, frecord, full_counts = p28.suffix_match(tables['full'], history)
            need((slen,srecord) == (blen,brecord) == (plen,precord),
                 label+' one address set across the three archives')
            need(int(row['matched']) == slen and int(row['record']) == srecord,
                 label+' turn30 match')
            need(int(full['matched']) == flen and int(full['record']) == frecord,
                 label+' full24 match')
            a_counts, b_counts = books if blen else (None, None)
            pooled_price = p28.source_candidate(cold, logs, rank, pooled_counts)
            a = p28.source_candidate(cold, logs, rank, a_counts)
            b = p28.source_candidate(cold, logs, rank, b_counts)
            perm_price = p28.source_candidate(cold, logs, rank, perm_counts)
            full_price = p28.source_candidate(cold, logs, rank, full_counts)
            check(row['source_pooled'], pooled_price, label+'/source-pooled')
            check(row['source_a'], a, label+'/source-a')
            check(row['source_b'], b, label+'/source-b')
            check(row['source_permuted'], perm_price, label+'/source-permuted')
            check(full['source'], full_price, label+'/source-full24')
            repeats, pooled_dist = distribution(logs, pooled_counts)
            _, a_dist = distribution(logs, a_counts)
            _, b_dist = distribution(logs, b_counts)
            _, perm_dist = distribution(logs, perm_counts)
            _, full_dist = distribution(logs, full_counts)

            three = bank3[srecord] if slen else Router3()
            two_pooled = pooled2[srecord] if slen else BinaryRouter()
            two_perm = permuted2[precord] if plen else BinaryRouter()
            two_full = full24[frecord] if flen else BinaryRouter()
            before3 = p28.floats(row['bank3_w_before'])
            need(len(before3) == 3, label+' three-way weight width')
            for i, value in enumerate(three.weights):
                check(before3[i], value, label+f'/bank3 before {i}')
            check(row['pooled2_w_before'], two_pooled.memory, label+'/pooled2 before')
            check(row['permuted2_w_before'], two_perm.memory, label+'/permuted2 before')
            check(full['local_w_before'], two_full.memory, label+'/full24 before')
            exact['router_weights'] &= (all(x > 0 for x in before3) and
                abs(math.fsum(before3)-1) <= 1e-10 and
                0 < float(row['pooled2_w_before']) < 1 and
                0 < float(row['permuted2_w_before']) < 1)

            candidates = {'bank3':three.quote(cold,a,b),
                          'pooled2':two_pooled.quote(cold,pooled_price),
                          'permuted2':two_perm.quote(cold,perm_price),
                          'cold':cold,
                          'full24':two_full.quote(cold,full_price)}
            for arm in GATED:
                check(row[arm+'_candidate'], candidates[arm], label+'/'+arm+' candidate')
            check(full['local_full_candidate'], candidates['full24'], label+'/full24 candidate')

            # Complete changed support, normalized independently of the truth.
            repeat_mass = math.fsum(repeats)
            need(0 <= repeat_mass < 1, label+' repeat mass')
            mixed = [1-repeat_mass]+[float(three.weights[0])*repeats[i]+
                                     float(three.weights[1])*a_dist[i]+
                                     float(three.weights[2])*b_dist[i] for i in range(k)]
            need(all(x > 0 for x in mixed) and abs(math.fsum(mixed)-1) <= 1e-8,
                 label+' three-way normalization')
            for weight, dist in ((two_pooled.memory,pooled_dist),(two_perm.memory,perm_dist),
                                 (two_full.memory,full_dist)):
                binary = [1-repeat_mass]+[(1-float(weight))*repeats[i]+float(weight)*dist[i]
                                          for i in range(k)]
                need(all(x > 0 for x in binary) and abs(math.fsum(binary)-1) <= 1e-8,
                     label+' binary normalization')

            shadow_before = admission_shadow
            need(int(row['admitted_before']) == int(admitted), label+' shared admission state')
            check(row['admission_shadow_before'], shadow_before, label+'/admission shadow')
            admission_shadow += candidates['pooled2']-cold
            admit = not admitted and admission_shadow >= 32.0
            if admit: admitted = True

            wanted = {}
            for arm in ARMS:
                if arm == 'full24':
                    wanted[arm] = states[arm].step(t, cold, candidates[arm])
                    source, field = full, 'local_full'
                else:
                    wanted[arm] = states[arm].step(t, cold, candidates[arm], admit)
                    source, field = row, arm
                for key, value in wanted[arm].items():
                    if type(value) is int:
                        need(int(source[field+'_'+key]) == value, label+'/'+arm+'/'+key)
                    else:
                        check(source[field+'_'+key], value, label+'/'+arm+'/'+key)
                if not rank:
                    need(float(source[field+'_candidate']) == cold and
                         float(source[field+'_live']) == cold, label+' protected NEW '+arm)
                if candidates[arm] == cold:
                    need(float(source[field+'_live']) == cold, label+' exact equal '+arm)
                if not wanted[arm]['active_before']:
                    need(float(source[field+'_live']) == cold, label+' cold before admission '+arm)
            exact['shared_admission'] &= (len({wanted[arm]['active_before'] for arm in GATED}) == 1
                and len({wanted[arm]['activated_after'] for arm in GATED}) == 1)

            if slen:
                bank3[srecord].observe(cold,a,b)
                pooled2[srecord].observe(pooled_price,candidates['pooled2'])
                permuted2[precord].observe(perm_price,candidates['permuted2'])
            if flen: full24[frecord].observe(full_price,candidates['full24'])
            after3 = p28.floats(row['bank3_w_after'])
            wanted3 = bank3[srecord].weights if slen else list(PRIOR3)
            for i, value in enumerate(wanted3):
                check(after3[i], value, label+f'/bank3 after {i}')
            check(row['pooled2_w_after'], pooled2[srecord].memory if slen else PRIOR2,
                  label+'/pooled2 after')
            check(row['permuted2_w_after'], permuted2[precord].memory if plen else PRIOR2,
                  label+'/permuted2 after')
            check(full['local_w_after'], full24[frecord].memory if flen else PRIOR2,
                  label+'/full24 after')
            exact['router_weights'] &= (all(x > 0 for x in after3) and
                abs(math.fsum(after3)-1) <= 1e-10 and
                0 < float(row['pooled2_w_after']) < 1 and
                0 < float(row['permuted2_w_after']) < 1)

            max_norm = max(max_norm,float(row['max_norm_error']),float(full['max_norm_error']))
            delta = float(row['bank3_live'])-float(row['pooled2_live'])
            sample = dict(row,difference=delta,raw_hex=raw[max(0,t-16):t+17].hex())
            if best is None or delta > best['difference']: best = sample
            if worst is None or delta < worst['difference']: worst = sample
            history.append(rank); history = history[-32:]
        need(next(rows,None) is None and next(fulls,None) is None, label+' no excess rows')

    life = dict(world=world, regime=regime,
        arms={arm:states[arm].result() for arm in ARMS}, max_norm_error=max_norm,
        exactness=exact, raw_help=best, raw_harm=worst)
    max_error = max(max_error, compare(life, saved, f'RESULT/{world}/{regime}'))
    return life, N*len(ARMS), max_error


def mean(values): return math.fsum(values)/len(values)


def rebuild(lives, books):
    by = {(x['world'],x['regime']):x for x in lives}
    vals = lambda regime,arm,key: [by[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs = lambda regime,a,b,key: [x-y for x,y in zip(vals(regime,a,key),vals(regime,b,key))]
    extra = sorted({x['extra_bytes'] for x in books})
    portable = {arm: sorted({x['portable_bytes'][arm] for x in books}) for arm in ARMS}
    d1 = diffs('partial','bank3','pooled2','early')
    pooled_early = mean(vals('recombined','pooled2','early'))
    pooled_full = mean(vals('recombined','pooled2','gain'))
    bank_early = mean(vals('recombined','bank3','early'))
    bank_full = mean(vals('recombined','bank3','gain'))
    excess = {regime:{key: mean(vals(regime,'permuted2',key))-mean(vals(regime,'pooled2',key))
                      for key in ('early','gain','tail')} for regime in REGIMES}
    per_byte = {regime:{key: mean(diffs(regime,'bank3','pooled2',key))/extra[0]
                        for key in ('early','gain','tail')} for regime in REGIMES}
    shared_prefix_identity = all(
        by[w,regime]['arms'][arm]['early'] == by[w,'recombined']['arms'][arm]['early']
        for w in WORLDS for regime in ('partial','switched','moved_mid') for arm in ARMS)
    admissions = {arm: sum(by[w,r]['arms'][arm]['activation'] is not None
                           for w in WORLDS for r in REGIMES) for arm in ARMS}
    identical = all(len({by[w,r]['arms'][arm]['activation'] for arm in GATED}) == 1
                    for w in WORLDS for r in REGIMES)
    unrelated = [dict(world=w, arm=arm, activation=by[w,'unrelated']['arms'][arm]['activation'],
                      gain=by[w,'unrelated']['arms'][arm]['gain'])
                 for w in WORLDS for arm in ARMS
                 if by[w,'unrelated']['arms'][arm]['activation'] is not None]
    q = dict(
        d1_partial_early_vs_pooled2=mean(d1), d1_partial_early_wins=sum(x > 0 for x in d1),
        d1_bank3_partial_early=mean(vals('partial','bank3','early')),
        d1_pooled2_partial_early=mean(vals('partial','pooled2','early')),
        d2_extra_portable_bytes=extra, d2_portable_bytes=portable,
        d2_bits_per_extra_byte=per_byte,
        d3_recombined_early_retention=bank_early/max(pooled_early,1e-300),
        d3_recombined_full_retention=bank_full/max(pooled_full,1e-300),
        d3_pooled2_recombined_early=pooled_early, d3_pooled2_recombined_full=pooled_full,
        d3_bank3_recombined_early=bank_early, d3_bank3_recombined_full=bank_full,
        d4_permuted2_excess=excess,
        d4_max_gain_excess=max(excess[regime]['gain'] for regime in REGIMES),
        partial_tail_vs_pooled2=mean(diffs('partial','bank3','pooled2','tail')),
        partial_tail_wins=sum(x > 0 for x in diffs('partial','bank3','pooled2','tail')),
        partial_whole_vs_pooled2=mean(diffs('partial','bank3','pooled2','gain')),
        full24_recombined_early=mean(vals('recombined','full24','early')),
        full24_recombined_full=mean(vals('recombined','full24','gain')),
        full24_partial_early=mean(vals('partial','full24','early')),
        shared_prefix_identity=shared_prefix_identity,
        unrelated_admissions=unrelated, admitted_arms=admissions)
    validity = dict(archive=True, source_counts=True,
        normalization=all(x['max_norm_error'] <= 1e-8 for x in lives),
        exact_quotes=all(all(x['exactness'].values()) for x in lives),
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                   for x in lives for s in x['arms'].values()),
        identical_admissions=identical, unrelated_disclosed=True)
    conditions = dict(
        D1=q['d1_partial_early_vs_pooled2'] > 0 and q['d1_partial_early_wins'] >= 5,
        D2=extra[0] > 0 and len(extra) == 1 and set(per_byte) == set(REGIMES),
        D3=pooled_early > 0 and pooled_full > 0 and
           q['d3_recombined_early_retention'] >= .95 and
           q['d3_recombined_full_retention'] >= .95,
        D4=all(excess[regime]['gain'] <= 0 for regime in REGIMES),
        D5=all(validity.values()), D6=True)
    tables = {regime:{arm:{key:mean(vals(regime,arm,key)) for key in ('early','gain','tail')}
              for arm in ARMS} for regime in REGIMES}
    material = conditions['D1'] and conditions['D3'] and conditions['D4'] and conditions['D5']
    return dict(tables=tables, quantities=q, validity=validity, conditions=conditions,
                material_pass=material, gate_pass=material and conditions['D6'])


def check_schema(result):
    for key in ('namespace','worlds','regimes','arms','gated_arms','protocol_sha256','lives',
                'tables','quantities','conditions','validity','material_pass','gate_pass',
                'independent_reader_pending'):
        need(key in result, 'RESULT.json missing '+key)
    need(set(result['conditions']) == {'D1','D2','D3','D4','D5','D6'},
         'RESULT.json condition keys')
    need(set(result['validity']) == {'archive','source_counts','normalization','exact_quotes',
                                     'bounds','identical_admissions','unrelated_disclosed'},
         'RESULT.json validity keys')
    return True


def identity(result):
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, wanted in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                            arms=list(ARMS)).items():
        need(result[key] == wanted and frozen[key] == wanted, 'identity '+key)
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA and result['protocol_sha256'] == PROTOCOL_SHA,
         'protocol identity '+str(HERE/'PROTOCOL.md'))
    need(result['independent_reader_pending'] is True, 'writer pending reader')
    need(result['conditions']['D6'] is False and result['gate_pass'] is False,
         'writer cannot certify the independent reader')
    for name, wanted in frozen['files'].items():
        need(digest(ROOT/name) == wanted, 'freeze '+str(ROOT/name))
    counts = {}
    for name in ('DATA_MANIFEST.json','EXTRACT_MANIFEST.json','MEMORY_MANIFEST.json',
                 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for rel, wanted in manifest.items():
            need(digest(HERE/rel) == wanted, 'manifest '+str(HERE/rel))
        counts[name] = len(manifest)
    return dict(freeze_sha256=digest(HERE/'FREEZE.json'), reader_sha256=digest(Path(__file__)),
                result_sha256=digest(HERE/'RESULT.json'), freeze_files=len(frozen['files']),
                manifest_entries=counts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'new output path '+str(args.output))
    result = json.loads((HERE/'RESULT.json').read_text())
    check_schema(result)
    pins = identity(result)
    recorded = {(x['world'],x['regime']):x for x in result['lives']}
    need(set(recorded) == {(w,r) for w in WORLDS for r in REGIMES}, 'complete life grid')
    lives, books, count, max_error = [], [], 0, 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            tables, receipt = verify_memory(world); books.append(receipt)
            for regime in REGIMES:
                life, n, error = check_life(world, regime, tables, recorded[world,regime])
                lives.append(life); count += n; max_error = max(max_error, error)
                print('verified', world, regime, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(ARMS), 'forecast count')
    rebuilt = rebuild(lives, books)
    for key in ('tables','quantities','validity'):
        max_error = max(max_error, compare(result[key], rebuilt[key], 'RESULT/'+key))
    for key in ('D1','D2','D3','D4','D5'):
        need(result['conditions'][key] == rebuilt['conditions'][key], 'RESULT/conditions/'+key)
    need(result['material_pass'] == rebuilt['material_pass'], 'RESULT/material_pass')
    receipt = dict(verification_pass=True, material_pass=rebuilt['material_pass'],
        gate_pass=rebuilt['gate_pass'], forecasts_checked=count, max_error=max_error,
        source_counts_checked=sum(x['counts_checked']+x['book_counts_checked'] for x in books),
        conditions=rebuilt['conditions'], quantities=rebuilt['quantities'],
        validity=rebuilt['validity'], pins=pins, source_books=books,
        tables=rebuilt['tables'], lives=lives,
        scope='Independent source continuation recount, exact bank/pooled/permuted archives on '
              'both address sets, three-way and binary router trajectories, the shared admission, '
              'cross-trace chronology, outer authority, metrics and gate. Inherited grammar '
              'selection and HEAD256 P0/bindings are hash-pinned supplied boundaries.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False); stream.write('\n')
    print(json.dumps({k:receipt[k] for k in ('verification_pass','material_pass','gate_pass',
                      'forecasts_checked','max_error','source_counts_checked','conditions')},
                     sort_keys=True))


if __name__ == '__main__': main()
