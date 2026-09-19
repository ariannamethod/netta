#!/usr/bin/env python3
"""Independent reader for source-grown relation episodes with priced doubt.

Does not import experiment.py, generate new worlds, or independently rebuild
the inherited HEAD256 frontend. Reconstructs the new organ from retained
source/target evidence and supplied pretruth recipient probabilities: the
branch prefix contents come back out of the archived rules with this file's
own expansion code, and the seven immediate-continuation counts come back out
of the source tapes with this file's own rolling-code recount.
"""
import argparse
from collections import Counter
import csv
from decimal import Decimal, localcontext
import gzip
import hashlib
import json
import math
from pathlib import Path
import struct

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
WORLDS = tuple(range(128, 136))
REGIMES = ('recombined', 'unrelated', 'switched')
SOURCES = ('sourceAB', 'sourceBA', 'sourceCD', 'sourceDC')
ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat', 'row')
MATCH_ARMS = ('episode', 'isolated', 'frequency', 'reverse', 'permuted', 'flat')
N = 16384
TOL = 1e-7
MAX_RULES = 32
ARCHIVE_CAP = 528
MAX_COUNTER = 0xffff
PROTOCOL_SHA = 'ec37980efb94865e6699cb8abe954fddf779924fc3fa5c2fa5f15d659a9594d4'


def need(condition, label):
    if not condition:
        raise AssertionError(label)


def near(actual, expected, label, tolerance=TOL):
    a, b = float(actual), float(expected)
    need(math.isfinite(a) and math.isfinite(b), label+' finite')
    error = abs(a-b)
    need(error <= tolerance, f'{label}: {a} != {b}, error={error}')
    return error


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda: stream.read(1 << 20), b''):
            h.update(part)
    return h.hexdigest()


def mass_log2(value):
    need(value > 0, 'positive mass for odds')
    simple = float(value)
    if simple >= 2.0**-1022 and math.isfinite(simple):
        return math.log2(simple)
    exponent = value.adjusted()
    return math.log2(float(value.scaleb(-exponent)))+exponent*math.log2(10)


def patterns():
    result = []
    def visit(prefix, maximum):
        if len(prefix) == 6:
            result.append(''.join(map(str, prefix)))
            return
        for value in range(maximum+2):
            visit(prefix+(value,), max(maximum, value))
    visit((0,), 0)
    need(len(result) == 203, 'Bell(6) rows')
    return tuple(result)


PATTERNS = patterns()
K = {p: max(map(int, p))+1 for p in PATTERNS}
CELLS = sum(K[p]+1 for p in PATTERNS)
need(CELLS == 877, 'row cell count')


def bindings(pattern, heads, truth):
    """Rank is recency of unique heads, not forward RGS class order."""
    if pattern == '-':
        need(heads == [], 'incomplete context has no role bindings')
        return 0, -1, []
    need(pattern in K and len(heads) == K[pattern] and
         len(set(heads)) == len(heads), 'canonical context and unique heads')
    need(all(0 <= x <= 255 for x in heads), 'byte-valued heads')
    recent_classes = list(dict.fromkeys(map(int, reversed(pattern))))
    mapping = dict(zip(recent_classes, heads))
    forward = [mapping[j] for j in range(K[pattern])]
    rank = heads.index(truth)+1 if truth in heads else 0
    group = forward.index(truth) if truth in forward else K[pattern]
    return rank, group, forward


def heads_from_field(text):
    return [] if text in ('', '-') else list(map(int, text.split(',')))


def mine_rules(tapes):
    """Non-overlapping token-pair counts, followed by overlapping recount."""
    streams = [list(tape) for tape in tapes]
    expansions = [(i,) for i in range(7)]
    children = []
    merge_supports = []
    for _ in range(MAX_RULES):
        support = Counter()
        for stream in streams:
            next_allowed = {}
            for i in range(len(stream)-1):
                pair = (stream[i], stream[i+1])
                if len(expansions[pair[0]])+len(expansions[pair[1]]) > 32:
                    continue
                if i >= next_allowed.get(pair, 0):
                    support[pair] += 1
                    next_allowed[pair] = i+2
        eligible = [pair for pair, count in support.items() if count >= 16]
        if not eligible:
            break
        pair = min(eligible, key=lambda x: (-support[x], x[0], x[1]))
        new_id = len(expansions)
        children.append(pair)
        merge_supports.append(support[pair])
        expansions.append(expansions[pair[0]]+expansions[pair[1]])
        for n, stream in enumerate(streams):
            merged = []
            i = 0
            while i < len(stream):
                if i+1 < len(stream) and (stream[i], stream[i+1]) == pair:
                    merged.append(new_id)
                    i += 2
                else:
                    merged.append(stream[i])
                    i += 1
            streams[n] = merged
    rules = []
    for index, (left, right) in enumerate(children):
        target = expansions[7+index]
        wanted = content_code(target)
        mask = (1 << (3*len(target)))-1
        count = 0
        for tape in tapes:
            code = 0
            for i, event in enumerate(tape):
                code = ((code << 3) | event) & mask
                if i+1 >= len(target) and code == wanted:
                    count += 1
        rules.append((left, right, count))
    return rules, expansions, merge_supports


