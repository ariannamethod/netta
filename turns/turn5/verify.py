#!/usr/bin/env python3
"""Independent BANK2-REVISE reader; imports no experiment/evaluator code.

The audited turn3 reader supplies only chronological HEAD256 trace and archive
primitives. New row revision is reconstructed with normalized probability
weights, static comparisons with absolute component capitals, and the outer
absorbing HMM with a separate Decimal probability forward pass.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor
import contextlib
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
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('audited_bank2_reader', HERE.parent/'turn3'/'verify.py')
primitive = importlib.util.module_from_spec(spec)
spec.loader.exec_module(primitive)
need, near, sha = primitive.need, primitive.near, primitive.sha
PATTERNS, WIDTH = primitive.PATTERNS, primitive.WIDTH
N = 16384
WORLDS = tuple(range(48, 56))
REGIMES = ('mosaic', 'unrelated', 'mosaic_then_unrelated')
ARMS = ('row2', 'static3', 'revise3', 'null3')
PRIOR = (1/8, 7/16, 7/16)
RHO = 2.0 ** -10
HAZARD = 2.0 ** -16
PENALTY = -math.log2(1-RHO)
TOL = 1e-7


def source_laws(world, books):
    real = tuple({p: tuple((2*x+1)/(2*sum(a)+len(a)) for x in a)
                  for p, a in book.items()} for book in books)
    key = f'netta-revision-v1|{world}|head-bank-null-0'.encode()
    rng = random.Random(int.from_bytes(hashlib.sha256(key).digest()[:8], 'big'))
    donors = {}
    for width in range(2, 8):
        rows = tuple(p for p in PATTERNS if WIDTH[p] == width)
        shift = rng.randrange(1, len(rows)) if len(rows) > 1 else 0
        donors.update(zip(rows, rows[shift:]+rows[:shift]))
    null = []
    for index, book in enumerate(books):
        shifted = {}
        for p, donor in donors.items():
            new = real[index][p][-1]
            counts = book[donor]
            repeat_denominator = 2*sum(counts[:-1])+len(counts)-1
            shifted[p] = tuple((1-new)*(2*n+1)/repeat_denominator
                               for n in counts[:-1]) + (new,)
            near(math.fsum(shifted[p]), 1.0, 'null normalization', 1e-12)
            need(shifted[p][-1] == real[index][p][-1], 'null protected NEW')
        null.append(shifted)
    return real, tuple(null), donors


class StaticRow:
    """Absolute group-likelihood capitals, unlike the writer's log ratios."""
    def __init__(self, two=False):
        self.capitals = [0.0, 0.0, 0.0]
        self.two = two

    def quote(self):
        scores = ([-math.inf, self.capitals[1]-1, self.capitals[2]-1]
                  if self.two else [math.log2(p)+v for p, v in zip(PRIOR, self.capitals)])
        top = max(scores)
        unnormalized = [2.0 ** (x-top) for x in scores]
        total = math.fsum(unnormalized)
        weights = tuple(x/total for x in unnormalized)
        ratios = ((0.0, self.capitals[2]-self.capitals[1]) if self.two else
                  (self.capitals[1]-self.capitals[0]+math.log2(3.5),
                   self.capitals[2]-self.capitals[0]+math.log2(3.5)))
        return weights, ratios

    def observe(self, likelihoods):
        for j, price in enumerate(likelihoods):
            self.capitals[j] += math.log2(price)


class RevisionRow:
    """Three probability masses with the declared post-observation sharing."""
    def __init__(self):
        self.weights = PRIOR

    def quote(self):
        w0, wa, wb = self.weights
        return self.weights, (math.log2(wa/w0), math.log2(wb/w0))

    def observe(self, likelihoods):
        joint = [w*price for w, price in zip(self.weights, likelihoods)]
        denominator = math.fsum(joint)
        self.weights = tuple((1-RHO)*j/denominator+RHO*p for j, p in zip(joint, PRIOR))
        near(math.fsum(self.weights), 1.0, 'revision posterior normalization', 1e-14)
        need(all(w >= RHO*p-1e-15 for w, p in zip(self.weights, PRIOR)),
             'post-transition revision floor')


