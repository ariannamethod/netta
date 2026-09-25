#!/usr/bin/env python3
"""Independent turn29 source, router, trace, authority and verdict reader."""
import argparse
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NAMESPACE = 'netta-local-pooled-permission-v1'
WORLDS = tuple(range(272, 280))
REGIMES = ('recombined','partial','switched','moved_mid','unrelated')
ARMS = ('local_full','global_full','pooled_full','bank_local','permuted_full')
FULL_ARMS = ('local_full','global_full','pooled_full','permuted_full')
N = 16384
MOVE = 8192
EARLY = 4096
TOL = 1e-7
RHO = Decimal(1)/1024
PRIOR = Decimal(7)/8
PROTOCOL_SHA = '3174e44eb930042d2018aab6108dcc558d4d00d4426f5fdd8af1247dc001166c'

spec = importlib.util.spec_from_file_location('turn29_reader_turn28', ROOT/'turns/turn28/verify.py')
p28 = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = p28
spec.loader.exec_module(p28)
p28.HERE, p28.ROOT = HERE, ROOT
p28.WORLDS, p28.REGIMES = WORLDS, REGIMES

FULL_COMMON = ('t','k','heads','cold_heads','truth','rank','history','logcold',
               'matched','record','source','perm_source','max_norm_error','new_exact')
FULL_FIELDS = (FULL_COMMON + tuple(a+'_'+s for a in ('local','global','permuted')
                                   for s in ('w_before','w_after')) +
               tuple(a+'_'+s for a in FULL_ARMS for s in
                     ('candidate','live','shadow_before','odds_before','active_before',
                      'activated_after','gain_after')))


def need(value, label):
    if not value: raise AssertionError(label)


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            h.update(part)
    return h.hexdigest()


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+' finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}; error={error}')
    return error


def compare(actual, expected, label):
    return p28.compare(actual, expected, label)


def rotate(counts):
    return [counts[0]]+counts[2:]+counts[1:2]


class BinaryRouter:
    def __init__(self): self.memory = PRIOR

    def quote(self, cold, source):
        if cold == source: return cold
        scale = max(cold, source)
        mass = (1-self.memory)*p28.power2(cold-scale)+self.memory*p28.power2(source-scale)
        return scale+p28.mass_log2(mass)

    def observe(self, source, quoted, matched=True):
        if not matched: return
        posterior = self.memory*p28.power2(source-quoted)
        self.memory = (1-RHO)*posterior+RHO*PRIOR
        need(RHO*PRIOR <= self.memory <= 1-RHO*(1-PRIOR), 'binary share bounds')


def verify_memory(world):
    tables, receipt = p28.verify_books(world)
    folder = HERE/'memory'/f'world{world}'
    meta = json.loads((folder/'BOOKS.json').read_text())
    pairs, _ = p28.compose(meta['rules'], str(folder))
    ordered = sorted(((index,prefix,counts) for prefix,(index,counts) in tables['full'].items()))
    records = [(meta['selected'][index]['rule_id'], prefix, rotate(counts))
               for index,prefix,counts in ordered]
    encoded = p28.encode(pairs, records)
    path = folder/'permuted_full.bin'
    need(len(encoded) <= 528 and path.read_bytes() == encoded,
         str(path)+' independently rebuilt full permutation')
    tables['permuted_full'] = {
        prefix:(index,rotate(counts)) for index,prefix,counts in ordered}
    receipt['permuted_full_bytes'] = len(encoded)
    receipt['permuted_full_sha256'] = digest(path)
    return tables, receipt


def distribution(cold_heads, counts):
    k = len(cold_heads)
    repeat = [math.exp2(x) for x in cold_heads]
    mass = math.fsum(repeat)
    valid = sum(counts[1:k+1]) if counts is not None else 0
    changed = ([mass*(counts[r]+.5)/(valid+.5*k) for r in range(1,k+1)]
               if valid and k else repeat)
    need(all(x>0 for x in changed) and abs(math.fsum([1-mass]+changed)-1)<=1e-8,
         'positive normalized source distribution')
    return repeat, changed


def blank_outer():
    return {arm:p28.Outer() for arm in ARMS}