def compose(children):
    """Forward expansion of a binary dictionary from its child references."""
    table = [(i,) for i in range(7)]
    for left, right in children:
        table.append(table[left]+table[right])
    return table


def window_counts(tapes, lengths):
    """Rolling base-8 codes recount every window's immediate continuation.

    For each length this walks the tapes once and charges tape[i] to the code
    of tape[i-length:i]. An occurrence with no successor is never charged, so
    a content ending a tape contributes nothing, exactly as a scan that asks
    for the next event would find nothing to ask about.
    """
    tables = {}
    for length in sorted(lengths):
        mask = (1 << (3*length))-1
        table = {}
        for tape in tapes:
            history = 0
            for i, event in enumerate(tape):
                if i >= length:
                    table.setdefault(history, [0]*7)[event] += 1
                history = ((history << 3) | event) & mask
        tables[length] = table
    return tables


def content_code(content):
    value = 0
    for event in content:
        value = (value << 3) | event
    return value


def information(counts, unconditional):
    support, total = sum(counts), sum(unconditional.values())
    need(support > 0 and total > 0, 'source information support')
    return math.fsum(c*math.log2((c/support)/(unconditional[r]/total))
                     for r, c in enumerate(counts) if c)


def prefix_records(expansions, tapes):
    """Canonical owners and all-occurrence counts for every proper prefix."""
    owner = {}
    for rule_id, expansion in enumerate(expansions):
        for length in range(1, len(expansion)):
            owner.setdefault(expansion[:length], rule_id)
    tables = window_counts(tapes, {len(content) for content in owner})
    records = []
    for content, rule_id in owner.items():
        counts = list(tables[len(content)].get(content_code(content), [0]*7))
        need(all(0 <= c <= MAX_COUNTER for c in counts), 'exact branch uint16')
        records.append((sum(counts), rule_id, len(content), content, counts))
    return records


def mine_branches(expansions, tapes, capacity, frequency=False):
    """Inherited isolated-information or frequency allocation."""
    records = prefix_records(expansions, tapes)
    unconditional = Counter(event for tape in tapes for event in tape)
    if frequency:
        records.sort(key=lambda r: (-r[0], r[1], -r[2]))
    else:
        scored = [(information(r[4], unconditional)-128, r)
                  for r in records if r[0] >= 16]
        scored = [(score, r) for score, r in scored if score > 0]
        scored.sort(key=lambda x: (-x[0], -x[1][2], x[1][3]))
        records = [r for _, r in scored]
    return records[:capacity]


def joint_branches(expansions, tapes, capacity):
    """Greedy source-only marginal selection with explicit position ownership.

    Recount positions through rolling codes, separately for each life and
    length. Group changed positions by the current winning record and truth;
    selection changes only the longest-match winners, never source counts.
    """
    records = [r for r in prefix_records(expansions, tapes) if r[0] >= 16]
    events = [event for tape in tapes for event in tape]
    global_counts = Counter(events)
    unconditional = [global_counts[r]/len(events) for r in range(7)]
    positions = {r[3]: [] for r in records}
    by_length = {}
    for record in records:
        by_length.setdefault(record[2], {})[content_code(record[3])] = record[3]
    for length, wanted in by_length.items():
        mask = (1 << (3*length))-1
        offset = 0
        for tape in tapes:
            code = 0
            for i, event in enumerate(tape):
                if i >= length and code in wanted:
                    positions[wanted[code]].append(offset+i)
                code = ((code << 3) | event) & mask
            offset += len(tape)
    for record in records:
        counts = [0]*7
        for pos in positions[record[3]]:
            counts[events[pos]] += 1
        need(counts == record[4], 'joint positions/all-occurrence counts')
    owner = [-1]*len(events)
    winning_length = [0]*len(events)
    selected, steps, conditionals = [], [], []
    remaining = list(records)
    source_gain = 0.0
    while remaining and len(selected) < capacity:
        alternatives = []
        for record in remaining:
            groups = Counter()
            affected = []
            for pos in positions[record[3]]:
                if record[2] > winning_length[pos]:
                    groups[(owner[pos], events[pos])] += 1
                    affected.append(pos)
            conditional = [c/record[0] for c in record[4]]
            terms = []
            for (incumbent, role), count in sorted(groups.items()):
                prior = unconditional if incumbent == -1 else conditionals[incumbent]
                need(conditional[role] > 0 and prior[role] > 0,
                     'positive observed joint likelihood terms')
                terms.append(count*math.log2(conditional[role]/prior[role]))
            gain = math.fsum(terms)-128
            alternatives.append((gain, record, affected, conditional))
        gain, record, affected, conditional = min(
            alternatives, key=lambda x: (-x[0], -x[1][2], x[1][3]))
        if gain <= 0:
            break
        next_owner = len(selected)
        selected.append(record)
        conditionals.append(conditional)
        remaining.remove(record)
        for pos in affected:
            owner[pos], winning_length[pos] = next_owner, record[2]
        source_gain += gain+128
        steps.append(dict(prefix=list(record[3]), marginal_gain=gain,
                          affected_events=len(affected), source_gain_after=source_gain))
    # A second aggregate checks that accumulated marginal changes telescope
    # to the final longest-match empirical source objective.
    final_groups = Counter((index, events[pos]) for pos, index in enumerate(owner)
                           if index >= 0)
    final_gain = math.fsum(count*math.log2(conditionals[index][role]/unconditional[role])
                          for (index, role), count in sorted(final_groups.items()))
    near(final_gain, source_gain, 'joint final source objective telescope')
    return selected, steps