class ProbabilityHMM:
    def __init__(self):
        self.cold = self.source = Decimal('0.5')
        self.active = False
        self.activation = None
        self.shadow = self.gain = self.peak = self.minimum = self.drawdown = 0.0
        self.horizons = {}
        self.shadow_horizons = {}

    def odds(self):
        if not self.active:
            return 0.0
        source, cold = float(self.source), float(self.cold)
        if source > 0 and cold > 0:
            return math.log2(source)-math.log2(cold)
        return float((self.source.ln()-self.cold.ln())/Decimal(2).ln())

    def step(self, t, ratio):
        increment, activated = 0.0, False
        if self.active:
            source_joint = self.source*Decimal.from_float(ratio)
            normalizer = self.cold+source_joint
            increment = math.log2(float(normalizer))
            self.gain += increment
            h = Decimal(1)/65536
            self.cold = (self.cold+h*source_joint)/normalizer
            self.source = (1-h)*source_joint/normalizer
            need(self.cold > 0 and self.source > 0, 'positive HMM path masses')
        self.shadow += math.log2(ratio)
        if not self.active and self.shadow >= 32:
            self.active, self.activation = True, t+1
            self.cold = self.source = Decimal('0.5')
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        need(self.gain >= -1-TOL, 'full-prefix one-bit bound')
        need(self.drawdown <= 16+TOL, 'all-interval sixteen-bit bound')
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
            self.shadow_horizons[str(t+1)] = self.shadow
        return increment, activated

    def result(self):
        return dict(gain=self.gain, at8192=self.horizons['8192'],
                    tail=self.gain-self.horizons['8192'], shadow=self.shadow,
                    candidate_tail=self.shadow-self.shadow_horizons['8192'],
                    activation=self.activation, horizons=self.horizons,
                    candidate_horizons=self.shadow_horizons,
                    minimum=self.minimum, drawdown=self.drawdown, odds=self.odds())


def read_world(job):
    with localcontext() as context:
        context.prec = 50
        context.Emin, context.Emax = -999999, 999999
        return verify_world(*job)


