#!/usr/bin/env python3
"""Every gate tooth turned red on its own, before a single world byte exists.

Three families, all on artificial records or a handcrafted tape, never on
protocol worlds: the six-arm C law against the reader's independent Decimal
recompute; the C1..C8 gate driven red one tooth at a time with the writer and
the reader agreeing on every verdict; and the reader's refusals.
"""
import copy
import csv
from decimal import localcontext
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
EXACT_KEYS = ('admission', 'inactive', 'equal_price', 'protected_new', 'new_column',
              'normalized', 'witness_clock', 'hysteresis_law', 'cusum_statistic',
              'cusum_one_way', 'cusum_chronology', 'orate0_off_law_is_slow',
              'orate0_pre_seam_is_slow', 'orate0_armed_is_fast')
GREEN = {('switched', 'fast'): (0., 0., 0.),
         ('switched', 'cusum'): (1., .5, 0.),
         ('switched', 'hysteresis'): (-3., -3., 0.),
         ('moved_mid', 'slow'): (0., 4., 0.),
         ('moved_mid', 'cusum'): (0., 2., 0.),
         ('recombined', 'slow'): (20., 0., 10.),
         ('recombined', 'cusum'): (20., 0., 10.)}


def load(name):
    spec = importlib.util.spec_from_file_location('preflight_'+name, HERE/(name+'.py'))
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def tape():
    """One handcrafted life, unrelated to any protocol world.

    64 rising bytes carry the shadow to admission; then a calm stretch that must
    not fire anything, a harmful stretch that must fire both latches, a healing
    stretch that must return the hysteresis flag and never the CUSUM one.
    """
    rows = [(-2.0, -1.5, 1)]*64
    rows += [(-2.0, -1.9, 1), (-2.0, -2.1, 1)]*20
    rows += [(-2.0, -2.0, 0), (-2.0, -2.0, 1)]
    rows += [(-1.0, -2.0, 1)]*30
    rows += [(-1.0, -2.0, 0)]*4
    rows += [(-3.0, -1.0, 1)]*40
    rows += [(-2.0, -1.9, 1)]*20
    return ''.join('%d %.17g %.17g %d\n' % (t, c, k, m)
                   for t, (c, k, m) in enumerate(rows)).encode()