def rotate(counts):
    """Null control rotation: slots 1..6 shift left by one, slot 0 stays."""
    return [counts[0]]+[counts[j % 6+1] for j in range(1, 7)]


def mine_flat(tapes, budget):
    """One context length at a time keeps an independent recount bounded."""
    unconditional = Counter(x for tape in tapes for x in tape)
    total = sum(unconditional.values())
    proposals = []
    for length in range(1, 32):
        mask = (1 << (3*length))-1
        counts = {}
        for tape in tapes:
            history = 0
            for i, truth in enumerate(tape):
                if i >= length:
                    counts.setdefault(history, [0]*7)[truth] += 1
                history = ((history << 3) | truth) & mask
        for code, row in counts.items():
            support = sum(row)
            if support < 16:
                continue
            score = math.fsum(c*math.log2((c/support)/(unconditional[r]/total))
                              for r, c in enumerate(row) if c) - 8*(15+length)
            if score <= 0:
                continue
            context = tuple((code >> (3*i)) & 7 for i in reversed(range(length)))
            proposals.append((score, context, tuple(row)))
    proposals.sort(key=lambda x: (-x[0], -len(x[1]), x[1]))
    used = 16
    chosen = []
    for score, context, row in proposals:
        size = 15+len(context)
        if used+size <= budget:
            chosen.append((context, row))
            used += size
    return chosen, used


def branch_bytes(children, records):
    need(all(0 <= c <= MAX_COUNTER for r in records for c in r[4]),
         'branch counter fits exactly')
    return (struct.pack('<8sII', b'NETEI001', len(children), len(records)) +
            b''.join(struct.pack('<HH', left, right) for left, right in children) +
            b''.join(struct.pack('<BB7H', rule_id, length, *counts)
                     for _, rule_id, length, _, counts in records))


def flat_bytes(entries):
    need(all(0 <= c <= MAX_COUNTER for _, row in entries for c in row),
         'flat counter fits exactly')
    return (struct.pack('<8sII', b'NETFI001', len(entries), 0) +
            b''.join(bytes([len(context)])+bytes(context)+struct.pack('<7H', *row)
                     for context, row in entries))


def decode_flat(data):
    magic, count, reserved = struct.unpack_from('<8sII', data)
    need(magic == b'NETFI001' and reserved == 0, 'literal format')
    records, offset, seen = [], 16, set()
    for _ in range(count):
        length = data[offset]
        need(1 <= length <= 31, 'literal context length')
        content = tuple(data[offset+1:offset+1+length])
        counts = list(struct.unpack_from('<7H', data, offset+1+length))
        need(all(0 <= x <= 6 for x in content) and content not in seen,
             'literal context terminals and uniqueness')
        seen.add(content)
        records.append((content, counts))
        offset += 15+length
    need(offset == len(data), 'literal complete decoding')
    return records


def row_bytes(counts):
    return (struct.pack('<8sII', b'NETHD256', 6, 877) +
            b''.join(struct.pack('<Q', value)
                     for p in PATTERNS for value in counts[p]))


