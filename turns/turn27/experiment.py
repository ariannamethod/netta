#!/usr/bin/env python3
"""Turn27: one frozen batch, six authority arms on one identical candidate/P0
price tape and one shared admission. The gated candidate is the CUSUM latch."""
import argparse
import concurrent.futures
import csv
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
WORLDS = tuple(range(256, 264))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
ARMS = ('slow', 'fast', 'witness', 'hysteresis', 'cusum', 'orate0')
CANDIDATE = 'cusum'
INCUMBENT = 'hysteresis'
YARDSTICK = 'orate0'
NAMESPACE = 'netta-cusum-latch-v1'
N = 16384
MOVE = 8192
EARLY = 4096
SEAM_REGIME = 'switched'
DRIFT = 0.5
THRESHOLD = 8.0
LATCH_DEADLINE = MOVE+256
HORIZONS = (1024, EARLY, MOVE, N)
TOL = 1e-7
EQUIVALENCE_LIVES = ((256, 'switched'), (256, 'recombined'), (257, 'moved_mid'))

FROZEN = (
    'turns/turn27/PROTOCOL.md', 'turns/turn27/Makefile', 'turns/turn27/cusum.c',
    'turns/turn27/cusum.h', 'turns/turn27/replay.c', 'turns/turn27/experiment.py',
    'turns/turn27/verify.py', 'turns/turn27/preflight.py',
    'turns/turn27/episode', 'turns/turn27/authority_replay',
    'turns/turn25/PROTOCOL.md', 'turns/turn25/INTERFACE.md', 'turns/turn25/latch.c',
    'turns/turn25/latch.h', 'turns/turn25/replay.c', 'turns/turn25/experiment.py',
    'turns/turn25/verify.py',
    'turns/turn24/PROTOCOL.md', 'turns/turn24/REPORT.md', 'turns/turn24/oracle.c',
    'turns/turn24/experiment.py', 'turns/turn24/verify.py', 'turns/turn24/oracle',
    'turns/turn23/split.c', 'turns/turn23/experiment.py',
    'turns/turn22/PROTOCOL.md', 'turns/turn22/experiment.py', 'turns/turn22/verify.py',
    'turns/turn22/authority.c', 'turns/turn22/authority.h', 'turns/turn22/replay.c',
    'turns/turn13/episode.c', 'turns/turn13/experiment.py', 'turns/turn13/verify.py',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c',
)

spec = importlib.util.spec_from_file_location('turn27_parent_turn22',
                                              ROOT/'turns/turn22/experiment.py')
parent = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = parent
spec.loader.exec_module(parent)
parent.HERE, parent.REPO, parent.WORLDS, parent.NAMESPACE = HERE, ROOT, WORLDS, NAMESPACE
parent.t13.HERE, parent.t13.REPO = HERE, ROOT
parent.t13.WORLDS, parent.t13.NAMESPACE = WORLDS, NAMESPACE
assert parent.MOVE == MOVE == parent.t13.N//2 and parent.t13.N == N


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
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def manifest(folder):
    return {str(p.relative_to(HERE)): digest(p)
            for p in sorted(Path(folder).rglob('*')) if p.is_file()}


def check_manifest(name):
    for rel, wanted in json.loads((HERE/name).read_text()).items():
        assert digest(HERE/rel) == wanted, rel


def freeze():
    for folder in ('data', 'memory', 'labels', 'results'):
        assert not (HERE/folder).exists(), folder
    save(HERE/'FREEZE.json', dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES,
         arms=ARMS, candidate=CANDIDATE, incumbent=INCUMBENT, yardstick=YARDSTICK,
         seam=MOVE, seam_regime=SEAM_REGIME, drift=DRIFT, threshold=THRESHOLD,
         latch_deadline=LATCH_DEADLINE,
         files={name: digest(ROOT/name) for name in FROZEN}))
    (HERE/'INHERITED_SHA256.txt').write_text(''.join(
        f'{digest(ROOT/name)}  {name}\n' for name in FROZEN))


