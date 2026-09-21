#!/usr/bin/env python3
"""One disjoint C/mass check and the actual serialized writer constructor."""
import csv
from decimal import localcontext
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent

def module(name, file):
    spec = importlib.util.spec_from_file_location(name, HERE/file)
    item = importlib.util.module_from_spec(spec)
    sys.modules[name] = item
    spec.loader.exec_module(item)
    return item

def main():
    writer = module('turn22_fixture_writer', 'experiment.py')
    reader = module('turn22_fixture_reader', 'verify.py')
    # JSON roundtrip is the real output boundary (tuples serialize as arrays).
    claim = json.loads(json.dumps(writer.pack_result([], {}, {}, {}, []), allow_nan=False))
    schema = reader.check_schema(claim)
    absent = []
    for key in schema:
        wrong = dict(claim)
        del wrong[key]
        try:
            reader.check_schema(wrong)
        except AssertionError as exc:
            assert str(exc) == 'RESULT.json schema is missing required key: '+key
            absent.append(key)
        else:
            raise AssertionError('missing field accepted: '+key)
    fixture = json.loads((HERE/'FIXTURE.json').read_text())
    states = {m: reader.Authority(m) for m in reader.MODES}
    rename = dict(evidence_before='clock_before', evidence_after='clock_after',
                  admitted='admitted_after')
    maximum = 0.0
    with localcontext() as context:
        context.prec = 50
        for row in fixture['events']:
            for mode in reader.MODES:
                expected = states[mode].step(row['t'], row['cold'], row['candidate'], row['matched'])
                for key, actual in row['modes'][mode].items():
                    wanted = expected[rename.get(key, key)]
                    if wanted is None or isinstance(wanted, int):
                        assert actual == wanted, (row['t'], mode, key)
                    else:
                        maximum = max(maximum, reader.near(actual, wanted, f'{row["t"]}/{mode}/{key}'))
    # This is the same fixture, through the real TSV entry point.
    payload = ''.join(f'{r["t"]} {r["cold"]:.17g} {r["candidate"]:.17g} {r["matched"]}\n'
                      for r in fixture['events'])
    process = subprocess.run([str(HERE/'authority_replay')], input=payload, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    tape = csv.DictReader(io.StringIO(process.stdout), delimiter='\t')
    assert tape.fieldnames == reader.COLUMNS
    assert len(list(tape)) == len(fixture['events'])
    result = dict(pass_=True, scope='One disjoint fixture; no world data generated.',
                  actual_serialized_constructor=True, required_schema_keys=schema,
                  named_missing_key_rejections=absent,
                  c_mass_state_events=len(fixture['events'])*len(reader.MODES),
                  maximum_numeric_error=maximum, exact_tsv_columns=len(reader.COLUMNS),
                  fixture_sha256=writer.digest(HERE/'FIXTURE.json'),
                  reader_sha256=writer.digest(HERE/'verify.py'),
                  writer_sha256=writer.digest(HERE/'experiment.py'))
    writer.save(HERE/'PREFREEZE_CHECK.json', result)
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    main()