def decode_branches(data):
    """Read the archive back the way the predictor must: tree addressing only."""
    magic, rule_count, record_count = struct.unpack_from('<8sII', data)
    need(magic == b'NETEI001' and rule_count <= MAX_RULES and
         record_count <= (ARCHIVE_CAP-16)//16 and
         len(data) == 16+4*rule_count+16*record_count <= ARCHIVE_CAP,
         'branch archive format')
    rules = []
    expanded = [(i,) for i in range(7)]
    for i in range(rule_count):
        left, right = struct.unpack_from('<HH', data, 16+4*i)
        need(left < 7+i and right < 7+i, 'branch topology')
        expanded.append(expanded[left]+expanded[right])
        need(len(expanded[-1]) <= 32, 'branch expansion length')
        rules.append((left, right))
    records = []
    seen = set()
    base = 16+4*rule_count
    for j in range(record_count):
        fields = struct.unpack_from('<BB7H', data, base+16*j)
        rule_id, length, counts = fields[0], fields[1], list(fields[2:])
        need(rule_id < rule_count and 1 <= length < len(expanded[7+rule_id]),
             'branch record addressing')
        content = expanded[7+rule_id][:length]
        need(content not in seen, 'duplicate branch prefix content')
        seen.add(content)
        records.append((content, counts))
    return rules, expanded, records


def suffix_match(table, history):
    """Greatest stored context length that equals the completed-event suffix."""
    for length in range(min(31, len(history)), 0, -1):
        entry = table.get(tuple(history[-length:]))
        if entry is not None:
            return length, [entry[0]], list(entry[1])
    return 0, [], [0]*7


def candidate_roles(cold_roles, votes):
    """cold_roles[0] is aggregate NEW; repeat ranks are 1..k."""
    k = len(cold_roles)-1
    valid = sum(votes[1:k+1])
    if not k or valid == 0:
        return cold_roles[:]
    repeat_mass = math.fsum(cold_roles[1:])
    return [cold_roles[0]] + [repeat_mass*(votes[r]+.5)/(valid+.5*k)
                             for r in range(1, k+1)]


def row_candidate(cold_roles, pattern, recent_heads, forward_heads, local, source):
    k = len(recent_heads)
    if pattern == '-' or k <= 1:
        return cold_roles[:]
    b, a = local[pattern], source[pattern]
    n, support = sum(b), sum(a)
    if n < 1 or support < 32:
        return cold_roles[:]
    cold_forward = [cold_roles[recent_heads.index(h)+1] for h in forward_heads]
    cold_forward.append(cold_roles[0])
    local_prior = [(x+.5)/(n+.5*(k+1)) for x in b]
    source_prior = [(x+.5)/(support+.5*(k+1)) for x in a]
    warm_prior = [(x+32*p)/(n+32) for x, p in zip(b, source_prior)]
    warm = [p+.5*(w-c) for p, w, c in zip(cold_forward, warm_prior, local_prior)]
    need(all(x > 0 for x in warm), 'positive recovered row mixture')
    factor = math.fsum(cold_roles[1:])/math.fsum(warm[:-1])
    by_head = {h: warm[j]*factor for j, h in enumerate(forward_heads)}
    return [cold_roles[0]] + [by_head[h] for h in recent_heads]


class Outer:
    def __init__(self):
        self.source = self.cold = Decimal('0.5')
        self.hazard = Decimal(1)/Decimal(65536)
        self.active = False
        self.activation = None
        self.shadow = self.gain = 0.0
        self.minimum = self.peak = self.drawdown = 0.0
        self.horizons = {}
        self.candidate_horizons = {}

    def step(self, t, cold, candidate):
        active = self.active
        shadow = self.shadow
        odds = mass_log2(self.source/self.cold) if active else 0.0
        increment = 0.0
        if active:
            joint = self.source*Decimal.from_float(math.exp2(candidate-cold))
            total = self.cold+joint
            increment = math.log2(float(total))
            self.source = (1-self.hazard)*(joint/total)
            self.cold = 1-self.source
            need(self.cold > 0 and self.source > 0, 'positive authority masses')
            near(self.cold+self.source, 1, 'authority normalization', 1e-12)
        self.gain += increment
        self.shadow += candidate-cold
        activated = False
        if not self.active and self.shadow >= 32:
            self.active = True
            self.activation = t+1
            self.source = self.cold = Decimal('0.5')
            activated = True
        self.minimum = min(self.minimum, self.gain)
        self.peak = max(self.peak, self.gain)
        self.drawdown = max(self.drawdown, self.peak-self.gain)
        need(self.minimum >= -1-TOL and self.drawdown <= 16+TOL,
             'one/sixteen-bit authority bounds')
        if t+1 in (1024, 4096, 8192, N):
            self.horizons[str(t+1)] = self.gain
            self.candidate_horizons[str(t+1)] = self.shadow
        return dict(candidate=candidate, live=cold+increment, shadow_before=shadow,
                    odds_before=odds, active_before=int(active),
                    activated_after=int(activated), gain_after=self.gain)

    def result(self):
        return dict(gain=self.gain, candidate_gain=self.shadow,
                    tail=self.gain-self.horizons['8192'],
                    horizons=self.horizons,
                    minimum=self.minimum, peak=self.peak, max_drawdown=self.drawdown,
                    activation=self.activation)