def check_freeze():
    """The pre-data pin, plus any recorded amendment (turn24's standing ruling).

    FREEZE.json is never rewritten. A file repaired after the batch existed is
    listed in FREEZE_AMENDMENT.json with both shas and is accepted at exactly
    those two states and no third one.
    """
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['regimes'] == list(REGIMES) and frozen['arms'] == list(ARMS)
    assert frozen['seam'] == MOVE and frozen['seam_regime'] == SEAM_REGIME
    assert frozen['drift'] == DRIFT and frozen['threshold'] == THRESHOLD
    amendment = HERE/'FREEZE_AMENDMENT.json'
    amended = {}
    if amendment.exists():
        note = json.loads(amendment.read_text())
        for name, pair in note['files'].items():
            assert pair['frozen_sha256'] == frozen['files'][name], name
            amended[name] = pair['amended_sha256']
    for name, wanted in frozen['files'].items():
        got = digest(ROOT/name)
        assert got == wanted or got == amended.get(name), name


def generate():
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(parent.generate_world, WORLDS))
    save(HERE/'DATA_MANIFEST.json', manifest(HERE/'data'))


def extract():
    check_manifest('DATA_MANIFEST.json')
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(parent.t13.extract_world, WORLDS))
    save(HERE/'EXTRACT_MANIFEST.json', manifest(HERE/'data'))


def learn():
    check_manifest('EXTRACT_MANIFEST.json')
    for w in WORLDS:
        parent.t13.learn_world(w)
        print('learned', w, flush=True)
    save(HERE/'MEMORY_MANIFEST.json', manifest(HERE/'memory'))


def labels():
    """The generator's hidden law label. Consumed by the orate0 arm only.

    The seam is constructional, not searched. turn13's generator emits the
    switched life from the recombined command prefix under the same recipient
    emission seed and changes the command law at N//2; turn22 builds moved_mid
    by a surface bijection of recombined, so it carries no command tape at all.
    Every one of those facts is checked here on the bytes themselves.
    """
    check_manifest('EXTRACT_MANIFEST.json')
    rows = {}
    for w in WORLDS:
        folder = HERE/'data'/f'world{w}'
        declared = json.loads((folder/'GENERATOR.json').read_text())
        assert declared['world'] == w
        seeds = declared['emission_seeds']
        assert seeds[SEAM_REGIME] == seeds['recombined'], w
        raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
        cmd = {r: (folder/(r+'.commands.bin')).read_bytes()
               for r in ('recombined', SEAM_REGIME, 'unrelated')}
        assert not (folder/'moved_mid.commands.bin').exists(), w
        assert all(len(b) == N for b in raw.values()), w
        assert all(len(b) == N for b in cmd.values()), w
        prefix = raw[SEAM_REGIME][:MOVE] == raw['recombined'][:MOVE]
        cmd_build = cmd[SEAM_REGIME] == cmd['recombined'][:MOVE]+cmd['unrelated'][MOVE:]
        assert prefix and cmd_build, w
        differing = sum(a != b for a, b in
                        zip(raw[SEAM_REGIME][MOVE:], raw['recombined'][MOVE:]))
        rows[str(w)] = dict(
            world=w, seam=MOVE, law_changed_regime=SEAM_REGIME,
            generator_sha256=digest(folder/'GENERATOR.json'),
            shared_emission_seed=seeds[SEAM_REGIME],
            raw_prefix_identical_to_recombined=prefix,
            command_tape_is_declared_construction=cmd_build,
            raw_tail_differing_bytes=differing,
            raw_tail_divergence=differing/(N-MOVE),
            raw_tail_differing_bytes_vs_unrelated=sum(
                a != b for a, b in zip(raw[SEAM_REGIME][MOVE:], raw['unrelated'][MOVE:])))
    save(HERE/'labels'/'GENERATOR_LABELS.json', dict(
        namespace=NAMESPACE, seam=MOVE, law_changed_regime=SEAM_REGIME, worlds=rows,
        derivation="constructional: turns/turn13/experiment.py generate_world sets "
                   "commands['switched'] = commands['recombined'][:N//2] + "
                   "commands['unrelated'][N//2:] under the shared 'recipient' emission "
                   'seed, so the emitted prefix is shared with recombined and the law '
                   'changes at N//2 = 8192',
        scope='hidden generator label, read from data/world{w}/GENERATOR.json and the '
              'generator tapes; consumed by the orate0 yardstick arm only, never by a '
              'gated arm and never by the predictor'))
    save(HERE/'LABELS_MANIFEST.json', manifest(HERE/'labels'))
    print(json.dumps(rows, indent=2))


