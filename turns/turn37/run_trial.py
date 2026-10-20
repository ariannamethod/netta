#!/usr/bin/env python3
"""Create one sealed batch and one reader receipt; retain each stage log."""
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
for stage in ('freeze','generate','extract','learn','evaluate','verify'):
    cmd=[sys.executable,'-B',str(HERE/('verify.py' if stage=='verify' else 'experiment.py'))]
    if stage!='verify': cmd.append(stage)
    print('start',stage,flush=True)
    with (HERE/(stage+'.stdout')).open('xb') as out, (HERE/(stage+'.stderr')).open('xb') as err:
        done=subprocess.run(cmd,cwd=HERE,stdout=out,stderr=err)
    print('finish',stage,done.returncode,flush=True)
    if done.returncode: raise SystemExit(done.returncode)
