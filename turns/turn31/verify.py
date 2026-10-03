#!/usr/bin/env python3
"""Independent turn31 probability-space reader; no new writer/C imports.

Recounts immutable source continuations through the frozen independent28
reader. Replays factorized u/v, balanced, flat and pooled comparisons on the
same tape; the old shared outer law is imported from the frozen reader30.
HEAD256 P0/bindings and inherited source selection are supplied boundaries.
"""
import argparse
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
WORLDS = tuple(range(288, 296))
REGIMES = ('recombined', 'partial', 'switched', 'moved_mid', 'unrelated')
ARMS = ('factored', 'flat3', 'balanced', 'pooled', 'permuted', 'cold')
NAMESPACE = 'netta-conditional-case-v1'
N = 16384
TOL = 1e-7
U0 = Decimal(7)/8
V0 = Decimal(1)/2
RHO = Decimal(1)/1024
PROTOCOL_SHA = '42a30609026a65c92b0e4e41eef948762dfd8ca13c09b581e8575c72556efde2'

spec = importlib.util.spec_from_file_location('turn31_frozen_reader30', ROOT/'turns/turn30/verify.py')
p30 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p30
spec.loader.exec_module(p30)
p28 = p30.p28
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES
need, near, digest, compare = p28.need, p28.near, p28.digest, p28.compare
power2, mass_log2 = p28.power2, p28.mass_log2
SharedOuter = p30.SharedOuter
Flat = p30.Router3

COMMON = ('t', 'k', 'heads', 'cold_heads', 'truth', 'rank', 'history', 'logcold',
          'matched', 'record', 'source_pooled', 'source_a', 'source_b',
          'source_permuted_a', 'source_permuted_b', 'max_norm_error', 'new_exact',
          'admission_shadow_before', 'admitted_before')
STATE_FIELDS = ('factored_u_before', 'factored_v_before', 'factored_u_after', 'factored_v_after',
                'flat3_w_before', 'flat3_w_after', 'balanced_u_before', 'balanced_u_after',
                'pooled_u_before', 'pooled_u_after', 'permuted_u_before', 'permuted_v_before',
                'permuted_u_after', 'permuted_v_after')
OUTER_FIELDS = ('candidate', 'live', 'shadow_before', 'odds_before',
                'active_before', 'activated_after', 'gain_after')
FIELDS = COMMON+STATE_FIELDS+tuple(a+'_'+f for a in ARMS for f in OUTER_FIELDS)


def log_mix(weight, first, second):
    if first == second:
        return first
    scale = max(first, second)
    total = weight*power2(first-scale)+(1-weight)*power2(second-scale)
    return scale+mass_log2(total)


class Binary:
    """One explicit normalized memory/cold mixture, probability Bayes update."""
    def __init__(self):
        self.u = U0

    def quote(self, cold, source):
        return log_mix(self.u, source, cold)

    def observe(self, cold, source, matched=True):
        if not matched:
            return
        scale = max(cold, source)
        memory = self.u*power2(source-scale)
        local = (1-self.u)*power2(cold-scale)
        self.u = (1-RHO)*memory/(memory+local)+RHO*U0
        need(RHO*U0 <= self.u <= 1-RHO*(1-U0), 'binary share support')


class Factored:
    """Two posterior ratios; each uses pretruth state and its own denominator."""
    def __init__(self):
        self.u, self.v = U0, V0

    def conditional(self, a, b):
        return log_mix(self.v, a, b)

    def quote(self, cold, a, b):
        if a == b == cold:
            return cold
        return log_mix(self.u, self.conditional(a, b), cold)

    def observe(self, cold, a, b, matched=True):
        if not matched:
            return
        # One common scale cancels from BOTH ratios; no updated u enters v.
        scale = max(cold, a, b)
        local, pa, pb = (power2(p-scale) for p in (cold, a, b))
        amass, bmass = self.v*pa, (1-self.v)*pb
        source = amass+bmass
        memory = self.u*source
        total = (1-self.u)*local+memory
        next_u = (1-RHO)*memory/total+RHO*U0
        next_v = (1-RHO)*amass/source+RHO*V0
        self.u, self.v = next_u, next_v
        need(RHO*U0 <= self.u <= 1-RHO*(1-U0), 'factored u share support')
        need(RHO*V0 <= self.v <= 1-RHO*(1-V0), 'factored v share support')