def empty_stats():
    return dict(gain=0., tail=0., early=None, minimum=0., peak=0., drawdown=0.,
                activation=None, horizons={}, slow_hazard_bytes=0,
                fast_hazard_bytes=0, tail_fast_hazard_bytes=0)


def empty_cusum():
    return dict(latched=False, latch_byte=None, latch_before_seam=None,
                s_at_seam=None, s_final=0., s_max=0., s_max_byte=None,
                s_max_before_seam=0., s_max_after_seam=None,
                s_after_at_latch=None, voting_bytes=0, voting_bytes_after_seam=0)


def empty_hysteresis():
    return dict(first_set=None, first_return=None, set_count=0, return_count=0,
                sets_before_seam=0, returns_before_seam=0,
                first_set_after_seam=None, latch_at_seam=None)


def evaluate_life(w, regime):
    source = HERE/'results'/f'world{w}'/(regime+'.tsv.gz')
    tape = []
    with gzip.open(source, 'rt') as stream:
        for t, r in enumerate(csv.DictReader(stream, delimiter='\t')):
            assert int(r['t']) == t
            tape.append((r['t'], r['logcold'], r['episode_candidate'],
                         r['episode_matchedL'], int(r['rank']), int(r['truth']),
                         int(r['episode_active_before']), int(r['episode_activated_after']),
                         r['episode_live'], float(r['max_norm_error']),
                         r['new_exact'], r['episode_shadow_before']))
    assert len(tape) == N
    payload = ''.join(' '.join(row[:4])+'\n' for row in tape).encode()
    seam = MOVE if regime == SEAM_REGIME else -1
    target = HERE/'results/authority'/f'world{w}'/(regime+'.tsv.gz')
    target.parent.mkdir(parents=True, exist_ok=True)
    done = subprocess.run([str(HERE/'authority_replay'), str(seam)], input=payload,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)

    arms = {a: empty_stats() for a in ARMS}
    cusum = empty_cusum()
    hysteresis = empty_hysteresis()
    exact = dict(admission=True, inactive=True, equal_price=True, protected_new=True,
                 new_column=True, normalized=True, witness_clock=True,
                 hysteresis_law=True, cusum_statistic=True, cusum_one_way=True,
                 cusum_chronology=True, orate0_off_law_is_slow=True,
                 orate0_pre_seam_is_slow=True, orate0_armed_is_fast=True)
    maxnorm = 0.
    rows = []
    with gzip.open(target, 'rt') as stream:
        for t, (src, r) in enumerate(zip(tape, csv.DictReader(stream, delimiter='\t'),
                                         strict=True)):
            (st, scold, scand, smatch, rank, truth, active, activated,
             elive, norm, newcol, sshadow) = src
            assert int(st) == int(r['t']) == t
            cold, candidate, matched = float(scold), float(scand), int(smatch)
            assert float(r['cold']) == cold and float(r['candidate']) == candidate
            assert int(r['matched']) == matched
            maxnorm = max(maxnorm, norm)
            exact['new_column'] &= newcol == '1'
            exact['admission'] &= (int(r['active_before']) == active and
                                   int(r['admitted_after']) == activated and
                                   abs(float(r['shadow_before'])-float(sshadow)) <= TOL)
            live = {a: float(r[a+'_live']) for a in ARMS}
            used = {a: int(r[a+'_slow_used']) for a in ARMS}
            assert abs(live['slow']-float(elive)) <= TOL
            for a in ARMS:
                assert math.isfinite(live[a]) and live[a] <= 1e-8
                exact['inactive'] &= bool(active or live[a] == cold)
                exact['equal_price'] &= bool(candidate != cold or live[a] == cold)
                exact['protected_new'] &= bool(rank != 0 or
                                               (candidate == cold and live[a] == cold))
                s = arms[a]
                s['gain'] += live[a]-cold
                if t >= MOVE:
                    s['tail'] += live[a]-cold
                s['minimum'] = min(s['minimum'], s['gain'])
                s['peak'] = max(s['peak'], s['gain'])
                s['drawdown'] = max(s['drawdown'], s['peak']-s['gain'])
                if t+1 in HORIZONS:
                    s['horizons'][str(t+1)] = s['gain']
                if activated:
                    assert s['activation'] is None
                    s['activation'] = t+1
                s['slow_hazard_bytes'] += used[a] == 1
                s['fast_hazard_bytes'] += used[a] == 0
                s['tail_fast_hazard_bytes'] += used[a] == 0 and t >= MOVE

            # The witness clock and turn25's hysteresis law, recomputed here.
            oldw, neww = float(r['w_before']), float(r['w_after'])
            oldl, newl = int(r['hysteresis_latch_before']), int(r['hysteresis_latch_after'])
            wantw = (31./32.)*oldw+(candidate-cold) if active and matched >= 1 else oldw
            if activated:
                wantw = 0.
            exact['witness_clock'] &= abs(wantw-neww) <= 1e-10
            wantl = (1 if neww <= -1 else 0 if neww >= 1 else oldl) if active else 0
            exact['hysteresis_law'] &= newl == wantl
            if active:
                exact['hysteresis_law'] &= used['hysteresis'] == 1-oldl
            if t == MOVE:
                hysteresis['latch_at_seam'] = oldl
            if newl != oldl:
                kind = 'set' if newl else 'return'
                hysteresis[kind+'_count'] += 1
                if hysteresis['first_'+kind] is None:
                    hysteresis['first_'+kind] = t
                if t < MOVE:
                    hysteresis[kind+'s_before_seam'] += 1
                elif newl and hysteresis['first_set_after_seam'] is None:
                    hysteresis['first_set_after_seam'] = t

            # This turn's candidate, recomputed here from the printed statistic.
            olds, news = float(r['s_before']), float(r['s_after'])
            oldc, newc = int(r['cusum_latch_before']), int(r['cusum_latch_after'])
            wants, wantc = olds, oldc
            if active:
                if matched >= 1:
                    raised = olds+(-(candidate-cold)-DRIFT)
                    wants = raised if raised > 0. else 0.
                if wants >= THRESHOLD:
                    wantc = 1
            elif activated:
                wants, wantc = 0., 0
            exact['cusum_statistic'] &= news == wants
            exact['cusum_one_way'] &= newc == wantc and newc >= oldc and news >= 0.
            if active:
                exact['cusum_chronology'] &= used['cusum'] == 1-oldc
            if t == MOVE:
                cusum['s_at_seam'] = olds
            if active and matched >= 1:
                cusum['voting_bytes'] += 1
                if t >= MOVE:
                    cusum['voting_bytes_after_seam'] += 1
            if news > cusum['s_max']:
                cusum['s_max'], cusum['s_max_byte'] = news, t
            if t < MOVE:
                cusum['s_max_before_seam'] = max(cusum['s_max_before_seam'], news)
            else:
                cusum['s_max_after_seam'] = max(cusum['s_max_after_seam'] or 0., news)
            if newc and not oldc:
                assert cusum['latch_byte'] is None
                cusum.update(latched=True, latch_byte=t, s_after_at_latch=news,
                             latch_before_seam=t < MOVE)
            cusum['s_final'] = news

            # turn24's O-rate-0 confinement, recomputed here.
            armed = int(r['orate0_armed'])
            assert armed == int(seam >= 0 and t >= seam)
            same = (r['orate0_live'] == r['slow_live'] and
                    r['orate0_odds_after'] == r['slow_odds_after'] and
                    used['orate0'] == used['slow'])
            if seam < 0:
                exact['orate0_off_law_is_slow'] &= same
            elif not armed:
                exact['orate0_pre_seam_is_slow'] &= same
            else:
                exact['orate0_armed_is_fast'] &= used['orate0'] != 1
            rows.append((t, truth, rank, matched, cold, candidate, live, used,
                         olds, news, oldc, newc))
    assert t+1 == N
    exact['normalized'] = 0 <= maxnorm <= 1e-8
    for a, s in arms.items():
        s['early'] = s['horizons'][str(EARLY)]
        assert s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL, (w, regime, a)
    shared = {s['activation'] for s in arms.values()}
    assert len(shared) == 1, (w, regime, shared)

    centre = cusum['latch_byte'] if cusum['latched'] else (MOVE if seam >= 0 else None)
    radius = 32 if cusum['latched'] and regime != SEAM_REGIME else 8
    window = []
    if centre is not None:
        for row in rows[max(0, centre-radius):min(N, centre+radius+1)]:
            t, truth, rank, matched, cold, candidate, live, used, olds, news, oldc, newc = row
            window.append(dict(t=t, truth=truth, rank=rank, matchedL=matched,
                               cold=cold, candidate=candidate, delta=candidate-cold,
                               cusum_live=live['cusum'], slow_live=live['slow'],
                               fast_live=live['fast'], cusum_slow_used=used['cusum'],
                               s_before=olds, s_after=news,
                               latch_before=oldc, latch_after=newc))
    return dict(world=w, regime=regime, seam=seam, arms=arms, cusum=cusum,
                hysteresis=hysteresis, exact=exact, admission=arms['slow']['activation'],
                max_norm_error=maxnorm, archive_bytes=(HERE/'memory'/f'world{w}'/
                                                       'episodes.bin').stat().st_size,
                source_sha256=digest(source), replay_sha256=digest(target),
                raw_window_centre=centre, raw_window=window)


