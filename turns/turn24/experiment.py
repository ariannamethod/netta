#!/usr/bin/env python3
"""Turn24: one frozen batch, six replayed reference arms and eight non-causal
oracle arms on one identical candidate/P0 tape and one shared admission."""
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
WORLDS = tuple(range(240, 248))
REGIMES = ('recombined', 'switched', 'moved_mid', 'unrelated')
NAMESPACE = 'netta-oracle-latency-v1'
N = 16384
MOVE = 8192
SEAM_REGIME = 'switched'
DELAYS = (0, 32, 128, 512)
CONTROL_ARMS = ('slow', 'fast', 'witness', 'h8l4', 'selfnorm')
REFERENCE_ARMS = CONTROL_ARMS + ('split',)
ORACLE_ARMS = tuple(f'orate{d}' for d in DELAYS) + tuple(f'olevel{d}' for d in DELAYS)
ARMS = REFERENCE_ARMS + ORACLE_ARMS
CAUSAL_NOMINEES = ('witness', 'h8l4', 'selfnorm', 'split')
LAW_SIDE_BARS = ('b1_law_tail_mean', 'b2_law_whole_mean')
HORIZONS = (1024, 4096, MOVE, N)
TOL = 1e-7
WITNESS_MEDIAN_BYTES = 38.5  # turn18, Astra's corrected median; context only.

FROZEN = (
    'turns/turn24/PROTOCOL.md', 'turns/turn24/Makefile', 'turns/turn24/oracle.c',
    'turns/turn24/experiment.py', 'turns/turn24/verify.py',
    'turns/turn24/episode', 'turns/turn24/authority_replay',
    'turns/turn24/split', 'turns/turn24/oracle',
    'turns/turn23/PROTOCOL.md', 'turns/turn23/split.c', 'turns/turn23/experiment.py',
    'turns/turn23/verify.py',
    'turns/turn22/PROTOCOL.md', 'turns/turn22/experiment.py', 'turns/turn22/verify.py',
    'turns/turn22/authority.c', 'turns/turn22/authority.h', 'turns/turn22/replay.c',
    'turns/turn21/authority.c', 'turns/turn21/authority.h',
    'turns/turn21/authority_replay', 'turns/turn21/episode',
    'turns/turn18/authority.c', 'turns/turn18/authority.h',
    'turns/turn18/authority_replay', 'turns/turn18/episode',
    'turns/turn15/experiment.py',
    'turns/turn13/episode.c', 'turns/turn13/experiment.py', 'turns/turn13/verify.py',
    'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
    'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
    'court4/transfer4_confirm_core.c',
)

spec = importlib.util.spec_from_file_location('turn24_parent_turn22',
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
         arms=ARMS, delays=DELAYS, seam=MOVE, seam_regime=SEAM_REGIME,
         files={name: digest(ROOT/name) for name in FROZEN}))
    (HERE/'INHERITED_SHA256.txt').write_text(''.join(
        f'{digest(ROOT/name)}  {name}\n' for name in FROZEN))