def verify_world(root_string, world):
    root = Path(root_string)
    books, hashes = primitive.read_memory(root, world)
    real, null, donors = source_laws(world, books)
    supports = tuple({p: sum(a) for p, a in book.items()} for book in books)
    metadata = json.loads((root/'results'/f'world{world:02d}-memory.json').read_text())
    need(metadata['counts'] == [{p: list(a) for p, a in b.items()} for b in books],
         f'{world}: re-counted memory metadata')
    need(metadata['support'] == list(supports), f'{world}: support metadata')
    need(metadata['null_donors'] == donors, f'{world}: one joint null permutation')
    folder = root/'data'/f'world{world:02d}'
    need((folder/'mosaic.bin').read_bytes()[:8192] ==
         (folder/'mosaic_then_unrelated.bin').read_bytes()[:8192],
         f'{world}: raw common prefix')
    maximum_error = maximum_normalization_error = maximum_bound_excess = 0.0
    minimum_floor_slack = math.inf
    predictions, lives = 0, []
    for regime in REGIMES:
        local = {p: [0]*WIDTH[p] for p in PATTERNS}
        routers = {
            'row2': {p: StaticRow(two=True) for p in PATTERNS},
            'static3': {p: StaticRow() for p in PATTERNS},
            'revise3': {p: RevisionRow() for p in PATTERNS},
            'null3': {p: RevisionRow() for p in PATTERNS}}
        outer = {arm: ProbabilityHMM() for arm in ARMS}
        complete, visited = 0, set()
        bound_excess = maximum_deficit = cold_loss = 0.0
        with contextlib.ExitStack() as stack:
            path = root/'results'/'events'/f'world{world:02d}'/(regime+'.tsv.gz')
            events = iter(csv.DictReader(stack.enter_context(gzip.open(path, 'rt')), delimiter='\t'))
            c_readers = {arm: iter(csv.DictReader(stack.enter_context(gzip.open(
                root/'results'/'c'/f'world{world:02d}'/f'{regime}-{arm}.tsv.gz', 'rt')), delimiter='\t'))
                for arm in ('row2', 'static3', 'revise3')}
            for t, truth, p, group, masses, logbase, norm in primitive.traced(root, world, regime):
                maximum_normalization_error = max(maximum_normalization_error, norm)
                if p == '-':
                    cold = (1.0,)
                    real_profiles = null_profiles = (cold, cold)
                    g = 0
                else:
                    g = group
                    b = local[p]
                    cold = primitive.cold_groups(b, masses)
                    real_profiles = tuple(primitive.source_groups(real[j][p], supports[j][p],
                                                                   b, masses, cold)[0] for j in range(2))
                    null_profiles = tuple(primitive.source_groups(null[j][p], supports[j][p],
                                                                   b, masses, cold)[0] for j in range(2))
                logcold = logbase+math.log2(cold[g]/masses[g])
                cold_loss -= logcold
                for arm in ARMS:
                    record = next(events, None)
                    label = f'{world}/{regime}/{arm}/{t}'
                    need(record is not None and record['arm'] == arm and int(record['t']) == t,
                         label+': event order')
                    need(record['pattern'] == p and int(record['truth']) == truth and
                         int(record['group']) == group, label+': event identity')
                    source_pair = null_profiles if arm == 'null3' else real_profiles
                    experts = (cold,)+source_pair
                    if p != '-':
                        router = routers[arm][p]
                    else:
                        router = (StaticRow(two=True) if arm == 'row2' else
                                  StaticRow() if arm == 'static3' else RevisionRow())
                    weights_before, ratios_before = router.quote()
                    bank = tuple(math.fsum(w*expert[i] for w, expert in zip(weights_before, experts))
                                 for i in range(len(cold)))
                    need(all(x > 0 for x in bank), label+': positive full-byte group factors')
                    near(math.fsum(bank), 1.0, label+': all-byte normalization', 1e-8)
                    maximum_normalization_error = max(maximum_normalization_error, abs(math.fsum(bank)-1))
                    if p != '-':
                        near(bank[-1], cold[-1], label+': protected NEW', 1e-10)
                    if arm in ('revise3', 'null3'):
                        for w, prior in zip(weights_before, PRIOR):
                            slack = w-RHO*prior
                            minimum_floor_slack = min(minimum_floor_slack, slack)
                            need(slack >= -1e-15, label+': pretruth revision floor')
                    loga, logb = tuple(logbase+math.log2(e[g]/masses[g]) for e in source_pair)
                    ratio = bank[g]/cold[g]
                    logbank = logcold+math.log2(ratio)
                    state = outer[arm]
                    active_before, odds_before, shadow_before = state.active, state.odds(), state.shadow
                    increment, activated = state.step(t, ratio)
                    loglive = logcold+increment
                    if p != '-':
                        router.observe(tuple(e[g] for e in experts))
                    weights_after, ratios_after = router.quote()
                    if arm in ('revise3', 'null3'):
                        for w, prior in zip(weights_after, PRIOR):
                            minimum_floor_slack = min(minimum_floor_slack, w-RHO*prior)
                            need(w >= RHO*prior-1e-15, label+': after-truth revision floor')
                    expected = dict(logbase=logbase, logcold=logcold, logA=loga, logB=logb,
                                    logbank=logbank, loglive=loglive,
                                    weight_cold=weights_before[0], weight_A=weights_before[1],
                                    weight_B=weights_before[2], shadow_before=shadow_before,
                                    odds_before=odds_before, gain_after=state.gain)
                    if arm == 'row2':
                        expected.update(inner_odds_before=ratios_before[1], inner_odds_after=ratios_after[1])
                        need(all(record[name] == '' for name in ('inner_logA_before', 'inner_logB_before',
                                                               'inner_logA_after', 'inner_logB_after')),
                             label+': row2 uses A/B odds, not cold-relative ratios')
                    else:
                        expected.update(inner_logA_before=ratios_before[0], inner_logB_before=ratios_before[1],
                                        inner_logA_after=ratios_after[0], inner_logB_after=ratios_after[1])
                        need(record['inner_odds_before'] == record['inner_odds_after'] == '',
                             label+': three-arm ratio convention')
                    for field, wanted in expected.items():
                        maximum_error = max(maximum_error, near(record[field], wanted, label+': '+field, TOL))
                    need(int(record['active_before']) == active_before and
                         int(record['activated_after']) == activated, label+': own following-event gate')
                    if arm in c_readers:
                        c = next(c_readers[arm], None)
                        need(c is not None and int(c['t']) == t and c['pattern'] == p and
                             int(c['truth']) == truth and int(c['group']) == group, label+': C chronology')
                        need(int(c['active_before']) == active_before and
                             int(c['activated_after']) == activated, label+': C gate')
                        if arm == 'row2':
                            c_fields = ('logbase', 'logcold', 'logA', 'logB', 'logbank', 'loglive',
                                        'inner_odds_before', 'shadow_before', 'gain_after')
                            maximum_error = max(maximum_error,
                                near(c['outer_odds_before'], odds_before, label+': C outer state', TOL))
                        else:
                            c_fields = tuple(expected)
                        for field in c_fields:
                            maximum_error = max(maximum_error,
                                near(c[field], expected[field], label+': C '+field, TOL))
                    predictions += 1
                if p != '-':
                    local[p][group] += 1
                    complete += 1
                    visited.add(p)
                static_price = outer['static3'].shadow-outer['revise3'].shadow
                maximum_deficit = max(maximum_deficit, static_price)
                allowed = (complete-len(visited))*PENALTY
                excess = static_price-allowed
                maximum_bound_excess = max(maximum_bound_excess, excess)
                bound_excess = max(bound_excess, excess)
                need(excess <= TOL, f'{world}/{regime}/{t}: candidate-only no-revision path bound')
            need(next(events, None) is None, f'{world}/{regime}: excess Python events')
            for arm, c in c_readers.items():
                need(next(c, None) is None, f'{world}/{regime}/{arm}: excess C events')
        lives.append(dict(world=world, regime=regime,
                          arms={arm: s.result() for arm, s in outer.items()},
                          candidate_static_bound=dict(complete_events=complete, visited_rows=len(visited),
                              final_bound=(complete-len(visited))*PENALTY,
                              maximum_deficit=maximum_deficit),
                          maximum_bound_excess=bound_excess, cold_loss=cold_loss,
                          event_sha256=sha(path), c_trace_sha256={arm:sha(root/'results'/'c'/
                              f'world{world:02d}'/f'{regime}-{arm}.tsv.gz') for arm in ARMS[:3]}))
    first, switched = lives[0], lives[2]
    for arm in ARMS:
        near(first['arms'][arm]['at8192'], switched['arms'][arm]['at8192'],
             f'{world}/{arm}: identical-prefix live gain', TOL)
        near(first['arms'][arm]['candidate_horizons']['8192'], switched['arms'][arm]['candidate_horizons']['8192'],
             f'{world}/{arm}: identical-prefix candidate gain', TOL)
    return dict(world=world, lives=lives, book_hashes=hashes,
                source_events=[sum(x.values()) for x in supports],
                supported_rows=[sum(n >= 32 for n in x.values()) for x in supports],
                predictions=predictions, maximum_numeric_error=maximum_error,
                maximum_normalization_error=maximum_normalization_error,
                maximum_candidate_bound_excess=maximum_bound_excess,
                minimum_revision_floor_slack=minimum_floor_slack)