def verify_books(world):
    folder = f'world{world:02d}'
    source_rows = {p: [0]*(K[p]+1) for p in PATTERNS}
    tapes = []
    for name in SOURCES:
        raw = (HERE/'data'/folder/(name+'.bin')).read_bytes()
        need(len(raw) == N, 'source exposure')
        tape = []
        with gzip.open(HERE/'data'/folder/(name+'.tsv.gz'), 'rt') as stream:
            rows = csv.DictReader(stream, delimiter='\t')
            for t in range(N):
                row = next(rows, None)
                need(row is not None and int(row['t']) == t and
                     int(row['truth']) == raw[t], 'source chronology')
                heads = heads_from_field(row['heads'])
                need(int(row['k']) == len(heads), 'source binding count')
                rank, group, _ = bindings(row['pattern'], heads, raw[t])
                need(int(row['rank']) == rank, 'source observed role')
                tape.append(rank)
                if row['pattern'] != '-':
                    source_rows[row['pattern']][group] += 1
            need(next(rows, None) is None, 'complete source trace')
        tapes.append(tape)
    rules, expanded, merge_supports = mine_rules(tapes)
    need(len(rules) <= MAX_RULES, 'thirty-two rule dictionary cap')
    children = [(left, right) for left, right, _ in rules]
    swapped = [(right, left) for left, right, _ in rules]
    # A swapped binary DAG reverses every terminal expansion independently
    # of the writer's string operations.
    mirrored = compose(swapped)
    need(all(a == tuple(reversed(b)) for a, b in zip(mirrored[7:], expanded[7:])),
         'reversal of ordered composition')
    need(compose(children) == expanded, 'forward composition agrees with mining')
    capacity = (ARCHIVE_CAP-16-4*len(rules))//16
    forward_records, joint_forward = joint_branches(expanded[7:], tapes, capacity)
    isolated_records = mine_branches(expanded[7:], tapes, capacity)
    frequency_records = mine_branches(expanded[7:], tapes, capacity, frequency=True)
    reverse_records, joint_reverse = joint_branches(mirrored[7:], tapes, capacity)
    permuted_records = [(total, rule_id, length, content, rotate(counts))
                        for total, rule_id, length, content, counts in forward_records]
    expected = {'episodes.bin': branch_bytes(children, forward_records),
                'isolated.bin': branch_bytes(children, isolated_records),
                'frequency.bin': branch_bytes(children, frequency_records),
                'reverse.bin': branch_bytes(swapped, reverse_records),
                'permuted.bin': branch_bytes(children, permuted_records),
                'row.bin': row_bytes(source_rows)}
    entries, flat_size = mine_flat(tapes, ARCHIVE_CAP)
    expected['flat.bin'] = flat_bytes(entries)
    memory = HERE/'memory'/folder
    for name, data in expected.items():
        need((memory/name).read_bytes() == data, 'source-rebuilt exact '+name)
    book_metadata = json.loads((memory/'BOOKS.json').read_text())
    compare_tree(book_metadata['joint_forward'], joint_forward, 'joint forward selection')
    compare_tree(book_metadata['joint_reverse'], joint_reverse, 'joint reverse selection')
    need(all(len(data) <= ARCHIVE_CAP for name, data in expected.items()
             if name != 'row.bin') and
         len(expected['permuted.bin']) == len(expected['episodes.bin']) and
         len(expected['flat.bin']) == flat_size and
         len(expected['row.bin']) == 7032, 'common portable byte budgets')
    tables, literal_sizes, cache_payloads = {}, {}, {}
    for arm, rebuilt in (('episode', (children, forward_records)),
                         ('isolated', (children, isolated_records)),
                         ('frequency', (children, frequency_records)),
                         ('reverse', (swapped, reverse_records)),
                         ('permuted', (children, permuted_records))):
        decoded_rules, decoded_expanded, decoded_records = decode_branches(
            expected[('episodes.bin' if arm == 'episode' else arm+'.bin')])
        need(decoded_rules == rebuilt[0], 'archive rule copy '+arm)
        need(decoded_expanded == compose(rebuilt[0]), 'archive expanded copy '+arm)
        need(decoded_records == [(content, counts)
                                 for _, _, _, content, counts in rebuilt[1]],
             'archive branch copy '+arm)
        # Re-encode the SAME selected records explicitly, then decode them.
        # Forecast replay below consumes this independently expanded literal
        # table, checking C tree matches, votes and every received price.
        literal = flat_bytes(decoded_records)
        literal_sizes[arm] = 16+sum(15+len(content) for content, _ in decoded_records)
        need(len(literal) == literal_sizes[arm], 'literal selected-record cost '+arm)
        literal_records = decode_flat(literal)
        need(literal_records == decoded_records, 'literal selected-record identity '+arm)
        tables[arm] = {content: (i, counts)
                       for i, (content, counts) in enumerate(literal_records)}
        cache_payloads[arm] = dict(expanded_rule_terminal_bytes=
                                      sum(map(len, decoded_expanded[7:])),
                                  selected_prefix_terminal_bytes=
                                      sum(len(content) for content, _ in decoded_records))
    flat_records = decode_flat(expected['flat.bin'])
    need(flat_records == [(context, list(counts)) for context, counts in entries],
         'literal flat archive identity')
    tables['flat'] = {context: (i, counts)
                     for i, (context, counts) in enumerate(flat_records)}
    artifacts = dict(source_bytes=sum(map(len, tapes)),
                     source_roles=[dict(sorted(Counter(t).items())) for t in tapes],
                     rules=len(rules), rule_merge_supports=merge_supports,
                     rule_overlapping_supports=[support for _, _, support in rules],
                     expanded_rule_lengths=[len(x) for x in expanded[7:]],
                     branch_capacity=capacity,
                     branch_records=len(forward_records),
                     isolated_records=len(isolated_records),
                     frequency_records=len(frequency_records),
                     reverse_records=len(reverse_records),
                     branch_prefix_lengths=dict(sorted(
                         Counter(str(r[2]) for r in forward_records).items())),
                     branch_totals=[r[0] for r in forward_records],
                     isolated_branch_totals=[r[0] for r in isolated_records],
                     joint_forward=joint_forward, joint_reverse=joint_reverse,
                     frequency_branch_totals=[r[0] for r in frequency_records],
                     reverse_branch_totals=[r[0] for r in reverse_records],
                     portable_cap=ARCHIVE_CAP,
                     episode_bytes=len(expected['episodes.bin']),
                     isolated_bytes=len(expected['isolated.bin']),
                     frequency_bytes=len(expected['frequency.bin']),
                     reverse_bytes=len(expected['reverse.bin']),
                     permuted_bytes=len(expected['permuted.bin']),
                     flat_bytes=len(expected['flat.bin']),
                     row_bytes=len(expected['row.bin']),
                     expanded_literal_bytes=literal_sizes,
                     expansion_content_payloads=cache_payloads,
                     hashes={name: digest(memory/name) for name in sorted(expected)})
    return tables, source_rows, artifacts


