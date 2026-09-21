#!/usr/bin/env python3
"""JOINT-PREFIX: one frozen batch, explicit irreversible stage outputs."""
import argparse
import collections
import concurrent.futures
import csv
import gzip
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import time

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(128, 136))
N = 16384
SOURCES = ('sourceAB', 'sourceBA', 'sourceCD', 'sourceDC')
REGIMES = ('recombined', 'unrelated', 'switched')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
NAMESPACE = 'netta-joint-prefix-v1'
MAX_RULES = 32
ARCHIVE_CAP = 528
COUNT_MAX = 0xffff


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def save(p, obj):
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open('x') as f:
        json.dump(obj, f, ensure_ascii=False, indent=2, sort_keys=True, allow_nan=False)
        f.write('\n')


def seed(w, name):
    return int.from_bytes(hashlib.sha256(f'{NAMESPACE}|{w}|{name}'.encode()).digest()[:8], 'big')


def freeze():
    names = ['turn13/PROTOCOL.md', 'turn13/experiment.py', 'turn13/verify.py',
             'turn13/episode.c', 'turn13/episode', 'turn13/Makefile',
             'byte_recurrence/frontend.c', 'byte_recurrence/frontend.h',
             'portable_recurrence/recurrence.c', 'portable_recurrence/recurrence.h',
             'court4/transfer4_confirm_core.c']
    save(HERE/'FREEZE.json', {'created_utc':time.time(), 'namespace':NAMESPACE,
         'worlds':WORLDS, 'files':{n:sha(REPO/n) for n in names}})


def check_freeze():
    for name, h in json.loads((HERE/'FREEZE.json').read_text())['files'].items():
        assert sha(REPO/name) == h, f'frozen code changed: {name}'


def manifest(folder):
    return {str(p.relative_to(HERE)):sha(p) for p in sorted(folder.rglob('*')) if p.is_file()}


def emit(raw, commands, alphabet, rng_seed):
    with commands.open('rb') as inp, raw.open('xb') as out:
        done = subprocess.run([str(HERE/'episode'), 'emit', str(alphabet), str(rng_seed or 1)],
                              stdin=inp, stdout=out, stderr=subprocess.PIPE, check=True)
    raw.with_suffix('.emit.log').write_bytes(done.stderr)


