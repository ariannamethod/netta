"""Posthoc presentation of the single retained batch; no new predictions."""
from pathlib import Path
import csv
import gzip
import json
import math
import statistics

P = Path(__file__).resolve().parent
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
REGIMES = ('recombined', 'unrelated', 'switched')
WORLDS = range(128, 136)
mean = statistics.mean


def put(name, text):
    with (P/name).open('x') as f:
        f.write(text)


def main():
    result = json.loads((P/'RESULT.json').read_text())
    verified = json.loads((P/'VERIFY.json').read_text())
    by = {(x['world'], x['regime']): x['arms'] for x in result['lives']}
    books = {w: json.loads((P/'memory'/f'world{w}'/'BOOKS.json').read_text()) for w in WORLDS}
    stats = {'scope': 'Posthoc presentation only, no alternative selection or prediction.',
             'means': {}, 'source_memory': [], 'examples': []}
    out = ['# All outcomes of the joint selection batch', '',
           'Gains are bits saved over the shared cold P0. Positive is better.',
           'Advice is the candidate before authority; received is the actual charged forecast.',
           'All byte horizons count raw bytes. No worlds or adverse cases are omitted.', '',
           '## Mean outcomes', '',
           '| Regime | Arm | First4096 received | Full16384 received | Full candidate | Last8192 received | Early+/full+ | Admitted |',
           '|---|---|---:|---:|---:|---:|---:|---:|']
    for regime in REGIMES:
        stats['means'][regime] = {}
        for arm in ARMS:
            ss = [by[w, regime][arm] for w in WORLDS]
            a = {k: mean(s[k] for s in ss) for k in ('gain', 'candidate_gain', 'tail')}
            a.update(early=mean(s['horizons']['4096'] for s in ss),
                     early_positive=sum(s['horizons']['4096'] > 0 for s in ss),
                     full_positive=sum(s['gain'] > 0 for s in ss),
                     admitted=sum(s['activation'] is not None for s in ss))
            stats['means'][regime][arm] = a
            out.append(f"| {regime} | {arm} | {a['early']:.6f} | {a['gain']:.6f} | "
                       f"{a['candidate_gain']:.6f} | {a['tail']:.6f} | {a['early_positive']}/{a['full_positive']} | {a['admitted']} |")
    out += ['', '## Paired episode minus isolated predecessor', '',
            '| World | Early4096 | Full recombined | Whole switched | Changed tail |',
            '|---:|---:|---:|---:|---:|']
    for w in WORLDS:
        u, s = by[w, 'recombined'], by[w, 'switched']
        out.append(f"| {w} | {u['episode']['horizons']['4096']-u['isolated']['horizons']['4096']:+.6f} | "
                   f"{u['episode']['gain']-u['isolated']['gain']:+.6f} | {s['episode']['gain']-s['isolated']['gain']:+.6f} | "
                   f"{s['episode']['tail']-s['isolated']['tail']:+.6f} |")
    out += ['', '## Source memory', '',
            'The literal column is the SAME jointly selected records in explicit-string form.',
            'It is not the separately learned flat arm. Every small arm has its own528-byte cap.', '',
            '| World | Joint/isolated rows | Joint bytes | Isolated bytes | Frequency bytes | Reverse bytes | Flat bytes | Same joint literal bytes | Common joint/isolated prefixes |',
            '|---:|---:|---:|---:|---:|---:|---:|---:|---:|']
    for w, b in books.items():
        common = len({tuple(r['prefix']) for r in b['branch_forward']} &
                     {tuple(r['prefix']) for r in b['branch_isolated']})
        memory = dict(world=w, joint_rows=len(b['branch_forward']), isolated_rows=len(b['branch_isolated']),
                      common_prefixes=common, **{k: v for k, v in b.items() if k.endswith('_bytes')})
        stats['source_memory'].append(memory)
        out.append(f"| {w} | {memory['joint_rows']}/{memory['isolated_rows']} | {b['episode_bytes']} | "
                   f"{b['isolated_bytes']} | {b['frequency_bytes']} | {b['reverse_bytes']} | {b['flat_bytes']} | "
                   f"{b['expanded_selected_flat_bytes']} | {common} |")
    out += ['', 'Permuted uses the joint archive size; pooled row uses7032bytes in every world.',
            'Source selection history is retained in BOOKS.joint_forward and joint_reverse.', '',
            '## Every horizon, admission and loss bound', '']
    for w in WORLDS:
        out += [f'### World{w}', '',
                '| Regime | Arm | 1024 | 4096 | 8192 | 16384 | Candidate total | Last8192 | Admission after byte count | Minimum prefix | Maximum drawdown |',
                '|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|']
        for regime in REGIMES:
            for arm in ARMS:
                s = by[w, regime][arm]
                horizons = ' | '.join(f"{s['horizons'][str(t)]:.6f}" for t in (1024, 4096, 8192, 16384))
                admission = s['activation'] if s['activation'] is not None else '-'
                out.append(f"| {regime} | {arm} | {horizons} | {s['candidate_gain']:.6f} | {s['tail']:.6f} | "
                           f"{admission} | {s['minimum']:.6f} | {s['max_drawdown']:.6f} |")
        out.append('')
    put('TABLES.md', '\n'.join(out)+'\n')

    worst = min(WORLDS, key=lambda w: by[w, 'switched']['episode']['tail']-by[w, 'switched']['isolated']['tail'])
    raw = ['# Raw quoted behavior', '',
           'Synthetic raw byte behavior; this is not speech. Quotes are prepared before',
           'truth; the tables append the subsequently observed byte. t is a zero-based offset.',
           'Role0 means NEW/incomplete; r>0 means the r-th distinct current recency head.', '',
           'Selection of these illustrations is POSTHOC: in the first world128, take the',
           'first early byte with >.5bit joint gain/P0 and >.5bit advantage over isolated,',
           'and the first early byte with <−.5bit joint gain/P0. Also take the first',
           '<−.5bit byte after switch in the world with the worst paired joint−isolated tail.',
           'These are explanatory excerpts, not a gate or a representative sample.',
           'Complete per-world outcomes and all negative tails remain in TABLES.md.', '']
    for w, regime in ((128, 'recombined'), (worst, 'switched')):
        with gzip.open(P/'results'/f'world{w}'/(regime+'.tsv.gz'), 'rt') as f:
            rows = list(csv.DictReader(f, delimiter='\t'))
        payload = (P/'data'/f'world{w}'/(regime+'.bin')).read_bytes()
        selected = {}
        for r in rows:
            t = int(r['t']); cold = float(r['logcold'])
            gain = float(r['episode_live'])-cold
            diff = float(r['episode_live'])-float(r['isolated_live'])
            tests = {'early benefit': regime == 'recombined' and t < 4096 and gain > .5 and diff > .5,
                     'early harm': regime == 'recombined' and t < 4096 and gain < -.5,
                     'changed-tail harm': regime == 'switched' and t >= 8192 and gain < -.5}
            for label, ok in tests.items():
                if ok and label not in selected: selected[label] = r
        for label, r in selected.items():
            t = int(r['t']); idx = int(r['episode_matches'])
            record = books[w]['branch_forward'][idx]
            step = books[w]['joint_forward'][idx]
            case = dict(label=label, world=w, regime=regime, t=t, record=record,
                        selection_step=step, trace_row=r,
                        preceding_hex=payload[max(0,t-16):t].hex(' '), truth_hex=f'{payload[t]:02x}')
            stats['examples'].append(case)
            raw += [f'## {label}: world{w}, {regime}, t={t}', '',
                    f"Raw preceding16 bytes: `{case['preceding_hex']}`; truth `{case['truth_hex']}`.",
                    f"Completed pretruth role history: `{r['history']}`.",
                    'Current heads: `' + ', '.join(f'R{i}={int(x):02x}' for i,x in enumerate(r['heads'].split(','),1)) + '`.',
                    f"Truth rank{r['rank']}; joint prefix `{''.join(map(str,record['prefix']))}`, source counts0..6 `{record['counts']}`.",
                    f"Selection step{idx+1}: marginal score {step['marginal_gain']:.6f}bits; "
                    f"{step['affected_events']} affected source events; original isolated score {record['score']:.6f}bits.", '',
                    '| Forecast for observed truth | Probability | Gain/P0 bits |', '|---|---:|---:|']
            for name, key in (('P0','logcold'),('Joint advice','episode_candidate'),('Joint received','episode_live'),
                              ('Isolated advice','isolated_candidate'),('Isolated received','isolated_live'),
                              ('Row received','row_live')):
                val = float(r[key]);raw.append(f"| {name} | {math.exp2(val):.9f} | {val-float(r['logcold']):+.6f} |")
            raw += ['', f"Pretruth log2(source/cold odds): joint={float(r['episode_odds_before']):.6f}, "
                    f"isolated={float(r['isolated_odds_before']):.6f}.", '',
                    '| t | Truth hex | Rank | Joint prefix L | Joint received gain | Isolated received gain |',
                    '|---:|---|---:|---:|---:|---:|']
            for rr in rows[max(0,t-4):t+5]:
                cold = float(rr['logcold'])
                raw.append(f"| {rr['t']} | {int(rr['truth']):02x} | {rr['rank']} | {rr['episode_matchedL']} | "
                           f"{float(rr['episode_live'])-cold:+.6f} | {float(rr['isolated_live'])-cold:+.6f} |")
            raw.append('')
    put('RAW.md','\n'.join(raw)+'\n')
    stats['verification'] = {k: verified[k] for k in ('verification_pass','gate_pass','predictions','maximum_numeric_error')}
    put('PRESENTATION.json',json.dumps(stats,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'cases':len(stats['examples']),'worst_paired_tail_world':worst,'verified':verified['verification_pass']}))


if __name__ == '__main__':
    main()