def mean(values):
    return math.fsum(values)/len(values)


def arm_quantities(by, arm):
    law_tail = [by[w, SEAM_REGIME]['arms'][arm]['tail'] -
                by[w, SEAM_REGIME]['arms']['fast']['tail'] for w in WORLDS]
    law_whole = [by[w, SEAM_REGIME]['arms'][arm]['gain'] -
                 by[w, SEAM_REGIME]['arms']['fast']['gain'] for w in WORLDS]
    moved_fast = [by[w, 'moved_mid']['arms'][arm]['tail'] -
                  by[w, 'moved_mid']['arms']['fast']['tail'] for w in WORLDS]
    moved_slow = [by[w, 'moved_mid']['arms'][arm]['tail'] -
                  by[w, 'moved_mid']['arms']['slow']['tail'] for w in WORLDS]
    intact = [by[w, 'recombined']['arms'][arm]['gain'] for w in WORLDS]
    early = mean([by[w, 'recombined']['arms'][arm]['early'] for w in WORLDS])
    slow_early = mean([by[w, 'recombined']['arms']['slow']['early'] for w in WORLDS])
    slow_full = mean([by[w, 'recombined']['arms']['slow']['gain'] for w in WORLDS])
    return dict(
        law_tail_vs_fast=law_tail, law_tail_mean=mean(law_tail),
        law_tail_worlds_at_least_minus_one=sum(x >= -1 for x in law_tail),
        law_whole_vs_fast=law_whole, law_whole_mean=mean(law_whole),
        law_whole_worlds_at_least_zero=sum(x >= 0 for x in law_whole),
        moved_tail_vs_fast=moved_fast, moved_tail_mean=mean(moved_fast),
        moved_tail_wins=sum(x > 0 for x in moved_fast),
        moved_tail_vs_slow=moved_slow, moved_tail_vs_slow_mean=mean(moved_slow),
        intact_full_gains=intact, intact_early_mean=early, intact_full_mean=mean(intact),
        intact_full_positive=sum(x > 0 for x in intact),
        intact_early_retention=early/slow_early if slow_early > 0 else None,
        intact_full_retention=mean(intact)/slow_full if slow_full > 0 else None)


