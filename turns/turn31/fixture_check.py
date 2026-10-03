#!/usr/bin/env python3
"""Reproduce the one prefreeze turn31 C-versus-independent-Decimal fixture."""
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
    spec = importlib.util.spec_from_file_location('independent_fixture31', HERE/'verify.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    run = subprocess.run([str(HERE/'case_router'), '--fixture'],
                         capture_output=True, text=True, check=True)
    rows = [json.loads(line) for line in run.stdout.splitlines()]
    factors, flats, balances, pools = {}, {}, {}, {}
    error, checks = 0.0, 0

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
                factors.setdefault(record, reader.Factored())
                flats.setdefault(record, reader.Flat())
                balances.setdefault(record, reader.Binary())
                pools.setdefault(record, reader.Binary())
                factor, flat = factors[record], flats[record]
                balanced, pool = balances[record], pools[record]
            else:
                factor, flat = reader.Factored(), reader.Flat()
                balanced, pool = reader.Binary(), reader.Binary()
            cold, a, b, pooled = [row[key] for key in ('cold', 'a', 'b', 'pooled')]
            equal = reader.log_mix(reader.V0, a, b)

            def states(stage):
                for i, value in enumerate((factor.u, factor.v)):
                    check(row['factored_'+stage][i], value, f'{t}/factored/{stage}/{i}')
                for i, value in enumerate(flat.weights):
                    check(row['flat_'+stage][i], value, f'{t}/flat/{stage}/{i}')
                check(row['balanced_'+stage], balanced.u, f'{t}/balanced/{stage}')
                check(row['pooled_'+stage], pool.u, f'{t}/pooled/{stage}')

            states('before')
            for name, price in (('factored', factor.quote(cold, a, b)),
                                ('flat3', flat.quote(cold, a, b)),
                                ('balanced', balanced.quote(cold, equal)),
                                ('pooled2', pool.quote(cold, pooled))):
                check(row[name], price, f'{t}/{name}/quote')
            factor.observe(cold, a, b, matched)
            flat.observe(cold, a, b, matched)
            balanced.observe(cold, equal, matched)
            pool.observe(cold, pooled, matched)
            states('after')
            assert 0 <= row['max_norm_error'] <= 1e-8
    receipt = dict(events=len(rows), records=sorted(factors), numeric_checks=checks,
                   max_error=error, passed=True, reader_sha256=reader.digest(HERE/'verify.py'),
                   fixture_sha256=hashlib.sha256(run.stdout.encode()).hexdigest(),
                   binary_sha256=reader.digest(HERE/'case_router'),
                   scope='One handcrafted-price fixture: independent Decimal factorized u/v, '
                         'three-mass flat, binary balanced and pooled quotes/states; matched '
                         'NEW/equal, unmatched, poor histories and changed favored case. No new worlds.')
    with args.output.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True)
        stream.write('\n')
    print(json.dumps(receipt, sort_keys=True))


if __name__ == '__main__':
    main()