def check_life(world, regime, tables, saved):
    folder = HERE/'results'/f'world{world}'
    full_path = folder/(regime+'.full.tsv.gz')
    bank_path = folder/(regime+'.bank.tsv.gz')
    raw = (HERE/'data'/f'world{world}'/(regime+'.bin')).read_bytes()
    need(len(raw)==N, str(full_path)+' raw budget')
    if regime in ('partial','switched','moved_mid'):
        base = (HERE/'data'/f'world{world}'/'recombined.bin').read_bytes()
        need(raw[:MOVE] == base[:MOVE], str(full_path)+' shared prefix')

    local = [BinaryRouter() for _ in tables['full']]
    permuted = [BinaryRouter() for _ in tables['full']]
    global_router = BinaryRouter()
    bank = [p28.Router() for _ in tables['bank']]
    states = blank_outer()
    history = []
    max_error = max_norm = 0.0
    exact = dict(new=True,equal=True,inactive=True,history=True,cross_trace=True)
    best = worst = bank_best = bank_worst = None

    def check(actual, wanted, label, tolerance=TOL):
        nonlocal max_error
        max_error = max(max_error, near(actual,wanted,label,tolerance))

    with gzip.open(full_path,'rt') as fa, gzip.open(bank_path,'rt') as fb:
        frs, brs = csv.DictReader(fa,delimiter='\t'), csv.DictReader(fb,delimiter='\t')
        need(tuple(frs.fieldnames or ()) == FULL_FIELDS, str(full_path)+' exact header')
        need(tuple(brs.fieldnames or ()) == p28.FIELDS, str(bank_path)+' frozen turn28 header')
        for t in range(N):
            fr, br = next(frs,None), next(brs,None)
            label = f'{world}/{regime}/{t}'
            need(fr is not None and br is not None, label+' complete traces')
            for key in ('t','k','heads','cold_heads','truth','rank','history','logcold'):
                need(fr[key] == br[key], label+' cross trace '+key)
            need(int(fr['t'])==t and int(fr['truth'])==raw[t], label+' causal chronology')
            heads, k = p28.ints(fr['heads']), int(fr['k'])
            rank = p28.bindings(k,heads,raw[t],label)
            need(int(fr['rank'])==rank, label+' rank')
            expected_history = ''.join(map(str,history)) if history else '-'
            need(fr['history']==expected_history, label+' pretruth history')
            cold = float(fr['logcold']); logs = p28.floats(fr['cold_heads'])
            need(math.isfinite(cold) and cold<=0 and len(logs)==k, label+' P0')
            if rank: check(cold,logs[rank-1],label+' bound truth',1e-10)

            flen, frecord, fcounts = p28.suffix_match(tables['full'],history)
            plen, precord, pcounts = p28.suffix_match(tables['permuted_full'],history)
            blen, brecord, books = p28.suffix_match(tables['bank'],history)
            need((flen,frecord)==(plen,precord), label+' full permutation match')
            need(int(fr['matched'])==flen and int(fr['record'])==frecord,
                 label+' full match')
            need(int(br['matched'])==blen and int(br['record'])==brecord and
                 int(br['full_matched'])==flen and int(br['full_record'])==frecord,
                 label+' bank/full match')
            source = p28.source_candidate(cold,logs,rank,fcounts)
            perm_source = p28.source_candidate(cold,logs,rank,pcounts)
            check(fr['source'],source,label+'/source')
            check(fr['perm_source'],perm_source,label+'/perm-source')
            _, full_dist = distribution(logs,fcounts)
            _, perm_dist = distribution(logs,pcounts)
            if blen:
                a_counts,b_counts = books
            else:
                a_counts=b_counts=None
            a = p28.source_candidate(cold,logs,rank,a_counts)
            b = p28.source_candidate(cold,logs,rank,b_counts)
            check(br['bank_a'],a,label+'/bank-a')
            check(br['bank_b'],b,label+'/bank-b')
            _, a_dist = distribution(logs,a_counts)
            repeats,b_dist = distribution(logs,b_counts)

            lrouter = local[frecord] if flen else BinaryRouter()
            prouter = permuted[frecord] if flen else BinaryRouter()
            brouter = bank[brecord] if blen else p28.Router()
            before = {'local':lrouter.memory,'global':global_router.memory,
                      'permuted':prouter.memory}
            for name,value in before.items():
                check(fr[name+'_w_before'],value,label+'/'+name+' before')
            bank_before = p28.floats(br['local_w_before'])
            need(len(bank_before)==3,label+' bank weight width')
            for i,value in enumerate(brouter.weights):
                check(bank_before[i],value,label+f'/bank before {i}')

            candidates = {
                'local_full':lrouter.quote(cold,source),
                'global_full':global_router.quote(cold,source),
                'pooled_full':source,
                'bank_local':brouter.quote(cold,a,b),
                'permuted_full':prouter.quote(cold,perm_source)}
            for arm in FULL_ARMS:
                check(fr[arm+'_candidate'],candidates[arm],label+'/'+arm+' candidate')
            check(br['local_candidate'],candidates['bank_local'],label+'/bank candidate')

            # Full distributions establish normalization independently of the
            # observed truth; mixtures are convex combinations of these rows.
            repeat_mass = math.fsum(repeats)
            need(0 <= repeat_mass < 1,label+' repeat mass')
            for weight,dist in ((lrouter.memory,full_dist),(global_router.memory,full_dist),
                                (prouter.memory,perm_dist)):
                mixed = [1-repeat_mass]+[(1-float(weight))*repeats[i]+float(weight)*dist[i]
                                          for i in range(k)]
                need(all(x>0 for x in mixed) and abs(math.fsum(mixed)-1)<=1e-8,
                     label+' binary full normalization')
            bmixed = [1-repeat_mass]+[
                float(brouter.weights[0])*repeats[i]+float(brouter.weights[1])*a_dist[i]+
                float(brouter.weights[2])*b_dist[i] for i in range(k)]
            need(all(x>0 for x in bmixed) and abs(math.fsum(bmixed)-1)<=1e-8,
                 label+' bank full normalization')

            wanted = {}
            for arm in ARMS:
                wanted[arm] = states[arm].step(t,cold,candidates[arm])
                row,field = (br,'local') if arm=='bank_local' else (fr,arm)
                for key,value in wanted[arm].items():
                    if type(value) is int:
                        need(int(row[field+'_'+key])==value,label+'/'+arm+'/'+key)
                    else:
                        check(row[field+'_'+key],value,label+'/'+arm+'/'+key)
                if not rank:
                    need(float(row[field+'_candidate'])==cold and float(row[field+'_live'])==cold,
                         label+' protected NEW '+arm)
                if candidates[arm]==cold:
                    need(float(row[field+'_live'])==cold,label+' exact equal '+arm)
                if not wanted[arm]['active_before']:
                    need(float(row[field+'_live'])==cold,label+' cold before admission '+arm)

            if flen:
                local[frecord].observe(source,candidates['local_full'])
                global_router.observe(source,candidates['global_full'])
                permuted[frecord].observe(perm_source,candidates['permuted_full'])
            if blen: bank[brecord].observe(cold,a,b,matched=True)
            after = {'local':(local[frecord].memory if flen else PRIOR),
                     'global':global_router.memory,
                     'permuted':(permuted[frecord].memory if flen else PRIOR)}
            for name,value in after.items():
                check(fr[name+'_w_after'],value,label+'/'+name+' after')
            bank_after = p28.floats(br['local_w_after'])
            wanted_bank = bank[brecord].weights if blen else list(p28.PRIOR)
            for i,value in enumerate(wanted_bank):
                check(bank_after[i],value,label+f'/bank after {i}')

            max_norm = max(max_norm,float(fr['max_norm_error']),float(br['max_norm_error']))
            if t >= MOVE:
                delta = float(fr['local_full_live'])-float(fr['pooled_full_live'])
                sample = dict(fr,difference=delta,raw_hex=raw[max(0,t-16):t+17].hex())
                if best is None or delta>best['difference']: best=sample
                if worst is None or delta<worst['difference']: worst=sample
                bd = float(fr['local_full_live'])-float(br['local_live'])
                bs = dict(fr,bank_local_live=br['local_live'],difference=bd,
                          raw_hex=raw[max(0,t-16):t+17].hex())
                if bank_best is None or bd>bank_best['difference']: bank_best=bs
                if bank_worst is None or bd<bank_worst['difference']: bank_worst=bs
            history.append(rank); history=history[-32:]
        need(next(frs,None) is None and next(brs,None) is None,label+' no excess rows')

    life = dict(world=world,regime=regime,arms={arm:states[arm].result() for arm in ARMS},
        max_norm_error=max_norm,
        exactness=dict(new=True,equal=True,inactive=True,history=True,cross_trace=True),
        raw_help=best,raw_harm=worst,bank_help=bank_best,bank_harm=bank_worst)
    max_error=max(max_error,compare(life,saved,f'RESULT/{world}/{regime}'))
    return life,N*len(ARMS),max_error