def verify_life(world, regime, books):
    tables, source_counts, _ = books
    folder = f'world{world:02d}'
    raw = (HERE/'data'/folder/(regime+'.bin')).read_bytes()
    need(len(raw) == N, 'target horizon')
    if regime == 'switched':
        recombined = (HERE/'data'/folder/'recombined.bin').read_bytes()
        need(raw[:8192] == recombined[:8192], 'identical raw prefix')
    path = HERE/'results'/folder/(regime+'.tsv.gz')
    local = {p: [0]*(K[p]+1) for p in PATTERNS}
    history = []
    states = {arm: Outer() for arm in ARMS}
    matched = {a: 0 for a in ARMS}
    maximum_lengths = {a: 0 for a in ARMS}
    episode_lengths = Counter()
    error = norm_error = 0.0
    predictions = 0
    masked_longest = {a: 0 for a in MATCH_ARMS}

    def check(actual, expected, label, tolerance=TOL):
        nonlocal error
        error = max(error, near(actual, expected, label, tolerance))

    with gzip.open(path, 'rt') as stream:
        rows = csv.DictReader(stream, delimiter='\t')
        for t in range(N):
            row = next(rows, None)
            need(row is not None and int(row['t']) == t and
                 int(row['truth']) == raw[t], 'complete target chronology')
            heads = heads_from_field(row['heads'])
            need(int(row['k']) == len(heads), 'target binding count')
            rank, group, forward = bindings(row['pattern'], heads, raw[t])
            need(int(row['rank']) == rank, 'observed target role')
            observed_history = ''.join(map(str, history)) if history else '-'
            need(row['history'] == observed_history, 'immutable pretruth relation history')
            quoted_heads = ([] if row['cold_heads'] in ('-', '') else
                            list(map(float, row['cold_heads'].split(','))))
            need(len(quoted_heads) == len(heads), 'pretruth cold role masses')
            cold_repeats = [math.exp2(x) for x in quoted_heads]
            need(all(math.isfinite(x) and x > 0 for x in cold_repeats),
                 'positive supplied P0 repeat probabilities')
            cold_roles = [1-math.fsum(cold_repeats)] + cold_repeats
            need(0 < cold_roles[0] <= 1, 'positive aggregate P0 NEW mass')
            cold_truth = float(row['logcold'])
            need(math.isfinite(cold_truth) and cold_truth <= 0, 'positive P0 truth price')
            if rank:
                check(cold_truth, quoted_heads[rank-1], 'current rank price', 1e-10)
            else:
                need(math.exp2(cold_truth) <= cold_roles[0]+1e-12,
                     'NEW byte contained in NEW mass')
            need(int(row['new_exact']) == 1, 'C all-byte protected NEW check')
            c_norm = float(row['max_norm_error'])
            need(math.isfinite(c_norm) and 0 <= c_norm <= 1e-8,
                 'C full-vector normalization check')
            norm_error = max(norm_error, c_norm)
            for arm in ARMS:
                if arm == 'row':
                    candidate = row_candidate(cold_roles, row['pattern'], heads,
                                              forward, local, source_counts)
                else:
                    length, matches, votes = suffix_match(tables[arm], history)
                    need(int(row[arm+'_matchedL']) == length, 'pretruth longest '+arm)
                    need(heads_from_field(row[arm+'_matches']) == matches,
                         'stored record identity '+arm)
                    actual_votes = list(map(int, row[arm+'_votes'].split(',')))
                    need(actual_votes == votes, 'exact stored branch votes '+arm)
                    matched[arm] += int(length > 0)
                    maximum_lengths[arm] = max(maximum_lengths[arm], length)
                    if arm == 'episode':
                        episode_lengths[str(length)] += 1
                    if length and not sum(votes[1:len(heads)+1]):
                        masked_longest[arm] += 1
                    candidate = candidate_roles(cold_roles, votes)
                need(all(math.isfinite(x) and x > 0 for x in candidate),
                     'positive reconstructed candidate groups')
                norm = abs(math.fsum(candidate)-1)
                need(norm <= 1e-8, 'reconstructed candidate normalization')
                norm_error = max(norm_error, norm)
                need(candidate[0] == cold_roles[0], 'exact reconstructed NEW mass')
                candidate_truth = math.log2(candidate[rank]) if rank else cold_truth
                wanted = states[arm].step(t, cold_truth, candidate_truth)
                for name, value in wanted.items():
                    actual = row[arm+'_'+name]
                    if name in ('active_before', 'activated_after'):
                        need(int(actual) == value, 'prospective admission '+arm+'/'+name)
                    else:
                        check(actual, value, 'prediction '+arm+'/'+name)
                if not rank:
                    need(float(row[arm+'_candidate']) == cold_truth,
                         'observed NEW byte candidate exactly P0')
                predictions += 1
            # Only after prices and admission updates does the event enter memory.
            if row['pattern'] != '-':
                local[row['pattern']][group] += 1
            history.append(rank)
            if len(history) > 32:
                del history[0]
        need(next(rows, None) is None, 'no surplus target events')
    arms = {}
    for arm in ARMS:
        arms[arm] = states[arm].result()
        arms[arm].update(matched_events=matched[arm],
                         max_matched_length=maximum_lengths[arm])
    life = dict(world=world, regime=regime, arms=arms, trace_sha256=digest(path),
                episode_match_lengths=dict(episode_lengths))
    return life, predictions, error, norm_error, masked_longest