def inherited_bars(q):
    return dict(b1_law_tail_mean=q['law_tail_mean'] >= -1,
                b2_law_whole_mean=q['law_whole_mean'] >= 0,
                b3_moved_tail_mean=q['moved_tail_mean'] >= 1,
                b4_moved_tail_wins=q['moved_tail_wins'] >= 5,
                b5_moved_tail_vs_slow=q['moved_tail_vs_slow_mean'] >= -3,
                b6_intact_early_retention=q['intact_early_retention'] is not None and
                q['intact_early_retention'] >= .95,
                b7_intact_full_retention=q['intact_full_retention'] is not None and
                q['intact_full_retention'] >= .95,
                b8_intact_full_positive=q['intact_full_positive'] == len(WORLDS))


def gate(lives, quantities):
    by = {(x['world'], x['regime']): x for x in lives}
    q, inc = quantities[CANDIDATE], quantities[INCUMBENT]
    false_latches = [dict(world=x['world'], regime=x['regime'],
                          latch_byte=x['cusum']['latch_byte'],
                          s_max=x['cusum']['s_max'],
                          s_max_byte=x['cusum']['s_max_byte'])
                     for x in lives if x['regime'] != SEAM_REGIME and x['cusum']['latched']]
    switched = [by[w, SEAM_REGIME] for w in WORLDS]
    latched_in_time = [x['world'] for x in switched
                       if x['cusum']['latched'] and x['cusum']['latch_byte'] <= LATCH_DEADLINE]
    pre_seam = [x['world'] for x in switched if x['cusum']['latch_before_seam']]
    hygiene = dict(
        admission_identity=all(len({s['activation'] for s in x['arms'].values()}) == 1
                               for x in lives),
        admission_shared_with_episode=all(x['exact']['admission'] for x in lives),
        unrelated_admissions_reported=(
            len([x for x in lives if x['regime'] == 'unrelated']) == len(WORLDS) and
            all(set(x['arms']) == set(ARMS)
                for x in lives if x['regime'] == 'unrelated')),
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                   for x in lives for s in x['arms'].values()),
        distributions_positive_and_normalized=all(
            x['exact']['normalized'] and x['exact']['new_column'] for x in lives),
        exact_quotes=all(x['exact'][k] for x in lives
                         for k in ('inactive', 'equal_price', 'protected_new')),
        label_confined=all(x['exact'][k] for x in lives
                           for k in ('orate0_off_law_is_slow', 'orate0_pre_seam_is_slow',
                                     'orate0_armed_is_fast')),
        laws_recomputed=all(x['exact'][k] for x in lives
                            for k in ('witness_clock', 'hysteresis_law', 'cusum_statistic',
                                      'cusum_one_way', 'cusum_chronology')),
        archive=all(x['archive_bytes'] <= 528 for x in lives))
    details = dict(
        c1=dict(law_tail_mean=q['law_tail_mean'],
                law_tail_vs_fast=q['law_tail_vs_fast'],
                worlds_at_least_minus_one=q['law_tail_worlds_at_least_minus_one']),
        c2=dict(law_whole_mean=q['law_whole_mean'], law_whole_vs_fast=q['law_whole_vs_fast']),
        c3=dict(moved_tail_mean=q['moved_tail_mean'], moved_tail_wins=q['moved_tail_wins'],
                moved_tail_vs_slow_mean=q['moved_tail_vs_slow_mean'],
                moved_tail_vs_fast=q['moved_tail_vs_fast']),
        c4=dict(intact_early_retention=q['intact_early_retention'],
                intact_full_retention=q['intact_full_retention'],
                intact_full_positive=q['intact_full_positive'],
                intact_full_gains=q['intact_full_gains']),
        c5=dict(cusum_law_tail_mean=q['law_tail_mean'],
                hysteresis_law_tail_mean=inc['law_tail_mean'],
                margin=q['law_tail_mean']-inc['law_tail_mean']),
        c6=dict(false_latch_count=len(false_latches), false_latches=false_latches,
                switched_latch_bytes={str(x['world']): x['cusum']['latch_byte']
                                      for x in switched},
                switched_latched_by_deadline=len(latched_in_time),
                latch_deadline=LATCH_DEADLINE,
                switched_latched_before_seam=pre_seam,
                intact_s_max={f"{x['world']}/{x['regime']}": x['cusum']['s_max']
                              for x in lives if x['regime'] != SEAM_REGIME}),
        c7=hygiene,
        c8=dict(note='the independent reader has not run inside the writer'))
    tests = dict(
        c1_law_tail=q['law_tail_mean'] >= -1 and q['law_tail_worlds_at_least_minus_one'] >= 5,
        c2_law_whole=q['law_whole_mean'] >= 0,
        c3_moved=(q['moved_tail_mean'] >= 1 and q['moved_tail_wins'] >= 5 and
                  q['moved_tail_vs_slow_mean'] >= -3),
        c4_recombined=(q['intact_early_retention'] is not None and
                       q['intact_early_retention'] >= .95 and
                       q['intact_full_retention'] is not None and
                       q['intact_full_retention'] >= .95 and
                       q['intact_full_positive'] == len(WORLDS)),
        c5_incumbent=q['law_tail_mean']-inc['law_tail_mean'] > 1,
        c6_mechanism=not false_latches and len(latched_in_time) >= 6,
        c7_hygiene=all(hygiene.values()),
        c8_reader=False)
    return tests, details, hygiene, false_latches


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    quantities = {a: arm_quantities(by, a) for a in ARMS}
    bars = {a: inherited_bars(quantities[a]) for a in ARMS}
    tests, details, hygiene, false_latches = gate(lives, quantities)
    table = {r: {a: {f: mean([by[w, r]['arms'][a][f] for w in WORLDS])
                     for f in ('early', 'tail', 'gain')} for a in ARMS} for r in REGIMES}
    life_table = {r: {a: {str(w): by[w, r]['arms'][a]['gain'] for w in WORLDS}
                      for a in ARMS} for r in REGIMES}
    trajectories = {f"{x['world']}/{x['regime']}": x['cusum'] for x in lives}
    admissions = [dict(world=x['world'], regime=x['regime'], admission=x['admission'],
                       gains={a: x['arms'][a]['gain'] for a in ARMS})
                  for x in lives if x['regime'] == 'unrelated']
    fast_own_bound = all(x['arms']['fast']['drawdown'] <= 10+TOL for x in lives)
    return dict(quantities=quantities, bars=bars, tests=tests, test_details=details,
                hygiene=hygiene, false_latch_census=false_latches, table=table,
                life_table=life_table, cusum_trajectories=trajectories,
                unrelated_admissions=admissions, fast_own_drawdown_bound=fast_own_bound)