def check_freeze():
    """The pre-data pin, plus any recorded amendment.

    FREEZE.json is never rewritten. A file repaired after the batch existed is
    listed in FREEZE_AMENDMENT.json with both shas, and is accepted at exactly
    those two states and no third one.
    """
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    assert frozen['namespace'] == NAMESPACE and frozen['worlds'] == list(WORLDS)
    assert frozen['arms'] == list(ARMS) and frozen['delays'] == list(DELAYS)
    assert frozen['seam'] == MOVE and frozen['seam_regime'] == SEAM_REGIME
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
    """The generator's hidden law label. Consumed by the oracle arms only.

    The seam is constructional, not searched: turn13's generator builds the
    switched life from the recombined command prefix and changes the command
    law at N//2, so the emitted prefix is shared and the tail is its own.
    Both facts are verified here on the bytes the predictor actually sees,
    and the command-tape construction is checked as well.
    """
    check_manifest('EXTRACT_MANIFEST.json')
    rows = {}
    for w in WORLDS:
        folder = HERE/'data'/f'world{w}'
        raw = {r: (folder/(r+'.bin')).read_bytes() for r in REGIMES}
        # moved_mid carries no command tape: turn22's generator makes it by
        # applying a surface bijection to recombined.bin, not by emission.
        cmd = {r: (folder/(r+'.commands.bin')).read_bytes()
               for r in ('recombined', SEAM_REGIME, 'unrelated')}
        assert all(len(b) == N for b in raw.values()), w
        assert all(len(b) == N for b in cmd.values()), w
        prefix = raw[SEAM_REGIME][:MOVE] == raw['recombined'][:MOVE]
        differing = sum(a != b for a, b in
                        zip(raw[SEAM_REGIME][MOVE:], raw['recombined'][MOVE:]))
        cmd_prefix = cmd[SEAM_REGIME][:MOVE] == cmd['recombined'][:MOVE]
        cmd_tail = cmd[SEAM_REGIME][MOVE:] == cmd['unrelated'][MOVE:]
        tail_vs_unrelated = sum(a != b for a, b in
                                zip(raw[SEAM_REGIME][MOVE:], raw['unrelated'][MOVE:]))
        assert prefix and cmd_prefix and cmd_tail, w
        rows[str(w)] = dict(
            world=w, seam=MOVE, law_changed_regime=SEAM_REGIME,
            raw_prefix_identical_to_recombined=prefix,
            raw_tail_differing_bytes=differing,
            raw_tail_divergence=differing/(N-MOVE),
            raw_tail_differing_bytes_vs_unrelated=tail_vs_unrelated,
            command_prefix_identical_to_recombined=cmd_prefix,
            command_tail_identical_to_unrelated=cmd_tail)
    save(HERE/'labels'/'ORACLE_LABELS.json', dict(
        namespace=NAMESPACE, seam=MOVE, law_changed_regime=SEAM_REGIME,
        delays=DELAYS, worlds=rows,
        derivation='constructional: turns/turn13/experiment.py generate_world builds '
                   "commands['switched'] = commands['recombined'][:N//2] + "
                   "commands['unrelated'][N//2:], so the emitted prefix is shared "
                   'with recombined and the law changes at N//2 = 8192',
        scope='hidden generator label; read by the oracle arms only, never by a '
              'reference arm and never by the predictor'))
    save(HERE/'LABELS_MANIFEST.json', manifest(HERE/'labels'))
    print(json.dumps(rows, indent=2))


def empty_stats():
    return dict(gain=0., early=None, tail=0., minimum=0., peak=0., drawdown=0.,
                activation=None, horizons={}, slow_hazard_bytes=0,
                fast_hazard_bytes=0, tail_fast_hazard_bytes=0)


def seam_anatomy():
    return dict(switch_byte=None, odds_at_seam=None, odds_carried_into_switch=None,
                odds_before_switch=None, odds_after_switch=None, clamped=None,
                clamp_value=None, window_gain=0., post_switch_gain=0.,
                first_fast_hazard_byte=None)