def law_probe(writer, reader):
    payload = tape()
    events = payload.decode().count('\n')
    seen = {}
    worst = 0.
    for seam in (-1, 120):
        done = subprocess.run([str(HERE/'authority_replay'), str(seam)], input=payload,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        rows = list(csv.DictReader(done.stdout.decode().splitlines(), delimiter='\t'))
        assert len(rows) == events
        assert list(rows[0]) == reader.COLUMNS, 'C header differs from the reader schema'
        with localcontext() as ctx:
            ctx.prec = 50
            arms = {a: reader.Authority(a, seam if a == reader.YARDSTICK else None)
                    for a in writer.ARMS}
            marks = dict(cusum_latch=0, hysteresis_set=0, hysteresis_return=0,
                         non_voting=0, equal_price=0, armed=0, admissions=0)
            for t, row in enumerate(rows):
                cold, candidate = float(row['cold']), float(row['candidate'])
                matched = int(row['matched'])
                step = {a: arms[a].step(t, cold, candidate, matched) for a in writer.ARMS}
                for a in writer.ARMS:
                    for f in ('live', 'odds_before', 'slow_used', 'odds_after'):
                        worst = max(worst, reader.near(row[a+'_'+f], step[a][f], a+'/'+f))
                for f in ('shadow_before', 'active_before', 'admitted_after'):
                    worst = max(worst, reader.near(row[f], step['slow'][f], f))
                for f in ('w_before', 'w_after', 'hysteresis_latch_before',
                          'hysteresis_latch_after'):
                    worst = max(worst, reader.near(row[f], step[reader.INCUMBENT][f], f))
                for f in ('s_before', 's_after', 'cusum_latch_before', 'cusum_latch_after'):
                    worst = max(worst, reader.near(row[f], step[reader.CANDIDATE][f], f))
                worst = max(worst, reader.near(row['orate0_armed'],
                                               step[reader.YARDSTICK]['orate0_armed'], 'armed'))
                marks['cusum_latch'] += row['cusum_latch_after'] > row['cusum_latch_before']
                marks['hysteresis_set'] += row['hysteresis_latch_after'] > row['hysteresis_latch_before']
                marks['hysteresis_return'] += row['hysteresis_latch_after'] < row['hysteresis_latch_before']
                marks['non_voting'] += matched == 0
                marks['equal_price'] += candidate == cold
                marks['armed'] += int(row['orate0_armed'])
                marks['admissions'] += int(row['admitted_after'])
        seen[seam] = marks
    for seam, marks in seen.items():
        assert marks['admissions'] == 1, (seam, 'the tape must admit exactly once')
        assert marks['cusum_latch'] == 1, (seam, 'the tape must fire the CUSUM latch once')
        assert marks['hysteresis_set'] >= 1 and marks['hysteresis_return'] >= 1, \
            (seam, 'the tape must both set and release the hysteresis flag')
        assert marks['non_voting'] >= 1 and marks['equal_price'] >= 1, (seam, 'thin tape')
    assert seen[-1]['armed'] == 0 and seen[120]['armed'] == events-120
    return dict(events=events, maximum_numeric_error=worst,
                exercised={str(k): v for k, v in seen.items()})


def grid(writer, values):
    lives = []
    for w in writer.WORLDS:
        for r in writer.REGIMES:
            arms = {}
            for a in writer.ARMS:
                gain, tail, early = values.get((r, a), (0., 0., 0.))
                s = writer.empty_stats()
                s.update(gain=gain, tail=tail, early=early, activation=33,
                         horizons={'1024': 0., '4096': early, '8192': 0., '16384': gain})
                arms[a] = s
            c = writer.empty_cusum()
            c.update(s_at_seam=.2, s_final=.3, s_max=.9, s_max_byte=100,
                     s_max_before_seam=.9, s_max_after_seam=.4,
                     voting_bytes=500, voting_bytes_after_seam=250)
            if r == writer.SEAM_REGIME:
                c.update(latched=True, latch_byte=8300, latch_before_seam=False,
                         s_final=9., s_max=9., s_max_byte=8300, s_max_after_seam=9.,
                         s_after_at_latch=8.1)
            lives.append(dict(
                world=w, regime=r, seam=writer.MOVE if r == writer.SEAM_REGIME else -1,
                arms=arms, cusum=c, hysteresis=writer.empty_hysteresis(),
                exact=dict.fromkeys(EXACT_KEYS, True), admission=33, max_norm_error=0.,
                archive_bytes=528, source_sha256='0'*64, replay_sha256='0'*64,
                raw_window_centre=None, raw_window=[]))
    return lives


def find(lives, world, regime):
    return next(x for x in lives if x['world'] == world and x['regime'] == regime)


def redden(writer, lives, tooth):
    lives = copy.deepcopy(lives)
    first = writer.WORLDS[0]
    if tooth == 'c1_law_tail':
        for x in lives:
            if x['regime'] == writer.SEAM_REGIME:
                x['arms']['cusum']['tail'] = -1.5
    elif tooth == 'c2_law_whole':
        for x in lives:
            if x['regime'] == writer.SEAM_REGIME:
                x['arms']['cusum']['gain'] = -1.
    elif tooth == 'c3_moved':
        for x in lives:
            if x['regime'] == 'moved_mid':
                x['arms']['cusum']['tail'] = .5
    elif tooth == 'c4_recombined':
        find(lives, first, 'recombined')['arms']['cusum']['gain'] = -1.
    elif tooth == 'c5_incumbent':
        for x in lives:
            if x['regime'] == writer.SEAM_REGIME:
                x['arms']['hysteresis']['tail'] = 0.
    elif tooth == 'c6_mechanism':
        life = find(lives, first, 'recombined')
        life['cusum'].update(latched=True, latch_byte=100, latch_before_seam=True,
                             s_after_at_latch=8.2, s_max=8.2)
    elif tooth == 'c6_mechanism_late':
        for w in writer.WORLDS[:3]:
            find(lives, w, writer.SEAM_REGIME)['cusum']['latch_byte'] = 9000
    elif tooth == 'c7_hygiene':
        find(lives, first, 'recombined')['exact']['cusum_statistic'] = False
    else:
        raise AssertionError('unknown tooth '+tooth)
    return lives


def gate_probes(writer, reader):
    green = grid(writer, GREEN)
    base = writer.summarize(green)['tests']
    mirror = reader.rebuild(green)['tests']
    gated = list(reader.GATED_TESTS)
    assert all(base[k] for k in gated), 'the green counter-probe is not green: '+str(base)
    assert base['c8_reader'] is False and mirror['c8_reader'] is True
    assert all(base[k] == mirror[k] for k in gated), 'writer and reader disagree when green'
    report = {'green': base}
    for tooth in gated+['c6_mechanism_late']:
        target = 'c6_mechanism' if tooth.endswith('_late') else tooth
        red = redden(writer, green, tooth)
        got = writer.summarize(red)['tests']
        echo = reader.rebuild(red)['tests']
        assert got[target] is False, tooth+' did not turn red: '+str(got)
        assert echo[target] is False, tooth+' stayed green for the reader: '+str(echo)
        assert all(got[k] == echo[k] for k in gated), 'writer and reader disagree on '+tooth
        collateral = sorted(k for k in gated if got[k] != base[k] and k != target)
        report[tooth] = dict(tests=got, target=target, also_flipped=collateral)
        print('red', tooth, '->', target, 'False; collateral', collateral, flush=True)
    return report


def refusals(writer, reader):
    stub = HERE/'.build'/'preflight_labels.json'
    stub.parent.mkdir(parents=True, exist_ok=True)
    stub.write_text(json.dumps({'worlds': {str(w): dict(world=w, seam=writer.MOVE)
                                           for w in writer.WORLDS}}))
    declared = json.loads(stub.read_text())
    result = json.loads(json.dumps(writer.pack_result(grid(writer, GREEN), declared, stub)))
    accepted = reader.check_schema(result)
    assert set(accepted) == set(reader.REQUIRED_RESULT_KEYS)
    missing = {}
    for key in sorted(reader.REQUIRED_RESULT_KEYS):
        short = copy.deepcopy(result)
        del short[key]
        try:
            reader.check_schema(short)
        except reader.Refusal as error:
            assert key in str(error), (key, str(error))
            missing[key] = str(error)
        else:
            raise AssertionError('a RESULT without '+key+' was accepted')
    assert result['tests']['c8_reader'] is False and result['gate_pass'] is False
    assert result['gate_pass_pending_reader'] is True
    with tempfile.TemporaryDirectory(prefix='preflight-', dir=HERE/'.build') as folder:
        probe = Path(folder)/'probe.txt'
        probe.write_text('original\n')
        wanted = writer.digest(probe)
        reader.check_artifact(probe, wanted)
        probe.write_text('changed\n')
        try:
            reader.check_artifact(probe, wanted)
        except reader.Refusal as error:
            named = str(error)
            assert str(probe) in named, named
        else:
            raise AssertionError('a changed artifact was accepted')
        occupied = subprocess.run([sys.executable, '-B', str(HERE/'verify.py'),
                                   '--output', str(probe)], capture_output=True, text=True)
        assert occupied.returncode == 1 and str(probe) in occupied.stderr
    print('refusal by name:', named)
    print('refusal on existing output:', occupied.stderr.strip())
    return dict(schema_accepted=accepted, missing_key_refusals=missing,
                changed_artifact_refused=named,
                existing_output_refused=occupied.stderr.strip(),
                existing_output_rc=occupied.returncode)


def main():
    for name in ('FREEZE.json', 'RESULT.json', 'data', 'memory', 'labels', 'results'):
        assert not (HERE/name).exists(), name
    writer, reader = load('experiment'), load('verify')
    assert writer.DRIFT == reader.DRIFT == 0.5 and writer.THRESHOLD == reader.THRESHOLD == 8.0
    assert writer.ARMS == reader.ARMS and writer.WORLDS == reader.WORLDS
    receipt = dict(scope='Handcrafted tape and artificial gate records only; no world bytes.',
                   law=law_probe(writer, reader),
                   gate=gate_probes(writer, reader),
                   refusals=refusals(writer, reader),
                   protocol_sha256=writer.digest(HERE/'PROTOCOL.md'),
                   writer_sha256=writer.digest(HERE/'experiment.py'),
                   reader_sha256=writer.digest(HERE/'verify.py'),
                   replay_sha256=writer.digest(HERE/'authority_replay'))
    writer.save(HERE/'PREFLIGHT.json', receipt)
    print(json.dumps(dict(law=receipt['law'],
                          teeth=sorted(k for k in receipt['gate'] if k != 'green')),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