def verify_tree(actual, expected, label):
    """Compare an independently reconstructed contract, ignoring no field in it."""
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+': expected object')
        for key, value in expected.items():
            need(key in actual, label+': missing '+key)
            verify_tree(actual[key], value, label+'/'+key)
    elif isinstance(expected, (list, tuple)):
        need(isinstance(actual, (list, tuple)) and len(actual) == len(expected), label+': list shape')
        for j, (a, e) in enumerate(zip(actual, expected)):
            verify_tree(a, e, label+f'/{j}')
    elif isinstance(expected, bool) or expected is None or isinstance(expected, str):
        need(actual == expected, label+': exact field')
    elif isinstance(expected, int):
        need(actual == expected, label+': integer field')
    else:
        near(actual, expected, label, TOL)


def gate(lives):
    preserved = [life for life in lives if life['regime'] == 'mosaic']
    switched = [life for life in lives if life['regime'] == 'mosaic_then_unrelated']
    mean_gains = {arm: math.fsum(life['arms'][arm]['gain'] for life in preserved)/8 for arm in ARMS}
    revised = mean_gains['revise3']
    retained = {arm: revised/value if value > 0 else None
                for arm, value in mean_gains.items() if arm in ('row2', 'static3')}
    positive = sum(life['arms']['revise3']['gain'] > 0 for life in preserved)
    tail = {}
    for comparator in ('row2', 'static3'):
        paired = [life['arms']['revise3']['tail']-life['arms'][comparator]['tail'] for life in switched]
        candidate = [life['arms']['revise3']['candidate_tail']-life['arms'][comparator]['candidate_tail']
                     for life in switched]
        tail[comparator] = dict(paired_differences=paired,
                               mean_improvement=math.fsum(paired)/8,
                               improved_worlds=sum(x > 0 for x in paired),
                               worst_improvement=min(life['arms']['revise3']['tail'] for life in switched)-
                                                 min(life['arms'][comparator]['tail'] for life in switched),
                               candidate_paired_differences=candidate,
                               candidate_mean_improvement=math.fsum(candidate)/8)
    summary = dict(mosaic_mean_gains=mean_gains, mosaic_gain_bpb=revised/N,
                   mosaic_null_contrast_bpb=(revised-mean_gains['null3'])/N,
                   mosaic_positive_worlds=positive, retained_fraction=retained,
                   changed_tail=tail)
    tests = dict(material_gain=summary['mosaic_gain_bpb'] >= .0075,
                 null_contrast=summary['mosaic_null_contrast_bpb'] >= .0075,
                 all_worlds_positive=positive == 8)
    for arm in ('row2', 'static3'):
        tests['retention_'+arm] = retained[arm] is not None and retained[arm] >= .70
        tests['changed_tail_mean_'+arm] = tail[arm]['mean_improvement'] > 1
        tests['changed_tail_count_'+arm] = tail[arm]['improved_worlds'] >= 5
        tests['changed_tail_worst_'+arm] = tail[arm]['worst_improvement'] > 1
    return summary, tests