def pack_result(lives, declared, labels_path):
    summary = summarize(lives)
    return dict(namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
                candidate=CANDIDATE, incumbent=INCUMBENT, yardstick=YARDSTICK,
                seam=MOVE, seam_regime=SEAM_REGIME, drift=DRIFT, threshold=THRESHOLD,
                latch_deadline=LATCH_DEADLINE,
                protocol_sha256=digest(HERE/'PROTOCOL.md'),
                labels_sha256=digest(labels_path),
                labels=declared['worlds'], lives=lives, **summary,
                gate_pass=all(summary['tests'].values()),
                gate_pass_pending_reader=all(v for k, v in summary['tests'].items()
                                             if k != 'c8_reader'),
                independent_reader_pending=True)


def evaluate():
    for name in ('EXTRACT_MANIFEST.json', 'MEMORY_MANIFEST.json', 'LABELS_MANIFEST.json'):
        check_manifest(name)
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    declared = json.loads((HERE/'labels'/'GENERATOR_LABELS.json').read_text())
    assert declared['seam'] == MOVE and declared['law_changed_regime'] == SEAM_REGIME
    assert all(declared['worlds'][str(w)]['seam'] == MOVE for w in WORLDS)
    pairs = [(w, r) for w in WORLDS for r in REGIMES]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        list(pool.map(lambda wr: parent.t13.run_target(*wr), pairs))
    lives = []
    for w, r in pairs:
        life = evaluate_life(w, r)
        lives.append(life)
        print(w, r, 'cusum=%.3f' % life['arms']['cusum']['gain'],
              'latch=%s' % life['cusum']['latch_byte'],
              'smax=%.3f' % life['cusum']['s_max'], flush=True)
    result = pack_result(lives, declared, HERE/'labels'/'GENERATOR_LABELS.json')
    save(HERE/'RESULT.json', result)
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps({k: result[k] for k in
                      ('tests', 'test_details', 'hygiene', 'gate_pass',
                       'gate_pass_pending_reader')}, indent=2, sort_keys=True))