def compare_tree(actual, expected, label):
    if isinstance(expected, dict):
        need(isinstance(actual, dict), label+' object')
        for key, value in expected.items():
            need(key in actual, label+' missing '+key)
            compare_tree(actual[key], value, label+'/'+key)
    elif isinstance(expected, list):
        need(isinstance(actual, list) and len(actual) == len(expected), label+' list')
        for i, (a, b) in enumerate(zip(actual, expected)):
            compare_tree(a, b, label+f'/{i}')
    elif isinstance(expected, (bool, str, int)) or expected is None:
        need(actual == expected, label+' exact')
    else:
        near(actual, expected, label)


def identity():
    need(digest(HERE/'PROTOCOL.md') == PROTOCOL_SHA, 'frozen protocol')
    frozen = json.loads((HERE/'FREEZE.json').read_text())
    for name, wanted in frozen['files'].items():
        need(digest(REPO/name) == wanted, 'frozen code '+name)
    identities = {'protocol_sha256': PROTOCOL_SHA,
                  'freeze_sha256': digest(HERE/'FREEZE.json'),
                  'result_sha256': digest(HERE/'RESULT.json'),
                  'reader_sha256': digest(Path(__file__).resolve())}
    for name in ('DATA_MANIFEST.json', 'MEMORY_MANIFEST.json', 'RESULTS_MANIFEST.json'):
        manifest = json.loads((HERE/name).read_text())
        for path, wanted in manifest.items():
            need(digest(HERE/path) == wanted, 'retained artifact '+path)
        identities[name] = digest(HERE/name)
    return identities


