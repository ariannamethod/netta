#!/usr/bin/env python3
"""Check the actual writer's schema with artificial records, before worlds."""
import copy
import csv
from decimal import localcontext
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def load(name):
    spec = importlib.util.spec_from_file_location('preflight_'+name, HERE/(name+'.py'))
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main():
    assert not any((HERE/x).exists() for x in ('FREEZE.json','data','memory','results'))
    writer, reader = load('experiment'), load('verify')
    fixed = subprocess.run([str(HERE/'fixture')], capture_output=True, text=True, check=True)
    fixed = json.loads(fixed.stdout)
    writer.save(HERE/'FIXTURE.json', fixed)
    tape = subprocess.run([str(HERE/'fixture'), '--tape'], capture_output=True, check=True).stdout
    replay = subprocess.run([str(HERE/'authority_replay')], input=tape,
                            capture_output=True, check=True)
    rows = list(csv.DictReader(io.StringIO(replay.stdout.decode()), delimiter='\t'))
    maximum = 0.
    with localcontext() as context:
        context.prec = 50
        states = {m:reader.Authority(m) for m in writer.MODES}
        for row in rows:
            expected = {m:states[m].step(int(row['t']),float(row['cold']),
                float(row['candidate']),int(row['matched'])) for m in writer.MODES}
            for m in writer.MODES:
                for field in ('live','odds_before','slow_used','odds_after'):
                    maximum = max(maximum, reader.near(row[m+'_'+field],expected[m][field],m+'/'+field))
            for field in ('shadow_before','active_before','admitted_after','w_before','w_after','latch_before','latch_after'):
                maximum = max(maximum,reader.near(row[field],expected['latch'][field],field))
    assert len(rows) == fixed['summary']['event_count'] == 48
    lives = []
    # These are explicitly invented schema records, not generated worlds or
    # measurements. Exercise the real production packer and its actual keys.
    for w in writer.WORLDS:
        for regime in writer.REGIMES:
            modes = {}
            for m in writer.MODES:
                s = writer.stats()
                s.update(gain=2.,tail=1.,early=.5,peak=2.,activation=33,
                         horizons={'1024':.1,'4096':.5,'8192':1.,'16384':2.})
                modes[m] = s
            lives.append(dict(world=w,regime=regime,modes=modes,
                diagnostics=dict(first_set=None,first_return=None,set_count=0,
                    return_count=0,sets_before_seam=0,returns_before_seam=0,
                    first_set_after_seam=None,latch_at_seam=0),
                exactness=dict.fromkeys(('admission','equal_price','inactive','new','clock','latch_law'),True),
                max_norm_error=0.,archive_bytes=528,raw_help=None,raw_harm=None))
    result = json.loads(json.dumps(writer.pack_result(lives)))
    reader.check_schema(result)
    incomplete = copy.deepcopy(result)
    del incomplete['validity']
    try:
        reader.check_schema(incomplete)
    except Exception as error:
        rejection = str(error)
        assert 'validity' in rejection
    else:
        raise AssertionError('missing validity accepted')
    with tempfile.TemporaryDirectory(prefix='preflight-', dir=HERE/'.build') as temporary:
        probe = Path(temporary)/'probe.txt'
        probe.write_text('original\n')
        wanted = writer.digest(probe)
        reader.check_artifact(probe, wanted)
        probe.write_text('changed\n')
        try:
            reader.check_artifact(probe, wanted)
        except Exception as error:
            artifact_rejection = str(error)
            assert str(probe) in artifact_rejection
        else:
            raise AssertionError('changed artifact accepted')
        occupied = subprocess.run([sys.executable, '-B', str(HERE/'verify.py'),
            '--output', str(probe)], capture_output=True, text=True)
        assert occupied.returncode == 1 and str(probe) in occupied.stderr
    receipt = dict(scope='Artificial schema records only; no fresh data.',
                   fixture=fixed['summary'], independent_fixture_max_error=maximum,
                   writer_schema=True, missing_validity_refused=rejection,
                   changed_artifact_refused=artifact_rejection,
                   existing_output_refused=occupied.stderr.strip(),
                   protocol_sha256=writer.digest(HERE/'PROTOCOL.md'),
                   writer_sha256=writer.digest(HERE/'experiment.py'),
                   reader_sha256=writer.digest(HERE/'verify.py'))
    writer.save(HERE/'PREFLIGHT.json',receipt)
    print(json.dumps(receipt,indent=2))


if __name__ == '__main__':
    main()