def equivalence():
    """The incumbent and the yardstick, arm by arm, against their own turns.

    turn25's binaries are gitignored and absent from this tree, so its replay is
    rebuilt from its FREEZE-pinned sources. turn24's oracle binary is retained at
    its own frozen sha and is run as it stands.
    """
    check_manifest('RESULTS_MANIFEST.json')
    t25 = HERE/'.build/turn25_replay'
    t24 = ROOT/'turns/turn24/oracle'
    assert t25.is_file(), 'make references'
    t24_frozen = json.loads((ROOT/'turns/turn24/FREEZE.json').read_text())['files']
    assert digest(t24) == t24_frozen['turns/turn24/oracle'], 'turn24 oracle drifted'
    t25_frozen = json.loads((ROOT/'turns/turn25/FREEZE.json').read_text())['files']
    sources = {name: digest(ROOT/name) for name in
               ('turns/turn25/replay.c', 'turns/turn25/latch.c', 'turns/turn25/latch.h',
                'turns/turn22/authority.c', 'turns/turn22/authority.h')}
    for name, got in sources.items():
        assert got == t25_frozen[name], name
    hyst = [('hysteresis_live', 'latch_live'), ('hysteresis_odds_before', 'latch_odds_before'),
            ('hysteresis_slow_used', 'latch_slow_used'),
            ('hysteresis_odds_after', 'latch_odds_after'),
            ('hysteresis_latch_before', 'latch_before'),
            ('hysteresis_latch_after', 'latch_after'),
            ('w_before', 'w_before'), ('w_after', 'w_after')]
    shared = [(a+'_'+f, a+'_'+f) for a in ('slow', 'fast', 'witness')
              for f in ('live', 'odds_before', 'slow_used', 'odds_after')]
    oracle = [(a, a) for a in ('orate0_live', 'orate0_odds_before',
                               'orate0_slow_used', 'orate0_odds_after')]
    report = {}
    for w, regime in EQUIVALENCE_LIVES:
        seam = MOVE if regime == SEAM_REGIME else -1
        with gzip.open(HERE/'results'/f'world{w}'/(regime+'.tsv.gz'), 'rt') as stream:
            payload = ''.join(' '.join((r['t'], r['logcold'], r['episode_candidate'],
                                        r['episode_matchedL']))+'\n'
                              for r in csv.DictReader(stream, delimiter='\t')).encode()
        mine_rows = []
        with gzip.open(HERE/'results/authority'/f'world{w}'/(regime+'.tsv.gz'), 'rt') as s:
            mine_rows = list(csv.DictReader(s, delimiter='\t'))
        runs = {}
        for name, binary, args in (('turn25', t25, ()), ('turn24', t24, (str(seam),))):
            done = subprocess.run([str(binary), *args], input=payload,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
            runs[name] = list(csv.DictReader(done.stdout.decode().splitlines(), delimiter='\t'))
        assert len(mine_rows) == len(runs['turn25']) == len(runs['turn24']) == N
        counts = {}
        for name, pairs in (('turn25_hysteresis', hyst), ('turn25_shared_controls', shared),
                            ('turn24_orate0', oracle)):
            other = runs['turn25' if name.startswith('turn25') else 'turn24']
            counts[name] = sum(mine_rows[t][a] != other[t][b]
                               for t in range(N) for a, b in pairs)
        report[f'world{w}/{regime}'] = dict(seam=seam, rows=N, mismatches=counts,
                                            columns_compared={k: len(v) for k, v in
                                                              (('turn25_hysteresis', hyst),
                                                               ('turn25_shared_controls', shared),
                                                               ('turn24_orate0', oracle))})
        print(w, regime, counts, flush=True)
    save(HERE/'EQUIVALENCE.json', dict(
        lives=report, turn24_oracle_sha256=digest(t24),
        turn24_oracle_frozen_sha256=t24_frozen['turns/turn24/oracle'],
        turn25_rebuilt_from=sources, rebuilt_binary_sha256=digest(t25),
        bitwise_equal=all(all(v == 0 for v in x['mismatches'].values())
                          for x in report.values()),
        scope='Printed-field bitwise comparison of this turn\'s hysteresis and orate0 arms '
              'against turn25\'s replay rebuilt from its frozen sources and turn24\'s '
              'retained oracle binary, on whole production lives.'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'learn',
                                          'labels', 'evaluate', 'equivalence'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    {'generate': generate, 'extract': extract, 'learn': learn, 'labels': labels,
     'evaluate': evaluate, 'equivalence': equivalence}[stage]()


if __name__ == '__main__':
    main()
