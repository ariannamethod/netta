#!/usr/bin/env python3
"""Reproduce the one prefreeze turn34 C-versus-independent-Decimal fixture."""
import argparse
from decimal import localcontext
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'FIXTURE_READER.json')
    args = parser.parse_args()
    if args.output.exists():
        raise FileExistsError(args.output)
    spec = importlib.util.spec_from_file_location(
        'independent_fixture34', HERE/'verify.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    run = subprocess.run([str(HERE/'sel4_router'), '--fixture'],
                         capture_output=True, text=True, check=True)
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    wealths, permissions, visits = {}, {}, {}
    error, checks = 0.0, 0
    witnessed = dict(first=False, silent=False, recovery=False, locality=False,
                     new=False, no_match=False, common=False)
    episode_positive = [False]*4
    episode_negative = [False]*4
    previous = {}

    def check(actual, expected, label):
        nonlocal error, checks
        error = max(error, reader.near(actual, expected, label))
        checks += 1

    with localcontext() as context:
        context.prec = 50
        for t, row in enumerate(rows):
            assert row['t'] == t
            record = row['record']
            matched = record >= 0
            if matched:
                wealths.setdefault(record, reader.SelWealth())
                permissions.setdefault(record, reader.Permission())
                visits.setdefault(record, 0)
                wealth = wealths[record]
                permission = permissions[record]
            else:
                wealth, permission = reader.SelWealth(), reader.Permission()
            cold, pooled = row['cold'], row['pooled']
            episodes = row['episodes']
            assert len(episodes) == 4
            assert row['visits_before'] == (visits[record] if matched else 0)

            before = (tuple(map(float, wealth.h)), float(permission.u))
            if matched and record in previous:
                witnessed['locality'] |= before == previous[record]
            for i in range(4):
                check(row['wealth_before'][i], wealth.h[i], f'{t}/wealth/{i}/before')
            check(row['permission_before'], permission.u, f'{t}/permission/before')

            corrected = wealth.source(pooled, episodes)
            sel_quote = permission.quote(cold, corrected)
            pooled_quote = permission.quote(cold, pooled)
            check(row['corrected'], corrected, f'{t}/corrected')
            check(row['sel4'], sel_quote, f'{t}/sel4')
            check(row['pooled_quote'], pooled_quote, f'{t}/pooled')

            silent = wealth.silent()
            if matched and visits[record] == 0:
                witnessed['first'] = True
                assert row['sel4'] == row['pooled_quote']
            if matched and silent:
                witnessed['silent'] = True
                assert row['sel4'] == row['pooled_quote']
            for i in range(4):
                episode_positive[i] |= wealth.h[i] > 0
                episode_negative[i] |= wealth.h[i] < 0
            witnessed['new'] |= row['rank'] == 0
            witnessed['no_match'] |= not matched
            witnessed['common'] |= matched and all(
                price == pooled == cold for price in episodes)

            old_active = not wealth.silent()
            if matched:
                wealth.observe(pooled, episodes)
                permission.observe(pooled, pooled_quote)
                visits[record] += 1
                previous[record] = (tuple(map(float, wealth.h)),
                                    float(permission.u))
            witnessed['recovery'] |= matched and not old_active and not wealth.silent()

            for i in range(4):
                check(row['wealth_after'][i], wealth.h[i], f'{t}/wealth/{i}/after')
            check(row['permission_after'], permission.u, f'{t}/permission/after')
            assert row['wealth_struct_bytes'] == 32
            assert 0 <= row['max_norm_error'] <= 1e-8

    assert len(rows) == 28 and sorted(wealths) == [0, 1]
    assert all(witnessed.values()), witnessed
    assert all(episode_positive), episode_positive
    assert all(episode_negative), episode_negative
    receipt = dict(
        events=len(rows), records=sorted(wealths), numeric_checks=checks,
        max_error=error, passed=True, witnessed=witnessed,
        episode_positive=episode_positive, episode_negative=episode_negative,
        reader_sha256=reader.digest(HERE/'verify.py'),
        fixture_sha256=hashlib.sha256(run.stdout.encode()).hexdigest(),
        binary_sha256=reader.digest(HERE/'sel4_router'),
        scope='One handcrafted full-byte fixture: independent Decimal episode '
              'wealths and pooled permission; first, silent, per-episode '
              'positive and negative, recovery, locality, NEW, no-match and '
              'exact common events. No generated worlds.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(receipt, sort_keys=True))


if __name__ == '__main__':
    main()