def mean(values): return math.fsum(values)/len(values)


def rebuild(lives):
    by={(x['world'],x['regime']):x for x in lives}
    vals=lambda regime,arm,key:[by[w,regime]['arms'][arm][key] for w in WORLDS]
    diffs=lambda regime,other,key:[a-b for a,b in zip(vals(regime,'local_full',key),
                                                        vals(regime,other,key))]
    tables={regime:{arm:{key:mean(vals(regime,arm,key)) for key in ('early','gain','tail')}
            for arm in ARMS} for regime in REGIMES}
    early=vals('recombined','local_full','early'); full=vals('recombined','local_full','gain')
    ep=diffs('recombined','pooled_full','early'); fb=diffs('recombined','bank_local','gain')
    pt=diffs('partial','pooled_full','tail'); pw=diffs('partial','pooled_full','gain')
    eg=diffs('recombined','global_full','early'); er=diffs('recombined','permuted_full','early')
    pooled=tables['recombined']['pooled_full']['gain']
    q=dict(early_gain_per_byte=mean(early)/EARLY,early_positive=sum(x>0 for x in early),
        early_vs_pooled=mean(ep),early_wins_pooled=sum(x>0 for x in ep),
        full_retention=mean(full)/max(pooled,1e-300),full_positive=sum(x>0 for x in full),
        full_vs_bank=mean(fb),full_wins_bank=sum(x>0 for x in fb),
        partial_tail_vs_pooled=mean(pt),partial_tail_wins_pooled=sum(x>0 for x in pt),
        partial_whole_vs_pooled=mean(pw),early_vs_global=mean(eg),
        early_wins_global=sum(x>0 for x in eg),early_vs_permuted=mean(er),
        early_wins_permuted=sum(x>0 for x in er))
    conditions=dict(T1=q['early_gain_per_byte']>=.005 and q['early_positive']>=6,
        T2=q['early_vs_pooled']>1 and q['early_wins_pooled']>=5,
        T3=pooled>0 and q['full_retention']>=.95 and q['full_positive']==8 and
           q['full_vs_bank']>1 and q['full_wins_bank']>=5,
        T4=q['partial_tail_vs_pooled']>1 and q['partial_tail_wins_pooled']>=5 and
           q['partial_whole_vs_pooled']>=0,
        T5=q['early_vs_global']>1 and q['early_wins_global']>=5 and
           q['early_vs_permuted']>1 and q['early_wins_permuted']>=5)
    validity={key:True for key in ('archive','source_counts','normalization','exact_quotes','bounds')}
    return dict(tables=tables,quantities=q,conditions=conditions,validity=validity,
                material_pass=all(conditions.values()))