def run_arm(binary, payload, target, args=()):
    done = subprocess.run([str(binary), *args], input=payload,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    target.parent.mkdir(parents=True, exist_ok=True)
    with gzip.open(target, 'xb') as stream:
        stream.write(done.stdout)
    target.with_suffix('.log').write_bytes(done.stderr)
    return digest(target)


def evaluate_life(w, regime, inherited):
    source = HERE/'results'/f'world{w}'/(regime+'.tsv.gz')
    tape = []
    with gzip.open(source, 'rt') as stream:
        for t, r in enumerate(csv.DictReader(stream, delimiter='\t')):
            assert int(r['t']) == t
            tape.append((r['t'], r['logcold'], r['episode_candidate'],
                         r['episode_matchedL'], int(r['rank']), int(r['truth']),
                         int(r['episode_active_before']),
                         int(r['episode_activated_after']), r['episode_live'],
                         float(r['max_norm_error']), r['new_exact']))
    assert len(tape) == N
    payload = ''.join(' '.join(row[:4])+'\n' for row in tape).encode()
    seam = MOVE if regime == SEAM_REGIME else -1
    folder = HERE/'results/policies'/f'world{w}'
    shas = dict(
        controls=run_arm(HERE/'authority_replay', payload, folder/(regime+'-controls.tsv.gz')),
        split=run_arm(HERE/'split', payload, folder/(regime+'-split.tsv.gz')),
        oracle=run_arm(HERE/'oracle', payload, folder/(regime+'-oracle.tsv.gz'),
                       (str(seam),)))

    summ = {a: empty_stats() for a in ARMS}
    anatomy = {a: seam_anatomy() for a in ORACLE_ARMS}
    exact = dict(inactive=True, equal_price=True, protected_new=True,
                 admission=True, normalized=True, new_column=True,
                 oracle_matches_slow_off_law=True, level_matches_fast_after_switch=True,
                 pre_switch_matches_slow=True)
    windows = {a: [] for a in ORACLE_ARMS}
    seam_odds = None
    maxnorm = 0.
    with gzip.open(folder/(regime+'-controls.tsv.gz'), 'rt') as cf, \
         gzip.open(folder/(regime+'-split.tsv.gz'), 'rt') as sf, \
         gzip.open(folder/(regime+'-oracle.tsv.gz'), 'rt') as of:
        streams = zip(tape, csv.DictReader(cf, delimiter='\t'),
                      csv.DictReader(sf, delimiter='\t'),
                      csv.DictReader(of, delimiter='\t'))
        for t, (row, c, s, o) in enumerate(streams):
            (st, scold, scand, smatch, rank, truth, active,
             activated, elive, norm, newcol) = row
            assert int(st) == int(c['t']) == int(s['t']) == int(o['t']) == t
            cold, candidate = float(scold), float(scand)
            matched = int(smatch)
            maxnorm = max(maxnorm, norm)
            exact['new_column'] &= newcol == '1'
            for other in (c, s, o):
                assert float(other['cold']) == cold
                assert float(other['candidate']) == candidate
                assert int(other['matched']) == matched
                assert int(other['active_before']) == active
                exact['admission'] &= int(other['admitted_after']) == activated
            live = {a: float(c[a+'_live']) for a in CONTROL_ARMS}
            live['split'] = float(s['live'])
            live.update({a: float(o[a+'_live']) for a in ORACLE_ARMS})
            assert abs(live['slow']-float(elive)) <= TOL
            for a, value in live.items():
                assert math.isfinite(value) and value <= 1e-8
                exact['inactive'] &= bool(active or value == cold)
                exact['equal_price'] &= bool(candidate != cold or value == cold)
                exact['protected_new'] &= bool(rank != 0 or value == cold)
                st_a = summ[a]
                st_a['gain'] += value-cold
                if t >= MOVE:
                    st_a['tail'] += value-cold
                st_a['minimum'] = min(st_a['minimum'], st_a['gain'])
                st_a['peak'] = max(st_a['peak'], st_a['gain'])
                st_a['drawdown'] = max(st_a['drawdown'], st_a['peak']-st_a['gain'])
                if t+1 in HORIZONS:
                    st_a['horizons'][str(t+1)] = st_a['gain']
                if activated:
                    assert st_a['activation'] is None
                    st_a['activation'] = t+1
            for a in CONTROL_ARMS:
                used = int(c[a+'_slow_used'])
                summ[a]['slow_hazard_bytes'] += used == 1
                summ[a]['fast_hazard_bytes'] += used == 0
                summ[a]['tail_fast_hazard_bytes'] += used == 0 and t >= MOVE
            for lane in ('short_hazard', 'long_hazard'):
                fast_lane = float(s[lane]) == 2.0**-10
                summ['split']['fast_hazard_bytes'] += fast_lane
                summ['split']['slow_hazard_bytes'] += float(s[lane]) == 2.0**-16
                summ['split']['tail_fast_hazard_bytes'] += fast_lane and t >= MOVE
            for a in ORACLE_ARMS:
                used = int(o[a+'_slow_used'])
                summ[a]['slow_hazard_bytes'] += used == 1
                summ[a]['fast_hazard_bytes'] += used == 0
                summ[a]['tail_fast_hazard_bytes'] += used == 0 and t >= MOVE
                delay = int(a.replace('orate', '').replace('olevel', ''))
                armed = seam >= 0 and t >= seam+delay
                if seam < 0:
                    exact['oracle_matches_slow_off_law'] &= (
                        o[a+'_live'] == o['slow_live'] and
                        o[a+'_odds_after'] == o['slow_odds_after'])
                elif not armed:
                    exact['pre_switch_matches_slow'] &= (
                        o[a+'_live'] == o['slow_live'] and
                        o[a+'_odds_after'] == o['slow_odds_after'])
                elif a.startswith('olevel'):
                    exact['level_matches_fast_after_switch'] &= (
                        o[a+'_live'] == o['fast_live'] and
                        o[a+'_odds_after'] == o['fast_odds_after'])
                if armed:
                    an = anatomy[a]
                    if t == seam+delay:
                        an.update(switch_byte=t,
                                  odds_carried_into_switch=float(o[a+'_odds_carried']),
                                  odds_before_switch=float(o[a+'_odds_before']),
                                  odds_after_switch=float(o[a+'_odds_after']),
                                  clamped=bool(int(o[a+'_clamped'])),
                                  clamp_value=float(o[a+'_odds_before'])
                                  if int(o[a+'_clamped']) else None)
                        assert not int(o[a+'_clamped']) or \
                            o[a+'_odds_before'] == o['fast_odds_before']
                    an['post_switch_gain'] += live[a]-cold
                    if an['first_fast_hazard_byte'] is None and used == 0:
                        an['first_fast_hazard_byte'] = t
                elif seam >= 0 and t >= seam:
                    anatomy[a]['window_gain'] += live[a]-cold
                if seam >= 0 and abs(t-(seam+delay)) <= 3:
                    windows[a].append(dict(
                        t=t, truth=truth, rank=rank, matched=matched, cold=cold,
                        candidate=candidate, slow=live['slow'], fast=live['fast'],
                        arm=live[a], clamped=int(o[a+'_clamped']),
                        odds_carried=float(o[a+'_odds_carried']),
                        odds_before=float(o[a+'_odds_before']),
                        slow_used=used))
            if seam >= 0 and t == seam:
                seam_odds = dict(slow=float(o['slow_odds_before']),
                                 fast=float(o['fast_odds_before']))
                for a in ORACLE_ARMS:
                    anatomy[a]['odds_at_seam'] = float(o[a+'_odds_carried'])
    assert t+1 == N
    exact['normalized'] = 0 <= maxnorm <= 1e-8
    for a, st_a in summ.items():
        st_a['early'] = st_a['horizons']['4096']
        assert st_a['early'] is not None
        assert st_a['minimum'] >= -1-TOL and st_a['drawdown'] <= 16+TOL, (w, regime, a)
    shared = {st_a['activation'] for st_a in summ.values()}
    assert len(shared) == 1, (w, regime, shared)
    assert summ['slow']['activation'] == inherited['arms']['episode']['activation']
    return dict(world=w, regime=regime, seam=seam, arms=summ, exact=exact,
                reference_odds_at_seam=seam_odds,
                anatomy=anatomy if seam >= 0 else None,
                raw_windows=windows if seam >= 0 else None,
                admission=summ['slow']['activation'],
                max_norm_error=maxnorm, source_sha256=digest(source),
                policy_sha256=shas,
                archive_bytes=(HERE/'memory'/f'world{w}'/'episodes.bin').stat().st_size)


def mean(values):
    return math.fsum(values)/len(values)


def arm_bars(by, arm):
    law_tail = [by[w, 'switched']['arms'][arm]['tail'] -
                by[w, 'switched']['arms']['fast']['tail'] for w in WORLDS]
    law_full = [by[w, 'switched']['arms'][arm]['gain'] -
                by[w, 'switched']['arms']['fast']['gain'] for w in WORLDS]
    moved_fast = [by[w, 'moved_mid']['arms'][arm]['tail'] -
                  by[w, 'moved_mid']['arms']['fast']['tail'] for w in WORLDS]
    moved_slow = [by[w, 'moved_mid']['arms'][arm]['tail'] -
                  by[w, 'moved_mid']['arms']['slow']['tail'] for w in WORLDS]
    intact_full = [by[w, 'recombined']['arms'][arm]['gain'] for w in WORLDS]
    early = mean([by[w, 'recombined']['arms'][arm]['early'] for w in WORLDS])
    full = mean(intact_full)
    slow_early = mean([by[w, 'recombined']['arms']['slow']['early'] for w in WORLDS])
    slow_full = mean([by[w, 'recombined']['arms']['slow']['gain'] for w in WORLDS])
    bars = dict(
        b1_law_tail_mean=mean(law_tail) >= -1,
        b2_law_whole_mean=mean(law_full) >= 0,
        b3_moved_tail_mean=mean(moved_fast) >= 1,
        b4_moved_tail_wins=sum(x > 0 for x in moved_fast) >= 5,
        b5_moved_tail_vs_slow=mean(moved_slow) >= -3,
        b6_intact_early_retention=slow_early > 0 and early/slow_early >= .95,
        b7_intact_full_retention=slow_full > 0 and full/slow_full >= .95,
        b8_intact_full_positive=all(x > 0 for x in intact_full))
    quantities = dict(
        law_tail_vs_fast=law_tail, law_tail_mean=mean(law_tail),
        law_whole_vs_fast=law_full, law_whole_mean=mean(law_full),
        law_tail_worlds_at_least_minus_one=sum(x >= -1 for x in law_tail),
        moved_tail_vs_fast=moved_fast, moved_tail_mean=mean(moved_fast),
        moved_tail_wins=sum(x > 0 for x in moved_fast),
        moved_tail_vs_slow=moved_slow, moved_tail_vs_slow_mean=mean(moved_slow),
        intact_early_mean=early, intact_full_mean=full,
        intact_early_retention=early/slow_early if slow_early > 0 else None,
        intact_full_retention=full/slow_full if slow_full > 0 else None,
        intact_full_gains=intact_full)
    return bars, quantities


def summarize(lives):
    by = {(x['world'], x['regime']): x for x in lives}
    bars, quantities = {}, {}
    for arm in ARMS:
        bars[arm], quantities[arm] = arm_bars(by, arm)
    cleared = {arm: sum(bars[arm].values()) for arm in ARMS}
    failed = {arm: sorted(k for k, v in bars[arm].items() if not v) for arm in ARMS}

    clearing = [d for d in DELAYS if all(bars[f'olevel{d}'].values())]
    dstar = max(clearing) if clearing else None
    rate_clearing = [d for d in DELAYS if all(bars[f'orate{d}'].values())]

    w3_bars = [b for b in LAW_SIDE_BARS if bars['olevel0'][b] and not bars['orate0'][b]]
    admissions = []
    for x in lives:
        if x['regime'] == 'unrelated' and x['admission'] is not None:
            admissions.append(dict(world=x['world'], regime=x['regime'],
                                   admission=x['admission'],
                                   gains={a: x['arms'][a]['gain'] for a in ARMS}))
    hygiene = dict(
        admission_identity=all(len({s['activation'] for s in x['arms'].values()}) == 1
                               for x in lives),
        admission_shared_with_episode=all(
            x['exact']['admission'] for x in lives),
        bounds=all(s['minimum'] >= -1-TOL and s['drawdown'] <= 16+TOL
                   for x in lives for s in x['arms'].values()),
        fast_own_bound=all(x['arms']['fast']['drawdown'] <= 10+TOL for x in lives),
        normalized=all(x['exact']['normalized'] and x['exact']['new_column']
                       for x in lives),
        exact_quotes=all(x['exact'][k] for x in lives
                         for k in ('inactive', 'equal_price', 'protected_new')),
        label_confined=all(x['exact'][k] for x in lives
                           for k in ('oracle_matches_slow_off_law',
                                     'pre_switch_matches_slow',
                                     'level_matches_fast_after_switch')),
        archive=all(x['archive_bytes'] <= 528 for x in lives))
    tests = dict(
        W1_satisfiability=all(bars['olevel0'].values()),
        W2_budget=dstar is not None and dstar >= 32,
        W3_rate_vs_level=bool(w3_bars),
        W4_references_stay_themselves=all(not all(bars[a].values())
                                          for a in CAUSAL_NOMINEES),
        W5_hygiene=all(hygiene.values()),
        W6_reader=False)
    curve = dict(
        olevel={str(d): dict(bars_cleared=cleared[f'olevel{d}'],
                             failed_bars=failed[f'olevel{d}'],
                             law_tail_mean=quantities[f'olevel{d}']['law_tail_mean'],
                             law_whole_mean=quantities[f'olevel{d}']['law_whole_mean'])
                for d in DELAYS},
        orate={str(d): dict(bars_cleared=cleared[f'orate{d}'],
                            failed_bars=failed[f'orate{d}'],
                            law_tail_mean=quantities[f'orate{d}']['law_tail_mean'],
                            law_whole_mean=quantities[f'orate{d}']['law_whole_mean'])
               for d in DELAYS},
        d_star=dstar, d_star_rate=max(rate_clearing) if rate_clearing else None,
        witness_median_detection_bytes=WITNESS_MEDIAN_BYTES,
        budget_beats_measured_detection=(dstar is not None and
                                         dstar >= WITNESS_MEDIAN_BYTES))
    table = {r: {a: {f: mean([by[w, r]['arms'][a][f] for w in WORLDS])
                     for f in ('early', 'tail', 'gain')} for a in ARMS}
             for r in REGIMES}
    return dict(bars=bars, quantities=quantities, bars_cleared=cleared,
                failed_bars=failed, curve=curve, hygiene=hygiene,
                unrelated_admissions=admissions, w3_law_bars_separating=w3_bars,
                tests=tests, table=table)


def evaluate():
    check_manifest('EXTRACT_MANIFEST.json')
    check_manifest('MEMORY_MANIFEST.json')
    check_manifest('LABELS_MANIFEST.json')
    assert not (HERE/'results').exists(), 'results exist; preserve evidence'
    declared = json.loads((HERE/'labels'/'ORACLE_LABELS.json').read_text())
    assert declared['seam'] == MOVE and declared['law_changed_regime'] == SEAM_REGIME
    assert declared['delays'] == list(DELAYS)
    assert all(declared['worlds'][str(w)]['seam'] == MOVE for w in WORLDS)
    pairs = [(w, r) for w in WORLDS for r in REGIMES]
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        inherited = list(pool.map(lambda wr: parent.t13.run_target(*wr), pairs))
    lives = []
    for item in inherited:
        life = evaluate_life(item['world'], item['regime'], item)
        lives.append(life)
        print(item['world'], item['regime'],
              'slow=%.3f' % life['arms']['slow']['gain'],
              'olevel0=%.3f' % life['arms']['olevel0']['gain'], flush=True)
    summary = summarize(lives)
    save(HERE/'RESULT.json', dict(
        namespace=NAMESPACE, worlds=WORLDS, regimes=REGIMES, arms=ARMS,
        delays=DELAYS, seam=MOVE, seam_regime=SEAM_REGIME,
        protocol_sha256=digest(HERE/'PROTOCOL.md'),
        labels_sha256=digest(HERE/'labels'/'ORACLE_LABELS.json'),
        labels=declared['worlds'], lives=lives, **summary,
        gate_pass=all(summary['tests'].values()),
        gate_pass_pending_reader=all(v for k, v in summary['tests'].items()
                                     if k != 'W6_reader'),
        independent_reader_pending=True))
    save(HERE/'RESULTS_MANIFEST.json', manifest(HERE/'results'))
    print(json.dumps(dict(tests=summary['tests'], curve=summary['curve'],
                          bars_cleared=summary['bars_cleared'],
                          failed_bars=summary['failed_bars'],
                          hygiene=summary['hygiene']), indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('stage', choices=('freeze', 'generate', 'extract', 'learn',
                                          'labels', 'evaluate'))
    stage = parser.parse_args().stage
    if stage == 'freeze':
        freeze()
        return
    check_freeze()
    {'generate': generate, 'extract': extract, 'learn': learn,
     'labels': labels, 'evaluate': evaluate}[stage]()


if __name__ == '__main__':
    main()