def input_identity(root):
    frozen = json.loads((root/'CODE_FREEZE.json').read_text())
    need(frozen['worlds'] == list(WORLDS) and frozen['namespace'] == 'netta-revision-v1',
         'frozen new-data identity')
    need(frozen['protocol_sha256'] == sha(root/'PROTOCOL.md'), 'frozen protocol')
    for name, digest in frozen['files'].items():
        need(sha(root.parent/name) == digest, 'frozen code changed: '+name)
    artifacts = json.loads((root/'ARTIFACT_MANIFESTS.json').read_text())
    need(set(artifacts) == {'data', 'traces', 'memory', 'results'}, 'artifact manifest categories')
    for section in artifacts.values():
        for name, digest in section.items():
            need(sha(root/name) == digest, 'sealed artifact changed: '+name)
    return dict(protocol_sha256=sha(root/'PROTOCOL.md'),
                code_freeze_sha256=sha(root/'CODE_FREEZE.json'),
                artifact_manifests_sha256=sha(root/'ARTIFACT_MANIFESTS.json'),
                result_sha256=sha(root/'RESULT.json'), reader_sha256=sha(Path(__file__)),
                primitive_reader_sha256=sha(root.parent/'turn3'/'verify.py'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=HERE)
    parser.add_argument('--workers', type=int, default=4)
    parser.add_argument('--output', type=Path, default=None)
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output or root/'VERIFY.json'
    need(not output.exists(), 'verification result already exists: '+str(output))
    identities = input_identity(root)
    with ProcessPoolExecutor(max_workers=args.workers) as workers:
        checked = list(workers.map(read_world, [(str(root), world) for world in WORLDS]))
    lives = [life for world in checked for life in world['lives']]
    claim = json.loads((root/'RESULT.json').read_text())
    need(claim['worlds'] == list(WORLDS) and claim['namespace'] == 'netta-revision-v1',
         'result new-data identity')
    need(claim['protocol_sha256'] == identities['protocol_sha256'], 'result protocol identity')
    need(len(claim['lives']) == len(lives), 'result life count')
    for stated, calculated in zip(claim['lives'], lives):
        fields = {key: value for key, value in calculated.items() if key != 'maximum_bound_excess'}
        verify_tree(stated, fields, f"life{calculated['world']}/{calculated['regime']}")
        near(stated['checks']['max_candidate_bound_excess'], calculated['maximum_bound_excess'],
             'reported all-prefix candidate bound check', TOL)
        need(stated['checks']['max_c_error'] <= TOL and stated['checks']['max_norm_error'] <= 1e-8,
             'reported C/full-vector contract')
    summary, tests = gate(lives)
    verify_tree(claim['summary'], summary, 'independent summary')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'declared all-comparator gate')
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  identities=identities, worlds=list(WORLDS), source_lives=32,
                  arm_lives=96, c_arm_lives=72, predictions=sum(w['predictions'] for w in checked),
                  maximum_numeric_error=max(w['maximum_numeric_error'] for w in checked),
                  maximum_normalization_error=max(w['maximum_normalization_error'] for w in checked),
                  maximum_candidate_bound_excess=max(w['maximum_candidate_bound_excess'] for w in checked),
                  minimum_revision_floor_slack=min(w['minimum_revision_floor_slack'] for w in checked),
                  summary=summary, tests=tests, checked=checked,
                  scope='Independent source recount, bank decoding, chronological HEAD256 trace contract, component laws, three-state revision, outer HMM, all-prefix bounds and live gate. No reconstruction of frontend BPE training.')
    with output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps({key: result[key] for key in ('verification_pass', 'gate_pass', 'arm_lives',
          'predictions', 'maximum_numeric_error', 'maximum_normalization_error',
          'maximum_candidate_bound_excess', 'minimum_revision_floor_slack', 'summary', 'tests')}, indent=2))


if __name__ == '__main__':
    main()
