#!/usr/bin/env python3
"""Replay the merged sitting2 mouth and expose every generated stream."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
OUT = HERE/'mouth_runs'


def run(name, command):
    with (HERE/(name+'.stdout')).open('xb') as stdout, (HERE/(name+'.stderr')).open('xb') as stderr:
        p = subprocess.run(list(map(str, command)), cwd=ROOT, stdout=stdout, stderr=stderr)
    assert p.returncode == 0, (name, p.returncode)


def main():
    OUT.mkdir()
    flags = ['cc', '-O2', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    for name in ('netta_mouth', 'netta_mouth_check'):
        run('build_'+name, flags+['-o', OUT/name, ROOT/(name+'.c'), '-lm'])
    sealed = (ROOT/'speech_court/sitting2/SITTING2.md').read_bytes()
    raw, receipt = [], {}
    for mode in ('plain', 'live', 'shuffled'):
        folder = OUT/mode
        folder.mkdir()
        modeflag = 'none' if mode == 'plain' else mode
        extra = [] if mode == 'plain' else ['--citizens', ROOT/'speech_court/court4_relations.tsv']
        run('mouth_'+mode, [OUT/'netta_mouth', ROOT/'netta.txt', '--out', folder,
                           '--corridor', '1', '--citizens-mode', modeflag]+extra)
        report = HERE/('MOUTH_'+mode+'.txt')
        run('mouth_reader_'+mode, [OUT/'netta_mouth_check', ROOT/'netta.txt', '--dir', folder,
                                  '--report', report, '--corridor', '1', '--citizens-mode', modeflag])
        assert report.read_bytes() == (ROOT/'speech_court/sitting2'/('report_'+mode+'.txt')).read_bytes()
        receipt[mode] = dict(report_sha256=hashlib.sha256(report.read_bytes()).hexdigest(), streams={})
        for seed in (7, 19, 42, 101, 271):
            speech = (folder/f'speech_{seed}.bin').read_bytes()
            pattern = (rb'BEGIN RAW SPEECH mode='+mode.encode()+rb' seed='+str(seed).encode()+
                       rb' bytes=(\d+) sha256=([a-f0-9]{64})\n(.*?)\nEND RAW SPEECH mode='+
                       mode.encode()+rb' seed='+str(seed).encode())
            match = re.search(pattern, sealed, re.S)
            assert match and len(speech) == int(match[1]) and speech == match[3]
            digest = hashlib.sha256(speech).hexdigest()
            assert digest == match[2].decode()
            receipt[mode]['streams'][seed] = digest
            raw.append(f'## {mode}, seed {seed}\n\n```text\n'+speech.decode()+'\n```\n')
    with (HERE/'MOUTH_RAW.md').open('x') as f:
        f.write('# Astra replay of sitting2, 2026-10-05\n\n'+ '\n'.join(raw))
    with (HERE/'MOUTH.json').open('x') as f:
        json.dump(receipt, f, indent=2, sort_keys=True)
        f.write('\n')
    print('Three sealed reports and all fifteen raw streams reproduced byte-for-byte.')


if __name__ == '__main__':
    main()