def distribution(log_heads, counts):
    repeats = [math.exp2(x) for x in log_heads]
    total = math.fsum(repeats)
    k = len(repeats)
    valid = sum(counts[1:k+1]) if counts is not None else 0
    changed = ([total*(counts[r]+.5)/(valid+.5*k) for r in range(1, k+1)]
               if k and valid else repeats)
    need(all(x > 0 for x in changed) and 0 <= total < 1,
         'positive source support and residual')
    need(abs(math.fsum([1-total]+changed)-1) <= 1e-8, 'source normalization')
    return repeats, changed


def mix_support(u, v, local, a, b):
    return [(1-float(u))*c+float(u)*(float(v)*x+(1-float(v))*y)
            for c, x, y in zip(local, a, b)]


def check_life(world, regime, tables, saved):
    path = HERE/'results'/f'world{world}'/(regime+'.turn31.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw) == N, str(path)+' raw-byte horizon')
    if regime in ('partial', 'switched', 'moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:8192] == base[:8192], str(path)+' shared raw prefix')
    nrecord = len(tables['bank'])
    factored = [Factored() for _ in range(nrecord)]
    flat = [Flat() for _ in range(nrecord)]
    balanced = [Binary() for _ in range(nrecord)]
    pooled = [Binary() for _ in range(nrecord)]
    permuted = [Factored() for _ in range(nrecord)]
    states = {arm: SharedOuter() for arm in ARMS}
    history, admitted, admission_shadow = [], False, 0.0
    max_error = max_norm = 0.0
    best = worst = None
    exactness = dict(new=True, equal=True, inactive=True, history=True,
                     shared_admission=True, router_weights=True)

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
            heads, k = p28.ints(row['heads']), int(row['k'])
            rank = p28.bindings(k, heads, raw[t], label)
            need(int(row['rank']) == rank, label+' observed rank')
            need(row['history'] == (''.join(map(str, history)) if history else '-'),
                 label+' pretruth history')
            cold, logs = float(row['logcold']), p28.floats(row['cold_heads'])
            need(math.isfinite(cold) and cold <= 0 and len(logs) == k and
                 all(math.isfinite(x) and x <= 0 for x in logs), label+' supplied P0')
            if rank:
                check(cold, logs[rank-1], label+' truth-head price', 1e-10)
            else:
                need(math.exp2(cold) <= 1-math.fsum(math.exp2(x) for x in logs)+1e-12,
                     label+' NEW in residual mass')
            need(int(row['new_exact']) == 1, label+' C all-vector protected NEW')
            norm = float(row['max_norm_error'])
            need(math.isfinite(norm) and 0 <= norm <= 1e-8, label+' C full-vector normalization')
            max_norm = max(max_norm, norm)
            length, record, books = p28.suffix_match(tables['bank'], history)
            plen, precord, pooled_counts = p28.suffix_match(tables['small'], history)
            nlen, nrecord_index, perm_counts = p28.suffix_match(tables['perm'], history)
            need((length, record) == (plen, precord) == (nlen, nrecord_index),
                 label+' identical addresses across books')
            need(int(row['matched']) == length and int(row['record']) == record,
                 label+' current longest match')
            a_counts, b_counts = books if length else (None, None)
            pa_counts, pb_counts = perm_counts if length else (None, None)
            prices = [p28.source_candidate(cold, logs, rank, c)
                      for c in (pooled_counts, a_counts, b_counts, pa_counts, pb_counts)]
            pool_price, a, b, pa, pb = prices
            for field, price in zip(('source_pooled', 'source_a', 'source_b',
                                      'source_permuted_a', 'source_permuted_b'), prices):
                check(row[field], price, label+'/'+field)
                if not rank or not length:
                    need(float(row[field]) == cold, label+' exact source fallback '+field)
            repeats, pdist = distribution(logs, pooled_counts)
            _, adist = distribution(logs, a_counts)
            _, bdist = distribution(logs, b_counts)
            _, nadist = distribution(logs, pa_counts)
            _, nbdist = distribution(logs, pb_counts)
            residual = 1-math.fsum(repeats)
            f = factored[record] if length else Factored()
            q = flat[record] if length else Flat()
            d = balanced[record] if length else Binary()
            p = pooled[record] if length else Binary()
            n = permuted[record] if length else Factored()

            def router_fields(stage):
                for name, state in (('factored', f), ('permuted', n)):
                    for key in ('u', 'v'):
                        value = float(row[name+'_'+key+'_'+stage])
                        need(0 < value < 1, label+' valid '+name+'/'+key)
                        check(value, getattr(state, key), label+'/'+name+'/'+key+'/'+stage)
                for name, state in (('balanced', d), ('pooled', p)):
                    value = float(row[name+'_u_'+stage])
                    need(0 < value < 1, label+' valid '+name+'/u')
                    check(value, state.u, label+'/'+name+'/u/'+stage)
                values = p28.floats(row['flat3_w_'+stage])
                need(len(values) == 3 and all(x > 0 for x in values) and
                     abs(math.fsum(values)-1) <= 1e-10, label+' valid flat weights')
                for i, (x, y) in enumerate(zip(values, q.weights)):
                    check(x, y, label+f'/flat3/{stage}/{i}')

            router_fields('before')
            equal_case = log_mix(V0, a, b)
            candidates = dict(factored=f.quote(cold, a, b), flat3=q.quote(cold, a, b),
                              balanced=d.quote(cold, equal_case), pooled=p.quote(cold, pool_price),
                              permuted=n.quote(cold, pa, pb), cold=cold)
            supports = dict(factored=mix_support(f.u, f.v, repeats, adist, bdist),
                            flat3=[float(q.weights[0])*c+float(q.weights[1])*x+
                                   float(q.weights[2])*y for c, x, y in zip(repeats, adist, bdist)],
                            balanced=mix_support(d.u, V0, repeats, adist, bdist),
                            pooled=[(1-float(p.u))*c+float(p.u)*x for c, x in zip(repeats, pdist)],
                            permuted=mix_support(n.u, n.v, repeats, nadist, nbdist), cold=repeats)
            for arm, changed in supports.items():
                need(all(x > 0 for x in changed) and
                     abs(math.fsum([residual]+changed)-1) <= 1e-8,
                     label+'/'+arm+' positive normalized support')
            need(int(row['admitted_before']) == int(admitted), label+' shared active state')
            check(row['admission_shadow_before'], admission_shadow, label+' incumbent shadow')
            admission_shadow += candidates['pooled']-cold
            admit = not admitted and admission_shadow >= 32
            if admit:
                admitted = True
            wanted = {}
            for arm in ARMS:
                wanted[arm] = states[arm].step(t, cold, candidates[arm], admit)
                for key, value in wanted[arm].items():
                    if type(value) is int:
                        need(int(row[arm+'_'+key]) == value, label+'/'+arm+'/'+key)
                    else:
                        check(row[arm+'_'+key], value, label+'/'+arm+'/'+key)
                if not rank:
                    need(float(row[arm+'_candidate']) == cold and float(row[arm+'_live']) == cold,
                         label+' exact protected NEW '+arm)
                if candidates[arm] == cold:
                    need(float(row[arm+'_candidate']) == cold and float(row[arm+'_live']) == cold,
                         label+' exact common-price passthrough '+arm)
                if not wanted[arm]['active_before']:
                    need(float(row[arm+'_live']) == cold, label+' cold before admission '+arm)
            need(len({x['active_before'] for x in wanted.values()}) == 1 and
                 len({x['activated_after'] for x in wanted.values()}) == 1,
                 label+' identical six-arm admission')

            # Update every router only after its current forecast was checked.
            f.observe(cold, a, b, bool(length))
            q.observe(cold, a, b, bool(length))
            d.observe(cold, equal_case, bool(length))
            p.observe(cold, pool_price, bool(length))
            n.observe(cold, pa, pb, bool(length))
            router_fields('after')
            difference = float(row['factored_live'])-float(row['flat3_live'])
            sample = dict(row, difference=difference, raw_hex=raw[max(0, t-16):t+17].hex())
            if best is None or difference > best['difference']:
                best = sample
            if worst is None or difference < worst['difference']:
                worst = sample
            history.append(rank)
            history = history[-32:]
        need(next(rows, None) is None, str(path)+' excess trace rows')
    life = dict(world=world, regime=regime, arms={a: states[a].result() for a in ARMS},
                max_norm_error=max_norm, exactness=exactness, raw_help=best, raw_harm=worst)
    max_error = max(max_error, compare(saved, life, f'RESULT/{world}/{regime}'))
    return life, N*len(ARMS), max_error


def mean(values):
    return math.fsum(values)/len(values)


def rebuild(lives, books):
    indexed = {(x['world'], x['regime']): x for x in lives}

    def values(regime, arm, key):
        return [indexed[w, regime]['arms'][arm][key] for w in WORLDS]

    def differences(regime, arm, key):
        return [a-b for a, b in zip(values(regime, 'factored', key), values(regime, arm, key))]

    tables = {regime: {arm: {key: mean(values(regime, arm, key))
                             for key in ('early', 'gain', 'tail')} for arm in ARMS}
              for regime in REGIMES}
    comparisons = {}
    for regime in REGIMES:
        comparisons[regime] = {}
        for arm in ('flat3', 'balanced', 'pooled'):
            comparisons[regime][arm] = {}
            for key in ('early', 'gain', 'tail'):
                paired = differences(regime, arm, key)
                comparisons[regime][arm][key] = dict(mean=mean(paired),
                                                     wins=sum(x > 0 for x in paired),
                                                     per_world=paired)
    retention = dict(
        recombined_early=tables['recombined']['factored']['early']/
                          max(tables['recombined']['pooled']['early'], 1e-300),
        recombined_full=tables['recombined']['factored']['gain']/
                         max(tables['recombined']['pooled']['gain'], 1e-300),
        partial_full=tables['partial']['factored']['gain']/
                     max(tables['partial']['pooled']['gain'], 1e-300))
    excess = {regime: tables[regime]['permuted']['gain']-tables[regime]['factored']['gain']
              for regime in REGIMES}
    flat_balanced = {
        regime: {key: mean([a-b for a, b in zip(values(regime, 'flat3', key),
                                               values(regime, 'balanced', key))])
                 for key in ('early', 'gain', 'tail')} for regime in REGIMES}
    portable = {arm: sorted({book['bytes'][('pooled_small.bin' if arm == 'pooled' else
                                           'permuted.bin' if arm == 'permuted' else 'bank.bin')]
                            for book in books}) if arm != 'cold' else [0] for arm in ARMS}
    sizes = dict(factored=16, flat3=24, balanced=8, pooled=8, permuted=16, cold=0)
    recipient = {arm: sorted({book['bank_records']*sizes[arm] for book in books}) for arm in ARMS}
    shared_prefix = all(
        indexed[w, r]['arms'][a]['horizons']['8192'] ==
        indexed[w, 'recombined']['arms'][a]['horizons']['8192']
        for w in WORLDS for r in ('partial', 'switched', 'moved_mid') for a in ARMS)
    admissions = [dict(world=w, arm=arm,
                       activation=indexed[w, 'unrelated']['arms'][arm]['activation'],
                       gain=indexed[w, 'unrelated']['arms'][arm]['gain'])
                  for w in WORLDS for arm in ARMS
                  if indexed[w, 'unrelated']['arms'][arm]['activation'] is not None]
    positive = sum(x > 0 for x in values('recombined', 'factored', 'gain'))
    quantities = dict(comparisons=comparisons, retention=retention, permuted_excess=excess,
                      flat_minus_balanced=flat_balanced, portable_bytes=portable,
                      recipient_router_bytes=recipient, shared_prefix_identity=shared_prefix,
                      unrelated_admissions=admissions, recombined_positive=positive)
    identical = all(len({x['arms'][arm]['activation'] for arm in ARMS}) == 1 for x in lives)
    validity = dict(archive=all(all(n <= 528 for n in book['bytes'].values()) for book in books),
                    source_counts=True,
                    normalization=all(x['max_norm_error'] <= 1e-8 for x in lives),
                    exact_quotes=all(all(x['exactness'].values()) for x in lives),
                    bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                               for x in lives for s in x['arms'].values()),
                    identical_admissions=identical, shared_prefix=shared_prefix,
                    unrelated_disclosed=True)
    positive_pooled = (tables['recombined']['pooled']['early'] > 0 and
                       tables['recombined']['pooled']['gain'] > 0 and
                       tables['partial']['pooled']['gain'] > 0)
    conditions = dict(
        F1=all(comparisons['partial'][a]['tail']['mean'] > 1 and
               comparisons['partial'][a]['tail']['wins'] >= 5 for a in ('flat3', 'balanced')),
        F2=positive_pooled and min(retention.values()) >= .95 and positive == 8,
        F3=all(x < 0 for x in excess.values()), F4=all(validity.values()))
    material = all(conditions[k] for k in ('F1', 'F2', 'F3')) and all(validity.values())
    return dict(tables=tables, quantities=quantities, validity=validity,
                conditions=conditions, material_pass=material,
                gate_pass=material and conditions['F4'])


def check_schema(result):
    for key in ('namespace', 'worlds', 'regimes', 'arms', 'protocol_sha256', 'lives',
                'tables', 'quantities', 'conditions', 'validity', 'material_pass',
                'independent_reader_pending', 'gate_pass'):
        need(key in result, 'RESULT.json missing '+key)
    need(set(result['conditions']) == {'F1', 'F2', 'F3', 'F4'}, 'RESULT condition keys')
    need(set(result['validity']) == {'archive', 'source_counts', 'normalization', 'exact_quotes',
                                    'bounds', 'identical_admissions', 'shared_prefix',
                                    'unrelated_disclosed'}, 'RESULT validity keys')
    need(set(result['quantities']) == {'comparisons', 'retention', 'permuted_excess',
                                      'flat_minus_balanced', 'portable_bytes',
                                      'recipient_router_bytes', 'shared_prefix_identity',
                                      'unrelated_admissions', 'recombined_positive'},
         'RESULT quantity keys')
    return True


def identity(result):
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for key, wanted in dict(namespace=NAMESPACE, worlds=list(WORLDS), regimes=list(REGIMES),
                            arms=list(ARMS)).items():
        need(result[key] == wanted and frozen[key] == wanted, 'identity '+key)
    p28.check_artifact(HERE/'PROTOCOL.md', PROTOCOL_SHA)
    need(result['protocol_sha256'] == PROTOCOL_SHA, 'RESULT protocol identity')
    need(result['independent_reader_pending'] is True and result['conditions']['F4'] is False
         and result['gate_pass'] is False, 'writer cannot certify independent reader')
    required = ('turns/turn31/verify.py', 'turns/turn31/PROTOCOL.md', 'turns/turn31/INTERFACE.md',
                'turns/turn30/verify.py', 'turns/turn28/verify.py')
    need(all(name in frozen['files'] for name in required), 'frozen direct reader dependencies')
    for name, wanted in frozen['files'].items():
        p28.check_artifact(ROOT/name, wanted)
    counts = {}
    for name in ('DATA_MANIFEST.json', 'EXTRACT_MANIFEST.json', 'MEMORY_MANIFEST.json',
                 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for relative, wanted in manifest.items():
            p28.check_artifact(HERE/relative, wanted)
        counts[name] = len(manifest)
    return dict(reader_sha256=digest(Path(__file__)), freeze_sha256=digest(HERE/'FREEZE.json'),
                result_sha256=digest(HERE/'RESULT.json'), freeze_files=len(frozen['files']),
                manifest_entries=counts, protocol_sha256=PROTOCOL_SHA)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'new output path '+str(args.output))
    result = json.loads((HERE/'RESULT.json').read_text())
    check_schema(result)
    pins = identity(result)
    recorded = {(x['world'], x['regime']): x for x in result['lives']}
    need(len(recorded) == len(result['lives']) and
         set(recorded) == {(w, r) for w in WORLDS for r in REGIMES},
         'complete unique life grid')
    lives, books, count, max_error = [], [], 0, 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            tables, source = p28.verify_books(world)
            books.append(source)
            for regime in REGIMES:
                life, forecasts, error = check_life(world, regime, tables, recorded[world, regime])
                lives.append(life)
                count += forecasts
                max_error = max(max_error, error)
                print('verified', world, regime, flush=True)
    need(count == len(WORLDS)*len(REGIMES)*N*len(ARMS), 'complete forecast count')
    rebuilt = rebuild(lives, books)
    for key in ('tables', 'quantities', 'validity', 'material_pass'):
        max_error = max(max_error, compare(result[key], rebuilt[key], 'RESULT/'+key))
    for key in ('F1', 'F2', 'F3'):
        need(result['conditions'][key] == rebuilt['conditions'][key], 'RESULT/conditions/'+key)
    need(rebuilt['conditions']['F4'], 'independent reconstruction validity')
    receipt = dict(verification_pass=True, material_pass=rebuilt['material_pass'],
                   gate_pass=rebuilt['gate_pass'], forecasts_checked=count, max_error=max_error,
                   source_counts_checked=sum(x['counts_checked']+x['book_counts_checked'] for x in books),
                   conditions=rebuilt['conditions'], quantities=rebuilt['quantities'],
                   validity=rebuilt['validity'], tables=rebuilt['tables'], pins=pins,
                   source_books=books, lives=lives,
                   scope='Independent source continuation recount and exact archive reconstruction; '
                         'factorized/flat/balanced/pooled/permuted quotes and recipient states, '
                         'shared admission, outer trajectories, all metrics and gates. Inherited '
                         'greedy selection and HEAD256 cold/bindings remain supplied pinned inputs; '
                         'no independent frontend or generator reconstruction.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({key: receipt[key] for key in
                      ('verification_pass', 'material_pass', 'gate_pass', 'forecasts_checked',
                       'max_error', 'source_counts_checked', 'conditions')}, sort_keys=True))


if __name__ == '__main__':
    main()
