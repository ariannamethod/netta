#!/usr/bin/env python3
"""Prefreeze source fixture: handwritten role tapes, no worlds or predictor."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def require(condition, name):
    if not condition:
        raise AssertionError(name)


def independent_counts(prefix, tapes):
    result = [[0]*7, [0]*7]
    for life in range(4):
        values = list(tapes[life])
        # Explicit windows, including overlaps, with truth still in this life.
        for truth_index in range(len(prefix), len(values)):
            if values[truth_index-len(prefix):truth_index] == list(prefix):
                result[life//2][values[truth_index]] += 1
    return result


def load_archive(data, bank):
    magic = b'NETEB001' if bank else b'NETEI001'
    require(data[:8] == magic, 'archive magic')
    read = lambda offset, width: int.from_bytes(data[offset:offset+width], 'little')
    nrules, nrecords = read(8, 4), read(12, 4)
    size = 32 if bank else 16
    require(len(data) == 16+4*nrules+size*nrecords <= 528, 'actual archive length')
    expanded = [[x] for x in range(7)]
    children = []
    for j in range(nrules):
        a, b = read(16+4*j, 2), read(18+4*j, 2)
        require(a < 7+j and b < 7+j, 'forward-only dictionary reference')
        expanded.append(expanded[a]+expanded[b])
        require(len(expanded[-1]) <= 32, 'dictionary expansion horizon')
        children.append([a, b])
    records = []
    for j in range(nrecords):
        start = 16+4*nrules+size*j
        owner, length = data[start], data[start+1]
        require(owner < nrules and 0 < length < len(expanded[7+owner]), 'proper prefix')
        if bank:
            require(read(start+2, 2) == 0, 'reserved u16 zero')
        offset = start+(4 if bank else 2)
        counts = [read(offset+2*k, 2) for k in range(14 if bank else 7)]
        records.append(dict(rule_id=owner, prefix_len=length,
            prefix=expanded[7+owner][:length], counts=counts))
    require(len({tuple(r['prefix']) for r in records}) == len(records), 'unique prefix contents')
    return children, records


def main():
    spec = importlib.util.spec_from_file_location('turn28_source_fixture_writer', HERE/'experiment.py')
    writer = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = writer
    spec.loader.exec_module(writer)

    def forbidden(*args, **kwargs):
        raise AssertionError('source fixture attempted generation, extraction, or prediction')

    for module, names in ((writer, ('generate_world','generate','extract','learn','evaluate','run_target')),
                          (writer.parent, ('generate_world',)),
                          (writer.t13, ('generate_world','emit','extract_world','source_tapes','run_target'))):
        for name in names:
            setattr(module, name, forbidden)

    short = [bytes([0,0,0]), bytes([1,0,0,1]), bytes([0,0,2]), bytes([0,0,0,2])]
    expected = [[1,1,0,0,0,0,0], [1,0,2,0,0,0,0]]
    require(writer.split_counts([0,0], short) == expected, 'literal overlap/boundary/case fixture')
    checked = 0
    for prefix in ([0], [0,0], [0,0,0], [1,0], [0,2], [6,6]):
        require(writer.split_counts(prefix, short) == independent_counts(prefix, short),
                'independent immediate-successor count '+str(prefix))
        checked += 1

    # Repetition makes the inherited 128-bit source-selection charge payable.
    # No random generator, raw byte world, source frontend, or recipient runs.
    tapes = [bytes([0,0,0,1])*128+bytes([6,6]), bytes([0,0,0,1])*128,
             bytes([0,0,0,2])*128, bytes([0,0,0,2])*128]
    archives, metadata = writer.build_memory(tapes)
    require(set(archives) == {'bank.bin','permuted.bin','pooled_full.bin','pooled_small.bin'},
            'four source archive names')
    children, bank = load_archive(archives['bank.bin'], True)
    perm_children, perm = load_archive(archives['permuted.bin'], True)
    full_children, full = load_archive(archives['pooled_full.bin'], False)
    small_children, small = load_archive(archives['pooled_small.bin'], False)
    require(children == perm_children == full_children == small_children, 'shared dictionary')
    require(children == [[r['left'],r['right']] for r in metadata['rules']], 'rule metadata')
    capacity = (528-16-4*len(children))//32
    require(len(bank) == len(small) == len(perm) == min(capacity, len(full)) > 0,
            'nonempty first-capacity projection')
    require(full == [{k:r[k] for k in ('rule_id','prefix_len','prefix','counts')}
                     for r in metadata['selected']], 'full greedy-order serialization')
    require(small == full[:capacity], 'no prefix reselection or sorting')
    differing_cases = 0
    for j, record in enumerate(bank):
        expected = independent_counts(record['prefix'], tapes)
        require(record['counts'] == expected[0]+expected[1], 'serialized distinct case counts')
        summed = [expected[0][k]+expected[1][k] for k in range(7)]
        require(small[j]['counts'] == summed, 'pooled small equals A+B')
        require(record['prefix'] == small[j]['prefix'] == perm[j]['prefix'], 'address identity')
        require(metadata['bank_selected'][j]['book_counts'] == expected, 'book count metadata')
        rotated = [c[0:1]+c[2:7]+c[1:2] for c in expected]
        require(perm[j]['counts'] == rotated[0]+rotated[1], 'joint repeat permutation and NEW retention')
        differing_cases += expected[0] != expected[1]
        checked += 1
    require(differing_cases > 0, 'fixture actually separates cases')
    for record in full:
        counts = independent_counts(record['prefix'], tapes)
        require(record['counts'] == [counts[0][k]+counts[1][k] for k in range(7)],
                'full archive independent recount')
    require(metadata['source_split'] == [['sourceAB','sourceBA'],['sourceCD','sourceDC']],
            'source-list half provenance')
    print(json.dumps(dict(source_fixture_pass=True, fresh_worlds_generated=0,
        recipient_predictions=0, handwritten_role_events=sum(map(len,tapes)),
        independent_prefix_counts_checked=checked, rules=len(children),
        selected=len(full), bank_selected=len(bank), differing_case_records=differing_cases,
        archive_bytes={name:len(data) for name,data in archives.items()},
        archive_sha256={name:hashlib.sha256(data).hexdigest() for name,data in archives.items()},
        writer_sha256=hashlib.sha256((HERE/'experiment.py').read_bytes()).hexdigest()),
        indent=2, sort_keys=True))


if __name__=='__main__':
    main()