def identity(result):
    frozen=json.loads((HERE/'FREEZE.json').read_text())
    for key,wanted in dict(namespace=NAMESPACE,worlds=list(WORLDS),regimes=list(REGIMES),
                           arms=list(ARMS)).items():
        need(result[key]==wanted and frozen[key]==wanted,'identity '+key)
    need(digest(HERE/'PROTOCOL.md')==PROTOCOL_SHA and result['protocol_sha256']==PROTOCOL_SHA,
         'protocol identity')
    need(result['independent_reader_pending'] is True,'writer pending reader')
    for name,wanted in frozen['files'].items(): need(digest(ROOT/name)==wanted,'freeze '+name)
    manifest_counts={}
    for name in ('DATA_MANIFEST.json','EXTRACT_MANIFEST.json','MEMORY_MANIFEST.json','RESULTS_MANIFEST.json'):
        manifest=json.loads((HERE/name).read_text())
        for rel,wanted in manifest.items(): need(digest(HERE/rel)==wanted,'manifest '+rel)
        manifest_counts[name]=len(manifest)
    return dict(freeze_sha256=digest(HERE/'FREEZE.json'),reader_sha256=digest(Path(__file__)),
                result_sha256=digest(HERE/'RESULT.json'),freeze_files=len(frozen['files']),
                manifest_entries=manifest_counts)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=HERE/'VERIFY.json')
    args=parser.parse_args(); need(not args.output.exists(),'new output path')
    result=json.loads((HERE/'RESULT.json').read_text())
    pins=identity(result)
    recorded={(x['world'],x['regime']):x for x in result['lives']}
    need(set(recorded)=={(w,r) for w in WORLDS for r in REGIMES},'complete life grid')
    lives=[]; books=[]; count=0; max_error=0.0
    with localcontext() as context:
        context.prec=50
        for world in WORLDS:
            tables,receipt=verify_memory(world); books.append(receipt)
            for regime in REGIMES:
                life,n,error=check_life(world,regime,tables,recorded[world,regime])
                lives.append(life); count+=n; max_error=max(max_error,error)
                print('verified',world,regime,flush=True)
    need(count==len(WORLDS)*len(REGIMES)*N*len(ARMS),'forecast count')
    rebuilt=rebuild(lives)
    for key,value in rebuilt.items(): max_error=max(max_error,compare(result[key],value,'RESULT/'+key))
    receipt=dict(verification_pass=True,material_pass=rebuilt['material_pass'],
        forecasts_checked=count,max_error=max_error,
        source_counts_checked=sum(x['counts_checked']+x['book_counts_checked'] for x in books),
        conditions=rebuilt['conditions'],quantities=rebuilt['quantities'],validity=rebuilt['validity'],
        pins=pins,source_books=books,tables=rebuilt['tables'],lives=lives,
        scope='Independent source continuation recount, exact pooled/permuted archives, binary and bank routers, cross-trace chronology, outer authority, metrics and verdict. Inherited grammar selection and HEAD256 P0/bindings are hash-pinned supplied boundaries.')
    with args.output.open('x') as stream:
        json.dump(receipt,stream,indent=2,sort_keys=True,allow_nan=False); stream.write('\n')
    print(json.dumps({k:receipt[k] for k in ('verification_pass','material_pass','forecasts_checked','max_error','source_counts_checked','conditions')},sort_keys=True))


if __name__=='__main__': main()