def gate(lives):
    mean = lambda values: math.fsum(values)/len(values)
    by_regime = {r: [x for x in lives if x['regime'] == r] for r in REGIMES}
    for regime, rows in by_regime.items():
        need([x['world'] for x in rows] == list(WORLDS), 'world coverage '+regime)
    means = {r: {arm: {key: mean([x['arms'][arm][key] for x in rows])
                       for key in ('gain', 'tail', 'candidate_gain')}
                  for arm in ARMS} for r, rows in by_regime.items()}
    recombined = by_regime['recombined']
    switched = by_regime['switched']
    early = [x['arms']['episode']['horizons']['4096'] for x in recombined]
    differences = {arm: [x['arms']['episode']['horizons']['4096']-
                         x['arms'][arm]['horizons']['4096'] for x in recombined]
                   for arm in ('frequency', 'row', 'reverse', 'permuted', 'flat')}
    switched_differences = [x['arms']['episode']['gain']-x['arms']['row']['gain']
                            for x in switched]
    tail_differences = [x['arms']['episode']['tail']-x['arms']['row']['tail']
                       for x in switched]
    summary = dict(means=means, early_gain_bpb=mean(early)/4096,
                   early_gains=early, early_differences=differences,
                   switched_differences=switched_differences,
                   tail_differences=tail_differences)
    isolated = {}
    for regime, rows in by_regime.items():
        paired = {key: [x['arms']['episode'][key]-x['arms']['isolated'][key]
                        for x in rows] for key in ('gain', 'tail', 'candidate_gain')}
        paired['horizons'] = {str(h): [x['arms']['episode']['horizons'][str(h)]-
                                      x['arms']['isolated']['horizons'][str(h)]
                                      for x in rows]
                              for h in (1024, 4096, 8192, N)}
        paired['early'] = paired['horizons']['4096']
        isolated[regime] = paired
    summary['isolated_differences'] = isolated
    tests = dict(early_gain=summary['early_gain_bpb'] >= .005,
                 early_positive=sum(x > 0 for x in early) >= 6,
                 full_gain=means['recombined']['episode']['gain']/N >= .005,
                 full_positive=all(x['arms']['episode']['gain'] > 0 for x in recombined),
                 switched_mean=mean(switched_differences) >= 0,
                 switched_count=sum(x > 0 for x in switched_differences) >= 5,
                 tail_tolerance=mean(tail_differences) >= -1)
    for arm, values in differences.items():
        tests[arm+'_early_mean'] = mean(values) > 1
        tests[arm+'_early_count'] = sum(x > 0 for x in values) >= 5
    need(len(tests) == 17, 'seventeen material conditions')
    return summary, tests


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=HERE/'VERIFY.json')
    args = parser.parse_args()
    need(not args.output.exists(), 'verification output already exists')
    identities = identity()
    lives = []
    source_books = []
    masked = []
    predictions = 0
    maximum_error = maximum_norm_error = 0.0
    with localcontext() as context:
        context.prec = 50
        for world in WORLDS:
            books = verify_books(world)
            source_books.append(dict(world=world, **books[-1]))
            for regime in REGIMES:
                life, count, error, norm, masked_counts = verify_life(world, regime, books)
                lives.append(life)
                predictions += count
                maximum_error = max(maximum_error, error)
                maximum_norm_error = max(maximum_norm_error, norm)
                masked.append(dict(world=world, regime=regime, arms=masked_counts))
    need(predictions == 8*3*N*7, 'all 2752512 forecasts')
    claim = json.loads((HERE/'RESULT.json').read_text())
    need(claim['worlds'] == list(WORLDS), 'declared world IDs')
    need(claim['namespace'] == 'netta-joint-prefix-v1', 'declared seed namespace')
    need(claim['protocol_sha256'] == PROTOCOL_SHA, 'result protocol')
    compare_tree(claim['lives'], lives, 'lives')
    for life in claim['lives']:
        need(0 <= life['max_norm_error'] <= 1e-8, 'claimed full-vector normalization')
    summary, tests = gate(lives)
    compare_tree(claim['summary'], summary, 'summary')
    need(claim['tests'] == tests and claim['gate_pass'] == all(tests.values()),
         'all seventeen material conditions agree')
    result = dict(verification_pass=True, gate_pass=all(tests.values()),
                  identities=identities, source_observations=8*4*N,
                  target_observations=8*3*N, predictions=predictions,
                  maximum_numeric_error=maximum_error,
                  maximum_normalization_error=maximum_norm_error,
                  source_books=source_books, masked_longest_matches=masked,
                  summary=summary, tests=tests,
                  scope='Independent source BPE/overlapping recount, own rolling-code '
                        'recount of the seven immediate-continuation counts at every '
                        'own-prefix content, exact archives for all seven arms; '
                        'complete greedy joint source selection, isolated/frequency and flat MDL; '
                        'expanded literal copies matching tree records, actual role tapes, '
                        'stored matches, all rank probabilities, row P2, prospective authority '
                        'and material gates. Inherited HEAD256 bindings and sparse P0 are '
                        'supplied, not independently retrained; full 256-byte normalization/NEW '
                        'checks come from the C quote path.')
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