def generate_world(w):
    d = HERE/'data'/f'world{w}'; d.mkdir(parents=True, exist_ok=False)
    rng = random.Random(seed(w, 'components'))
    components = []
    while len(components) < 4:
        c = [0, 1, 2, 3]*3; rng.shuffle(c)
        if c not in components: components.append(c)
    cycles = {'sourceAB':(0,1), 'sourceBA':(1,0), 'sourceCD':(2,3),
              'sourceDC':(3,2), 'recombined':(0,3,1,2)}
    commands = {}
    for name, cycle in cycles.items():
        rng = random.Random(seed(w, f'commands-{name}'))
        tape = []
        for t in range(N):
            c = components[cycle[(t//48) % len(cycle)]][t % 12]
            if rng.random() < .08: c = rng.randrange(4)
            tape.append(c)
        commands[name] = bytes(tape)
    rng = random.Random(seed(w, 'commands-unrelated'))
    commands['unrelated'] = bytes(rng.randrange(4) for _ in range(N))
    commands['switched'] = commands['recombined'][:N//2] + commands['unrelated'][N//2:]
    seeds = {}
    for name in SOURCES+REGIMES:
        body = 'recipient' if name in ('recombined', 'switched') else name
        alphabet = list(range(256)); random.Random(seed(w, 'alphabet-'+body)).shuffle(alphabet)
        ap = d/(name+'.alphabet.bin'); ap.write_bytes(bytes(alphabet[:64]))
        cp = d/(name+'.commands.bin'); cp.write_bytes(commands[name])
        seeds[name] = seed(w, 'emit-'+body) or 1
        emit(d/(name+'.bin'), cp, ap, seeds[name])
    assert (d/'recombined.bin').read_bytes()[:8192] == (d/'switched.bin').read_bytes()[:8192]
    save(d/'GENERATOR.json', {'world':w, 'components':components, 'cycles':cycles,
        'emission_seeds':seeds, 'source_joints':[[0,1],[1,0],[2,3],[3,2]],
        'target_joints':[[0,3],[3,1],[1,2],[2,0]],
        'scope':'hidden generation labels; learner receives only emitted bytes'})
    return w


def extract_world(w):
    d = HERE/'data'/f'world{w}'
    for name in SOURCES:
        outp = d/(name+'.tsv.gz'); assert not outp.exists()
        with (d/(name+'.bin')).open('rb') as inp, gzip.open(outp, 'wb') as out:
            p = subprocess.Popen([str(HERE/'episode'), 'trace', 'compact'], stdin=inp,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            while block := p.stdout.read(1 << 20): out.write(block)
            err = p.stderr.read(); assert p.wait() == 0, err
        (d/(name+'.trace.log')).write_bytes(err)
    return w


def patterns(p=(0,)):
    if len(p)==6: yield ''.join(map(str,p))
    else:
        for a in range(max(p)+2): yield from patterns(p+(a,))


PATTERNS = tuple(patterns())
OFFSETS = {}; off = 0
for pattern in PATTERNS:
    OFFSETS[pattern] = off; off += max(map(int,pattern))+2
assert off == 877


def source_tapes(w):
    tapes = []; row = [0]*877
    d = HERE/'data'/f'world{w}'
    for name in SOURCES:
        events = []
        with gzip.open(d/(name+'.tsv.gz'), 'rt') as f:
            for t, r in enumerate(csv.DictReader(f, delimiter='\t')):
                assert int(r['t']) == t
                rank, k = int(r['rank']), int(r['k'])
                assert 0 <= rank <= k <= 6
                events.append(rank)
                if k:
                    labels = list(dict.fromkeys(map(int, r['pattern'][::-1])))
                    g = labels[rank-1] if rank else k
                    row[OFFSETS[r['pattern']]+g] += 1
        assert len(events) == N
        tapes.append(bytes(events))
    return tapes, row


def count_pairs(seq, counts):
    # Equal-symbol pairs overlap; count the disjoint left-to-right matches.
    last_equal = None
    for i in range(len(seq)-1):
        pair = (seq[i],seq[i+1])
        if pair[0] == pair[1] and last_equal == i-1:
            last_equal = None
            continue
        counts[pair] += 1
        last_equal = i if pair[0] == pair[1] else None


def grow(tapes):
    seqs = [list(t) for t in tapes]
    expanded = [bytes([i]) for i in range(7)]
    rules = []; pairs_seen = set()
    for stage in range(MAX_RULES):
        counts = collections.Counter()
        for seq in seqs: count_pairs(seq, counts)
        eligible = [(n,a,b) for (a,b),n in counts.items()
                    if n>=16 and (a,b) not in pairs_seen and len(expanded[a])+len(expanded[b])<=32]
        if not eligible: break
        n,a,b = min(eligible, key=lambda x:(-x[0],x[1],x[2]))
        symbol = len(expanded); ex = expanded[a]+expanded[b]
        expanded.append(ex); pairs_seen.add((a,b))
        support = sum(sum(t[i:i+len(ex)] == ex for i in range(len(t)-len(ex)+1)) for t in tapes)
        rules.append({'left':a,'right':b,'support':support,'stage_support':n,'expansion':list(ex)})
        updated = []
        for seq in seqs:
            out=[]; i=0
            while i < len(seq):
                if i+1<len(seq) and seq[i]==a and seq[i+1]==b:
                    out.append(symbol); i+=2
                else: out.append(seq[i]); i+=1
            updated.append(out)
        seqs=updated
    return rules


def expand_children(children):
    """Forward expansion of a binary dictionary; swapped children reverse it."""
    table = [bytes([i]) for i in range(7)]
    for left, right in children:
        table.append(table[left]+table[right])
    return table[7:]


def information(counts, unconditional, total):
    n = sum(counts)
    if not n: return 0.0
    return math.fsum(c*math.log2((c/n)/(unconditional[r]/total))
                     for r,c in enumerate(counts) if c)


def branch_candidates(children, tapes):
    """Recount all immediate alternatives at each distinct owned prefix."""
    unconditional = collections.Counter(b''.join(tapes))
    total = sum(unconditional.values())
    owner = {}
    for rule_id, expansion in enumerate(expand_children(children)):
        for length in range(1, len(expansion)):
            owner.setdefault(expansion[:length], rule_id)
    records = []
    for content, rule_id in owner.items():
        counts = [0]*7
        for tape in tapes:
            start = 0
            while (found := tape.find(content, start)) >= 0:
                start = found+1  # Overlapping occurrences all count.
                after = found+len(content)
                if after < len(tape): counts[tape[after]] += 1
        assert max(counts) <= COUNT_MAX
        score = information(counts, unconditional, total)-8*16
        records.append({'rule_id':rule_id, 'prefix_len':len(content),
                        'prefix':list(content), 'counts':counts,
                        'total':sum(counts), 'score':score})
    return records


def select_branches(records, capacity, law):
    if law == 'frequency':
        ordered = sorted(records, key=lambda r:(-r['total'],r['rule_id'],-r['prefix_len']))
    else:
        assert law == 'information'
        eligible = [r for r in records if r['total']>=16 and r['score']>0]
        ordered = sorted(eligible, key=lambda r:(-r['score'],-r['prefix_len'],r['prefix']))
    return ordered[:capacity]


def joint_select(records, tapes, capacity):
    """Greedy source gain under the CURRENT longest selected match.

    Only source event positions are inspected. Per-record counts remain the
    original full occurrence counts; this function changes selection alone.
    The empirical seven-role objective is distinct from the recipient quote.
    """
    eligible = [r for r in records if r['total'] >= 16]
    all_events = b''.join(tapes)
    unconditional = collections.Counter(all_events)
    prior = [unconditional[r]/len(all_events) for r in range(7)]
    probabilities = [[c/r['total'] for c in r['counts']] for r in eligible]
    occurrences = []
    for record in eligible:
        context = bytes(record['prefix'])
        positions = []
        offset = 0
        for tape in tapes:
            start = 0
            while (found := tape.find(context, start)) >= 0:
                start = found + 1
                after = found + len(context)
                if after < len(tape): positions.append((offset+after, tape[after]))
            offset += len(tape)
        assert len(positions) == record['total']
        occurrences.append(positions)

    winners = [-1]*len(all_events)
    lengths = [0]*len(all_events)
    selected = []
    chosen_ids = set()
    distributions = []
    steps = []
    source_gain = 0.0
    while len(selected) < capacity:
        best = None
        for idx, record in enumerate(eligible):
            if idx in chosen_ids: continue
            length = record['prefix_len']
            groups = collections.Counter((winners[pos], truth)
                for pos, truth in occurrences[idx] if lengths[pos] < length)
            distribution = probabilities[idx]
            gain = math.fsum(count * math.log2(distribution[truth] /
                (prior[truth] if incumbent < 0 else distributions[incumbent][truth]))
                for (incumbent, truth), count in sorted(groups.items())) - 128.0
            key = (-gain, -length, record['prefix'])
            if best is None or key < best[0]:
                best = (key, idx, gain, sum(groups.values()))
        if best is None or best[2] <= 0.0: break
        _, idx, gain, affected = best
        record = eligible[idx]
        new_owner = len(selected)
        length = record['prefix_len']
        for pos, _ in occurrences[idx]:
            if lengths[pos] < length:
                winners[pos] = new_owner
                lengths[pos] = length
        chosen_ids.add(idx)
        selected.append(record)
        distributions.append(probabilities[idx])
        source_gain += gain + 128.0
        steps.append({'prefix':record['prefix'], 'marginal_gain':gain,
                      'affected_events':affected, 'source_gain_after':source_gain})
    return selected, steps


def rotate_records(records):
    """Null control: slots 1..6 rotate left by one, slot 0 stays in place."""
    return [dict(r, counts=[r['counts'][0]]+[r['counts'][j%6+1] for j in range(1,7)])
            for r in records]


def branch_archive(children, records):
    data = struct.pack('<8sII', b'NETEI001', len(children), len(records))
    data += b''.join(struct.pack('<HH',l,r) for l,r in children)
    data += b''.join(struct.pack('<BB7H', r['rule_id'], r['prefix_len'],
                                 *r['counts']) for r in records)
    assert len(data) <= ARCHIVE_CAP
    return data


def learn_flat(tapes, budget):
    unconditional = collections.Counter(b''.join(tapes))
    total = sum(unconditional.values())
    candidates=[]
    for length in range(1,32):
        freq=collections.Counter(t[i-length:i] for t in tapes for i in range(length,len(t)))
        counts={s:[0]*7 for s,n in freq.items() if n>=16}
        for t in tapes:
            for i in range(length,len(t)):
                ctx=t[i-length:i]
                if ctx in counts: counts[ctx][t[i]]+=1
        for ctx, nums in counts.items():
            n=sum(nums)
            assert max(nums)<=COUNT_MAX
            gain=information(nums,unconditional,total)
            score=gain-8*(15+length)
            if score>0: candidates.append((score,ctx,nums))
    candidates.sort(key=lambda x:(-x[0],-len(x[1]),x[1]))
    records=[]; used=16
    for score,ctx,nums in candidates:
        cost=15+len(ctx)
        if used+cost > budget: continue
        used+=cost; records.append({'context':list(ctx),'counts':nums,'score':score})
    return records, used


def learn_world(w):
    tapes,row=source_tapes(w)
    rules=grow(tapes)
    children=[(r['left'],r['right']) for r in rules]
    swapped=[(r['right'],r['left']) for r in rules]
    capacity=(ARCHIVE_CAP-16-4*len(rules))//16
    candidates=branch_candidates(children,tapes)
    reverse_candidates=branch_candidates(swapped,tapes)
    forward,joint_forward=joint_select(candidates,tapes,capacity)
    isolated=select_branches(candidates,capacity,'information')
    frequency=select_branches(candidates,capacity,'frequency')
    backward,joint_reverse=joint_select(reverse_candidates,tapes,capacity)
    permuted=rotate_records(forward)
    ep=branch_archive(children,forward)
    iso=branch_archive(children,isolated)
    fq=branch_archive(children,frequency)
    rv=branch_archive(swapped,backward)
    pm=branch_archive(children,permuted)
    assert len(pm)==len(ep) and len(rv)<=ARCHIVE_CAP
    folder=HERE/'memory'/f'world{w}'; folder.mkdir(parents=True,exist_ok=False)
    flat, flat_size=learn_flat(tapes,ARCHIVE_CAP)
    fb=struct.pack('<8sII',b'NETFI001',len(flat),0)
    for r in flat:
        ctx=bytes(r['context']); fb+=bytes([len(ctx)])+ctx+struct.pack('<7H',*r['counts'])
    assert len(fb)==flat_size<=ARCHIVE_CAP
    (folder/'episodes.bin').write_bytes(ep); (folder/'reverse.bin').write_bytes(rv)
    (folder/'isolated.bin').write_bytes(iso)
    (folder/'frequency.bin').write_bytes(fq)
    (folder/'permuted.bin').write_bytes(pm); (folder/'flat.bin').write_bytes(fb)
    (folder/'row.bin').write_bytes(struct.pack('<8sII',b'NETHD256',6,877)+struct.pack('<877Q',*row))
    save(folder/'BOOKS.json',{'world':w,'source_bytes':4*N,'rules':rules,'flat':flat,
        'branch_capacity':capacity,'branch_forward':forward,'branch_reverse':backward,
        'branch_frequency':frequency,'branch_isolated':isolated,
        'joint_forward':joint_forward,'joint_reverse':joint_reverse,
        'branch_candidates':candidates,
        'branch_reverse_candidates':reverse_candidates,
        'branch_permuted':permuted,
        'episode_bytes':len(ep),'isolated_bytes':len(iso),
        'reverse_bytes':len(rv),'permuted_bytes':len(pm),
        'frequency_bytes':len(fq),'common_cap_bytes':ARCHIVE_CAP,
        'flat_bytes':len(fb),'row_bytes':7032,
        'expanded_selected_flat_bytes':16+sum(15+r['prefix_len'] for r in forward),
        'source_event_sha256':[hashlib.sha256(t).hexdigest() for t in tapes]})
    return w


def run_target(w, regime):
    d=HERE/'results'/f'world{w}'; d.mkdir(parents=True,exist_ok=True)
    p=d/(regime+'.tsv.gz'); assert not p.exists()
    bank=HERE/'memory'/f'world{w}'
    cmd=[str(HERE/'episode'),'predict',str(bank/'episodes.bin'),str(bank/'isolated.bin'),str(bank/'frequency.bin'),str(bank/'reverse.bin'),
         str(bank/'permuted.bin'),str(bank/'flat.bin'),str(bank/'row.bin')]
    with (HERE/'data'/f'world{w}'/(regime+'.bin')).open('rb') as inp, gzip.open(p,'wb') as out:
        proc=subprocess.Popen(cmd,stdin=inp,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        while block:=proc.stdout.read(1<<20): out.write(block)
        err=proc.stderr.read(); assert proc.wait()==0,err
    (d/(regime+'.log')).write_bytes(err)
    arms={a:dict(gain=0.,candidate_gain=0.,tail=0.,minimum=0.,peak=0.,max_drawdown=0.,
                activation=None,horizons={},matched_events=0,max_matched_length=0) for a in ARMS}
    maxnorm=0.; lengths=collections.Counter()
    with gzip.open(p,'rt') as f:
        for t,r in enumerate(csv.DictReader(f,delimiter='\t')):
            assert int(r['t'])==t
            cold=float(r['logcold'])
            maxnorm=max(maxnorm,float(r['max_norm_error']))
            assert r['new_exact']=='1'
            lengths[int(r['episode_matchedL'])]+=1
            for a,s in arms.items():
                delta=float(r[a+'_live'])-cold
                s['gain']+=delta; s['candidate_gain']+=float(r[a+'_candidate'])-cold
                if t>=8192: s['tail']+=delta
                s['minimum']=min(s['minimum'],s['gain']); s['peak']=max(s['peak'],s['gain'])
                s['max_drawdown']=max(s['max_drawdown'],s['peak']-s['gain'])
                assert abs(s['gain']-float(r[a+'_gain_after'])) < 1e-7
                if int(r[a+'_activated_after']):
                    assert s['activation'] is None
                    s['activation']=t+1
                if t+1 in (1024,4096,8192,16384): s['horizons'][str(t+1)]=s['gain']
                if a!='row':
                    L=int(r[a+'_matchedL']); s['matched_events']+=int(L>0)
                    s['max_matched_length']=max(s['max_matched_length'],L)
    assert t+1==N and maxnorm<1e-8
    for s in arms.values(): assert s['minimum']>=-1-1e-7 and s['max_drawdown']<=16+1e-7
    return {'world':w,'regime':regime,'arms':arms,'trace_sha256':sha(p),
            'max_norm_error':maxnorm,'episode_match_lengths':dict(lengths)}


def summarize(lives):
    by={(x['world'],x['regime']):x['arms'] for x in lives}
    avg=lambda xs:sum(xs)/len(xs)
    early=[by[w,'recombined']['episode']['horizons']['4096'] for w in WORLDS]
    full=[by[w,'recombined']['episode']['gain'] for w in WORLDS]
    diffs={a:[by[w,'recombined']['episode']['horizons']['4096']-
              by[w,'recombined'][a]['horizons']['4096'] for w in WORLDS]
           for a in ('frequency','row','reverse','flat','permuted')}
    switched=[by[w,'switched']['episode']['gain']-by[w,'switched']['row']['gain'] for w in WORLDS]
    tail=[by[w,'switched']['episode']['tail']-by[w,'switched']['row']['tail'] for w in WORLDS]
    tests={'early_gain':avg(early)/4096>=.005,'early_positive':sum(x>0 for x in early)>=6,
           'full_gain':avg(full)/N>=.005,'full_positive':all(x>0 for x in full),
           'switched_mean':avg(switched)>=0,'switched_count':sum(x>0 for x in switched)>=5,
           'tail_tolerance':avg(tail)>=-1}
    for a,ds in diffs.items(): tests[a+'_early_mean']=avg(ds)>1; tests[a+'_early_count']=sum(x>0 for x in ds)>=5
    assert len(tests)==17
    means={reg:{a:{field:avg([by[w,reg][a][field] for w in WORLDS]) for field in ('gain','tail','candidate_gain')}
                for a in ARMS} for reg in REGIMES}
    isolated_differences={reg:{field:[by[w,reg]['episode'][field]-by[w,reg]['isolated'][field]
                                    for w in WORLDS]
                              for field in ('gain','tail','candidate_gain')}
                          for reg in REGIMES}
    for reg in REGIMES:
        isolated_differences[reg]['early']=[by[w,reg]['episode']['horizons']['4096']-
                                           by[w,reg]['isolated']['horizons']['4096'] for w in WORLDS]
        isolated_differences[reg]['horizons']={str(t):[
            by[w,reg]['episode']['horizons'][str(t)]-by[w,reg]['isolated']['horizons'][str(t)]
            for w in WORLDS] for t in (1024,4096,8192,16384)}
    summary={'means':means,'early_gain_bpb':avg(early)/4096,'early_gains':early,
             'isolated_differences':isolated_differences,
             'early_differences':diffs,'switched_differences':switched,'tail_differences':tail}
    return summary,tests


def main():
    parser=argparse.ArgumentParser(); parser.add_argument('stage',choices=['freeze','generate','extract','learn','evaluate'])
    a=parser.parse_args()
    if a.stage=='freeze': freeze(); return
    check_freeze()
    if a.stage in ('generate','extract'):
        fn=generate_world if a.stage=='generate' else extract_world
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool: print(list(pool.map(fn,WORLDS)))
    elif a.stage=='learn':
        for w in WORLDS: print('learned',learn_world(w),flush=True)
    else:
        for n in ('data','memory'):
            for path,h in json.loads((HERE/(n.upper()+'_MANIFEST.json')).read_text()).items(): assert sha(HERE/path)==h
        with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
            futures=[pool.submit(run_target,w,r) for w in WORLDS for r in REGIMES]
            lives=[f.result() for f in futures]
        summary,tests=summarize(lives)
        save(HERE/'RESULT.json',{'worlds':WORLDS,'namespace':NAMESPACE,
            'protocol_sha256':sha(HERE/'PROTOCOL.md'),
            'lives':lives,'summary':summary,'tests':tests,
            'gate_pass':all(tests.values()),'independent_reader_pending':True})
        save(HERE/'RESULTS_MANIFEST.json',manifest(HERE/'results'))
        print(json.dumps({'summary':summary,'tests':tests,'gate_pass':all(tests.values())},indent=2))
    if a.stage=='extract': save(HERE/'DATA_MANIFEST.json',manifest(HERE/'data'))
    if a.stage=='learn': save(HERE/'MEMORY_MANIFEST.json',manifest(HERE/'memory'))


if __name__=='__main__': main()
