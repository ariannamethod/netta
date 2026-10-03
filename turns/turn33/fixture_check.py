#!/usr/bin/env python3
"""Reproduce the one prefreeze turn33 C-versus-independent-Decimal fixture."""
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
        'independent_fixture33', HERE/'verify.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    run = subprocess.run([str(HERE/'earned_router'), '--fixture'],
                         capture_output=True, text=True, check=True)
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    wealths, permissions, flats, visits = {}, {}, {}, {}
    error, checks = 0.0, 0
    witnessed = dict(first=False, silent=False, positive=False, negative=False,
                     recovery=False, locality=False, new=False, no_match=False,
                     common=False)
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
                wealths.setdefault(record, reader.Wealth())
                permissions.setdefault(record, reader.Permission())
                flats.setdefault(record, reader.Flat())
                visits.setdefault(record, 0)
                wealth = wealths[record]
                permission = permissions[record]
                flat = flats[record]
            else:
                wealth, permission, flat = (
                    reader.Wealth(), reader.Permission(), reader.Flat())
            cold, a, b, pooled = [
                row[key] for key in ('cold', 'a', 'b', 'pooled')]
            assert row['visits_before'] == (visits[record] if matched else 0)

            before = (float(wealth.a), float(wealth.b), float(permission.u),
                      tuple(map(float, flat.weights)))
            if matched and record in previous:
                witnessed['locality'] |= before == previous[record]
            check(row['wealth_before'][0], wealth.a, f'{t}/wealth/a/before')
            check(row['wealth_before'][1], wealth.b, f'{t}/wealth/b/before')
            check(row['permission_before'], permission.u, f'{t}/permission/before')
            for index, value in enumerate(flat.weights):
                check(row['flat_before'][index], value, f'{t}/flat/before/{index}')

            corrected = wealth.source(pooled, a, b)
            earned = permission.quote(cold, corrected)
            pooled_quote = permission.quote(cold, pooled)
            flat_quote = flat.quote(cold, a, b)
            check(row['corrected'], corrected, f'{t}/corrected')
            check(row['earned'], earned, f'{t}/earned')
            check(row['pooled_quote'], pooled_quote, f'{t}/pooled')
            check(row['flat3'], flat_quote, f'{t}/flat3')

            silent = wealth.a <= 0 and wealth.b <= 0
            if matched and visits[record] == 0:
                witnessed['first'] = True
                assert row['earned'] == row['pooled_quote']
            if matched and silent:
                witnessed['silent'] = True
                assert row['earned'] == row['pooled_quote']
            witnessed['positive'] |= wealth.a > 0 or wealth.b > 0
            witnessed['negative'] |= wealth.a < 0 or wealth.b < 0
            witnessed['new'] |= row['rank'] == 0
            witnessed['no_match'] |= not matched
            witnessed['common'] |= matched and a == b == pooled == cold

            old_active = wealth.a > 0 or wealth.b > 0
            if matched:
                wealth.observe(pooled, a, b)
                permission.observe(pooled, pooled_quote)
                flat.observe(cold, a, b)
                visits[record] += 1
                previous[record] = (
                    float(wealth.a), float(wealth.b), float(permission.u),
                    tuple(map(float, flat.weights)))
            new_active = wealth.a > 0 or wealth.b > 0
            witnessed['recovery'] |= matched and not old_active and new_active

            check(row['wealth_after'][0], wealth.a, f'{t}/wealth/a/after')
            check(row['wealth_after'][1], wealth.b, f'{t}/wealth/b/after')
            check(row['permission_after'], permission.u, f'{t}/permission/after')
            for index, value in enumerate(flat.weights):
                check(row['flat_after'][index], value, f'{t}/flat/after/{index}')
            assert row['wealth_struct_bytes'] == 16
            assert row['binary_struct_bytes'] == 8
            assert row['flat_struct_bytes'] == 24
            assert 0 <= row['max_norm_error'] <= 1e-8

    assert len(rows) == 24 and sorted(wealths) == [0, 1]
    assert all(witnessed.values()), witnessed
    receipt = dict(
        events=len(rows), records=sorted(wealths), numeric_checks=checks,
        max_error=error, passed=True, witnessed=witnessed,
        reader_sha256=reader.digest(HERE/'verify.py'),
        fixture_sha256=hashlib.sha256(run.stdout.encode()).hexdigest(),
        binary_sha256=reader.digest(HERE/'earned_router'),
        scope='One handcrafted full-byte fixture: independent Decimal case '
              'wealth, pooled permission and flat3 quotes/states; first, silent, '
              'positive, negative, recovery, locality, NEW, no-match and exact '
              'common events. No generated worlds.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(receipt, sort_keys=True))


if __name__ == '__main__':
    main()
