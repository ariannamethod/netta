#!/usr/bin/env python3
"""NETTA ZERO -- Python version: reference transliteration of netta.c.

Not the organism; netta.c stays the sole developing heart. Written for the
paper, judged by the same external hands and by byte-for-byte state and
biography equality with the C body. Stdlib only; tanhf and fmaf come from
the platform libm via ctypes. Bit-identity mirrors the canonical build's
float codegen (cc -O2, arm64 clang): fused fmadd chain over embeddings,
serial round-per-product dot sums, elementwise fmaf readout update, double
fma in the prophecy baseline -- read from the disassembly, pinned by the
acceptance battery; another compiler may drift in final float ulps.
"""

import ctypes
import ctypes.util
import errno as errno_mod
import math
import os
import stat as stat_mod
import struct
import sys
import tempfile

# ------------------------------------------------------------------ f32

_F32 = struct.Struct('<f')

def f32(x):
    return _F32.unpack(_F32.pack(x))[0]

_libm = ctypes.CDLL(ctypes.util.find_library('m') or None)
_tanhf = _libm.tanhf
_tanhf.restype = ctypes.c_float
_tanhf.argtypes = [ctypes.c_float]

def tanhf(x):
    return _tanhf(x)

_fmaf = _libm.fmaf
_fmaf.restype = ctypes.c_float
_fmaf.argtypes = [ctypes.c_float, ctypes.c_float, ctypes.c_float]

def fmaf(x, y, z):
    return _fmaf(x, y, z)

MASK64 = 0xFFFFFFFFFFFFFFFF

CTX = 16
MAX_ISLANDS = 32
MAX_REGISTRY = 1024
ACTIONS = 256
STATE_MAGIC = b"NETTAZR0"
STATE_VER = 20

MAX_UNITS = 4096
UNIT_MAX_LEN = 16
BIRTH_SUPPORT = 64
UNIT_TTL = 16384
PAIR_SLOTS = 1 << 18

def die(msg):
    sys.stderr.write(msg)
    sys.stderr.flush()
    sys.exit(1)

# ----------------------------------------------------------------- hash

FNV_SEED = 0xcbf29ce484222325
FNV_WITNESS_SEED = 0x6c62272e07bb0142

def fnv1a64(data, h):
    for b in data:
        h ^= b
        h = (h * 0x100000001b3) & MASK64
    return h

def fnv1a64_reverse(data, h):
    for i in range(len(data) - 1, -1, -1):
        h ^= data[i]
        h = (h * 0x100000001b3) & MASK64
    return h

# ------------------------------------------------------------------ rng

rng_state = 0

def rng_next():
    global rng_state
    rng_state = (rng_state + 0x9e3779b97f4a7c15) & MASK64
    z = rng_state
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & MASK64
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & MASK64
    return z ^ (z >> 31)

# ---------------------------------------------------------------- world

class Island:
    __slots__ = ('bytes', 'len', 'digest', 'witness', 'name')

islands = []
reg_digest = [0] * MAX_REGISTRY
reg_witness = [0] * MAX_REGISTRY
reg_len = [0] * MAX_REGISTRY
reg_count = 0
episode_no = 0
atomic_bytes_lived = 0
fixed_start_set = 0
fixed_start = 0

def island_load(path):
    if len(islands) >= MAX_ISLANDS:
        die("netta: too many islands (max %d)\n" % MAX_ISLANDS)
    try:
        f = open(path, 'rb')
    except OSError:
        die("netta: cannot open %s\n" % path)
    data = f.read()
    f.close()
    isl = Island()
    isl.bytes = data
    isl.len = len(data)
    isl.digest = fnv1a64(data, FNV_SEED)
    isl.witness = fnv1a64_reverse(data, FNV_WITNESS_SEED)
    isl.name = path
    islands.append(isl)

# ----------------------------------------------------------- biography

bio_file = None
bio_chain = FNV_SEED
bio_lines = 0

def bio_lower_hex(s, n):
    if len(s) != n:
        return 0
    for c in s:
        if not (('0' <= c <= '9') or ('a' <= c <= 'f')):
            return 0
    return 1

def bio_canonical_u64(s):
    """returns parsed value or None (C: bio_canonical_u64)"""
    if not s or s[0] in '-+':
        return None
    if not s.isascii() or not s.isdigit():
        return None
    if len(s) > 20:
        return None
    v = int(s)
    if v > MASK64:
        return None
    if str(v) != s:
        return None
    return v

def bio_signature_ok(line):
    # line: str without trailing newline check handled by caller passing raw
    raw = line
    if not raw.endswith('\n'):
        return 0
    body = raw[:-1]
    f = body.split('\t')
    if len(f) != 9 or f[0] != 's':
        return 0
    ep = bio_canonical_u64(f[1])
    bts = bio_canonical_u64(f[3])
    seed = bio_canonical_u64(f[4])
    lived = bio_canonical_u64(f[8])
    if (ep is None or not bio_lower_hex(f[2], 16) or bts is None or
            bts == 0 or seed is None or
            f[5] not in ('uni', 'bi', 'tri') or
            f[6] not in ('supported-backoff', 'laplace-red') or
            not bio_lower_hex(f[7], 4) or lived is None or lived == 0 or
            ep > episode_no or lived > atomic_bytes_lived):
        return 0
    canonical = "s\t%d\t%s\t%d\t%d\t%s\t%s\t%s\t%d\n" % (
        ep, f[2], bts, seed, f[5], f[6], f[7], lived)
    return 1 if canonical == raw else 0

def bio_arrival_ok(line, expected):
    raw = line
    if not raw.endswith('\n'):
        return 0
    body = raw[:-1]
    f = body.split('\t')
    if len(f) != 6 or f[0] != 'i':
        return 0
    ep = bio_canonical_u64(f[1])
    iid = bio_canonical_u64(f[2])
    ln = bio_canonical_u64(f[5])
    if (ep is None or iid is None or iid > 1023 or
            not bio_lower_hex(f[3], 16) or not bio_lower_hex(f[4], 16) or
            ln is None or iid != expected or iid >= reg_count or
            ep > episode_no):
        return 0
    digest = int(f[3], 16)
    witness = int(f[4], 16)
    if (digest != reg_digest[iid] or witness != reg_witness[iid] or
            ln != reg_len[iid]):
        return 0
    canonical = "i\t%d\t%d\t%s\t%s\t%d\n" % (ep, iid, f[3], f[4], ln)
    return 1 if canonical == raw else 0

def bio_u64_shape(s):
    return bio_canonical_u64(s) is not None

def bio_u64_max(s, mx):
    v = bio_canonical_u64(s)
    return v is not None and v <= mx

def bio_fix6_shape(s):
    dot = s.find('.')
    if dot <= 0 or len(s) - dot - 1 != 6:
        return 0
    for c in s[dot + 1:]:
        if not ('0' <= c <= '9'):
            return 0
    whole = s[:dot]
    if len(whole) > 20:
        return 0
    return bio_u64_shape(whole)

def bio_fix6_max8(s):
    if not bio_fix6_shape(s) or (len(s) < 2 or s[1] != '.'):
        return 0
    return s[0] < '8' or (s[0] == '8' and s[1:] == '.000000')

def bio_actor_word(s):
    return s in ('uni', 'bi', 'tri', 'mv', 'null')

def bio_byte_actor_word(s):
    return s in ('uni', 'bi', 'tri')

def bio_seated_word(s):
    return bio_byte_actor_word(s) or s == 'mv'

def bio_challenger_word(s):
    return bio_byte_actor_word(s) or s == 'null'

def bio_hexpair_shape(s, nbytes):
    return 2 <= nbytes <= 16 and bio_lower_hex(s, 2 * nbytes)

def bio_grammar_ok(line):
    raw = line
    n = len(raw)
    if n < 2 or n - 1 >= 1024:
        return 0
    body = raw[:-1]
    f = body.split('\t')
    if len(f) > 17:
        return 0
    k = len(f)
    t = f[0]
    if t and '0' <= t[0] <= '9':
        return (k == 12 and bio_u64_shape(f[0]) and bio_u64_shape(f[1]) and
                bio_u64_max(f[2], 1023) and bio_u64_shape(f[3]) and
                bio_lower_hex(f[4], 16) and bio_u64_max(f[5], 255) and
                bio_u64_max(f[6], 255) and bio_fix6_shape(f[7]) and
                bio_lower_hex(f[8], 16) and f[9] == 'atomic' and
                f[10] == '1' and bio_actor_word(f[11]))
    if t == 'a':
        return (k == 3 and bio_u64_shape(f[1]) and
                (bio_actor_word(f[2]) or f[2] == 'mvp'))
    if t == 'b':
        if k != 6 or not bio_u64_shape(f[1]) or not bio_u64_max(f[2], 4095):
            return 0
        ln = bio_canonical_u64(f[3])
        sup = bio_canonical_u64(f[5])
        return (ln is not None and bio_hexpair_shape(f[4], ln) and
                sup is not None and sup >= 64 and sup % 64 == 0)
    if t == 'u':
        if k != 4 or not bio_u64_shape(f[1]) or not bio_u64_max(f[2], 4095):
            return 0
        sup = bio_canonical_u64(f[3])
        return sup is not None and sup >= 64 and sup % 64 == 0
    if t == 'd':
        if (k != 5 or not bio_u64_shape(f[1]) or
                not bio_u64_max(f[2], 4095) or not bio_u64_shape(f[3])):
            return 0
        idle = bio_canonical_u64(f[4])
        return idle is not None and idle >= 16384
    if t == 'm':
        if (k != 7 or not bio_u64_shape(f[1]) or
                not bio_u64_max(f[2], 31) or not bio_u64_shape(f[3]) or
                not bio_u64_max(f[4], 4095)):
            return 0
        ln = bio_canonical_u64(f[5])
        return (ln is not None and 2 <= ln <= 16 and bio_fix6_shape(f[6]))
    if t == 'v':
        if k != 9 and k != 10:
            return 0
        move = bio_canonical_u64(f[4])
        ln = bio_canonical_u64(f[5])
        advance = bio_canonical_u64(f[6])
        target = bio_canonical_u64(f[8])
        if (not bio_u64_shape(f[1]) or not bio_u64_max(f[2], 1023) or
                not bio_u64_shape(f[3]) or move is None or move > 4351 or
                ln is None or ln < 1 or ln > 16 or
                (ln != 1 if move < 256 else ln < 2) or
                advance is None or advance < 1 or advance > ln or
                not bio_fix6_shape(f[7]) or target is None or
                target > 4351):
            return 0
        if k == 9:
            return 1
        policy = bio_canonical_u64(f[9])
        return policy is not None and policy <= 4351
    if t == 't':
        if k < 3 or not bio_u64_shape(f[1]) or not bio_u64_max(f[2], 1023):
            return 0
        if k == 4:
            return f[3] == 'eligible'
        if k == 6:
            lived = bio_canonical_u64(f[4])
            return (f[3] == 'chart' and lived is not None and
                    lived < 1000 and bio_u64_shape(f[5]))
        if k == 7:
            lived = bio_canonical_u64(f[6])
            return (f[3] == 'earned' and bio_fix6_max8(f[4]) and
                    bio_fix6_max8(f[5]) and lived is not None and
                    lived >= 1000)
        return 0
    if t == 'r':
        if k != 7 and k != 8:
            return 0
        if k == 8 and f[7] != '8.000000':
            return 0
        return (bio_u64_shape(f[1]) and bio_u64_max(f[2], 1023) and
                f[3] != 'uni' and bio_seated_word(f[3]) and
                (bio_byte_actor_word(f[4]) if k == 7
                 else bio_challenger_word(f[4])) and
                f[3] != f[4] and
                bio_fix6_shape(f[5]) and bio_fix6_shape(f[6]))
    if t == 'q':
        return (k == 6 and bio_u64_shape(f[1]) and
                bio_u64_max(f[2], 1023) and bio_seated_word(f[3]) and
                bio_challenger_word(f[4]) and f[3] != f[4] and
                f[5] == '0')
    if t == 'w':
        if k != 9:
            return 0
        bts = bio_canonical_u64(f[3])
        shores = bio_canonical_u64(f[5])
        verdicts = bio_canonical_u64(f[8])
        return (bio_u64_shape(f[1]) and bio_lower_hex(f[2], 16) and
                bts is not None and 2 <= bts <= 16384 and
                (f[4] == 'cold' or bio_lower_hex(f[4], 4)) and
                shores is not None and 1 <= shores <= 32 and
                bio_lower_hex(f[6], 16) and bio_lower_hex(f[7], 16) and
                verdicts is not None and verdicts == shores)
    return 0

def bio_record_type_ok(line):
    n = len(line)
    if n >= 2 and '0' <= line[0] <= '9':
        i = 0
        while i < n and '0' <= line[i] <= '9':
            i += 1
        if line[0] == '0' and i != 1:
            return 0
    elif n >= 2 and line[0] in 'abdimqrstuvw':
        i = 1
    else:
        return 0
    return 1 if i < n and line[i] == '\t' else 0

def bio_verify(path):
    try:
        named = os.stat(path)
    except OSError as e:
        if e.errno == errno_mod.ENOENT:
            die("netta: biography %s is missing; refusing resume\n" % path)
        die("netta: cannot inspect biography %s\n" % path)
    if not stat_mod.S_ISREG(named.st_mode):
        die("netta: biography %s is not a regular file; refusing\n" % path)
    try:
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    except OSError:
        die("netta: biography %s is missing; refusing resume\n" % path)
    opened = os.fstat(fd)
    if (not stat_mod.S_ISREG(opened.st_mode) or
            opened.st_dev != named.st_dev or opened.st_ino != named.st_ino):
        os.close(fd)
        die("netta: biography %s changed while being opened; refusing\n"
            % path)
    f = os.fdopen(fd, 'rb')
    data = f.read()
    f.close()
    chain = FNV_SEED
    lines = 0
    arrivals = 0
    malformed = 0
    unknown = 0
    pos = 0
    total = len(data)
    while pos < total:
        nl = data.find(b'\n', pos)
        if nl < 0:
            raw = data[pos:]
            pos = total
        else:
            raw = data[pos:nl + 1]
            pos = nl + 1
        chain = fnv1a64(raw, chain)
        lines += 1
        line = raw.decode('latin-1')
        n = len(line)
        if n == 0 or line[-1] != '\n':
            malformed = 1
        elif not bio_record_type_ok(line):
            unknown = 1
        if n >= 2 and line[0] == 'i' and line[1] == '\t':
            if not bio_arrival_ok(line, arrivals):
                malformed = 1
            arrivals += 1
        if (n >= 2 and line[0] == 's' and line[1] == '\t' and
                not bio_signature_ok(line)):
            malformed = 1
        if (n >= 2 and line[-1] == '\n' and line[0] != 'i' and
                line[0] != 's' and bio_record_type_ok(line) and
                not bio_grammar_ok(line)):
            malformed = 1
    if unknown:
        die("netta: biography %s carries a record of no known type; "
            "refusing\n" % path)
    if malformed or arrivals != reg_count:
        die("netta: biography %s does not conserve the island registry; "
            "refusing\n" % path)
    if chain != bio_chain or lines != bio_lines:
        die("netta: biography %s does not match state "
            "(lines %d/%d, chain %016x/%016x); refusing\n"
            % (path, lines, bio_lines, chain, bio_chain))

def bio_open(path, reset):
    global bio_file
    flags = os.O_WRONLY | os.O_CREAT | os.O_NONBLOCK | \
        (os.O_TRUNC if reset else os.O_APPEND)
    try:
        fd = os.open(path, flags, 0o666)
    except OSError:
        die("netta: cannot open biography %s\n" % path)
    try:
        opened = os.fstat(fd)
        named = os.stat(path)
    except OSError:
        os.close(fd)
        die("netta: biography %s must be a regular file\n" % path)
    if (not stat_mod.S_ISREG(opened.st_mode) or
            not stat_mod.S_ISREG(named.st_mode) or
            opened.st_dev != named.st_dev or opened.st_ino != named.st_ino):
        os.close(fd)
        die("netta: biography %s must be a regular file\n" % path)
    bio_file = os.fdopen(fd, 'wb' if reset else 'ab')

def bio_append(line):
    global bio_chain, bio_lines
    data = line.encode('latin-1')
    try:
        bio_file.write(data)
    except OSError:
        die("netta: biography write failed\n")
    bio_chain = fnv1a64(data, bio_chain)
    bio_lines += 1

# --------------------------------------------------------------- units

class Unit:
    __slots__ = ('bytes', 'len', 'born_episode', 'support_at_birth',
                 'uses', 'last_use', 'dead')

units = []
unit_living = 0
births_rejected_cap = 0
unit_deaths = 0

pair_key = None
pair_cnt = None
pair_used = 0

units_enabled = 1

def is_ws(b):
    return b in (0x20, 0x09, 0x0a, 0x0d)

def move_len(m):
    return 1 if m < ACTIONS else units[m - ACTIONS].len

def move_bytes(m):
    if m < ACTIONS:
        return bytes((m,))
    return units[m - ACTIONS].bytes

def unit_find(b, ln):
    for u in range(len(units)):
        if units[u].len == ln and units[u].bytes[:ln] == b[:ln]:
            return u
    return -1

def unit_find_living(b, ln):
    u = unit_find(b, ln)
    return u if (u >= 0 and not units[u].dead) else -1

def unit_prefix_alive(b, ln):
    pref = bytes(b[:ln])
    for u in units:
        if not u.dead and u.len >= ln and u.bytes[:ln] == pref:
            return 1
    return 0

steps_total = 0

def unit_birth(a, b, support):
    global unit_living, births_rejected_cap
    la = move_len(a)
    lb = move_len(b)
    if la + lb > UNIT_MAX_LEN:
        return
    buf = move_bytes(a)[:la] + move_bytes(b)[:lb]
    L = la + lb
    for i in range(L):
        if is_ws(buf[i]):
            return
    prior = unit_find(buf, L)
    if prior >= 0:
        if not units[prior].dead:
            return
        live_mass_add_unit(prior)
        units[prior].dead = 0
        units[prior].last_use = steps_total
        unit_living += 1
        bio_append("u\t%d\t%d\t%d\n" % (episode_no, prior, support))
        return
    if len(units) >= MAX_UNITS:
        births_rejected_cap += 1
        return
    u = Unit()
    u.bytes = buf + b'\x00' * (UNIT_MAX_LEN - L)
    u.len = L
    u.born_episode = episode_no
    u.support_at_birth = support
    u.uses = 0
    u.last_use = steps_total
    u.dead = 0
    hexs = ''.join('%02x' % c for c in buf[:L])
    bio_append("b\t%d\t%d\t%d\t%s\t%d\n" %
               (episode_no, len(units), L, hexs, support))
    units.append(u)
    unit_living += 1

_KEYPACK = struct.Struct('<Q')

def _key_hash(key):
    return fnv1a64(_KEYPACK.pack(key), FNV_SEED)

def pair_get(prev, cur):
    key = ((prev << 32) | cur) & MASK64
    h = _key_hash(key)
    for probe in range(PAIR_SLOTS):
        idx = (h + probe) & (PAIR_SLOTS - 1)
        if pair_cnt[idx] == 0:
            return 0
        if pair_key[idx] == key:
            return pair_cnt[idx]
    return 0

def pair_feed(prev, cur):
    global pair_used
    key = ((prev << 32) | cur) & MASK64
    h = _key_hash(key)
    for probe in range(PAIR_SLOTS):
        idx = (h + probe) & (PAIR_SLOTS - 1)
        if pair_cnt[idx] == 0:
            if pair_used * 10 >= PAIR_SLOTS * 9:
                die("netta: pair table full\n")
            pair_key[idx] = key
            pair_cnt[idx] = 1
            pair_used += 1
            return
        if pair_key[idx] == key:
            pair_cnt[idx] += 1
            if pair_cnt[idx] % BIRTH_SUPPORT == 0:
                unit_birth(prev, cur, pair_cnt[idx])
            return
    die("netta: pair table full\n")

# matcher
mbuf = bytearray(UNIT_MAX_LEN)
mbuf_atomic_bits = [0.0] * UNIT_MAX_LEN
mlen = 0
prev_move = -1
mstart_pos = 0
mstart_isl = 0
macro_events = 0
macro_bytes = 0
moves_emitted = 0

move_count = [0] * (ACTIONS + MAX_UNITS)
move_total = 0
move_live_total = 0
mv_live_out = [0] * (ACTIONS + MAX_UNITS)
tombstone_silence_enabled = 1
unitlm_bits = 0.0
unitlm_bytes = 0
atomic_bits_lived = 0.0

bi_count = [[0] * ACTIONS for _ in range(ACTIONS)]
bi_row = [0] * ACTIONS
bilm_bits = 0.0
bilm_bytes = 0
mv_out = [0] * (ACTIONS + MAX_UNITS)
mvlm_bits = 0.0
mvlm_bytes = 0
mvlm_ref_bits = 0.0
mvlm_ref_bytes = 0
mvp_bits = 0.0
mvp_bytes = 0
mvp_ref_bits = [0.0, 0.0, 0.0]
mvp_ref_bytes = 0
mvc_bits = 0.0
mvc_bytes = 0
mvc_ref_bits = [0.0, 0.0, 0.0]

TRI_SLOTS = 1 << 20
tri_key = None
tri_cnt = None
tri_used = 0
tri_row = [0] * 65536
trilm_bits = 0.0
trilm_bytes = 0

def tri_get(ctx, b):
    key = ((ctx << 8) | b) & MASK64
    h = _key_hash(key)
    for probe in range(TRI_SLOTS):
        idx = (h + probe) & (TRI_SLOTS - 1)
        if tri_cnt[idx] == 0:
            return 0
        if tri_key[idx] == key:
            return tri_cnt[idx]
    return 0

def tri_add(ctx, b):
    global tri_used
    key = ((ctx << 8) | b) & MASK64
    h = _key_hash(key)
    for probe in range(TRI_SLOTS):
        idx = (h + probe) & (TRI_SLOTS - 1)
        if tri_cnt[idx] == 0:
            if tri_used * 10 >= TRI_SLOTS * 9:
                die("netta: trigram table full\n")
            tri_key[idx] = key
            tri_cnt[idx] = 1
            tri_used += 1
            tri_row[ctx] += 1
            return
        if tri_key[idx] == key:
            tri_cnt[idx] += 1
            tri_row[ctx] += 1
            return
    die("netta: trigram table full\n")

# ------------------------------------------------------ the neural core

CORE_EMBED = 24
CORE_HIDDEN = 32
CORE_INIT_SEED = 0x4e455454414e4e31
CORE_LR_OUT = f32(0.05)
CORE_LR_IN = f32(0.01)
CORE_DECAY = f32(0.9995)
CORE_CLIP = 1.0
CORE_GATE = 0.5
CORE_WCLAMP = f32(4.0)

core_E = [[0.0] * CORE_EMBED for _ in range(ACTIONS)]
core_Wxh = [[0.0] * CORE_EMBED for _ in range(CORE_HIDDEN)]
core_Whh = [[0.0] * CORE_HIDDEN for _ in range(CORE_HIDDEN)]
core_Who = [[0.0] * CORE_HIDDEN for _ in range(ACTIONS)]
core_h = [0.0] * CORE_HIDDEN
core_bits = 0.0
core_bytes = 0
core_nll_ema = 8.0
core_sat_sum = 0.0
core_sat_n = 0
core_enabled = 1
core_hebb_enabled = 0
core_hebb_pos = 0
core_hebb_neg = 0

JURY_GENOMES = 8

def _g(a, b, c, d, e, ff, g_):
    return tuple(f32(x) for x in (a, b, c, d, e, ff, g_))

jury_gene = [
    _g(0.0, 0.0, 0.5, 1.00, 0.0, 1.00, 0.0),
    _g(0.0003125, 0.0003125, 0.5, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0006250, 0.0006250, 0.5, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0012500, 0.0012500, 0.5, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0006250, 0.0003125, 0.5, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0003125, 0.0006250, 0.5, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0006250, 0.0006250, 1.0, 0.25, 0.000001, 0.35, 0.000244140625),
    _g(0.0006250, 0.0006250, 0.5, 0.50, 0.000001, 0.25, 0.000488281250),
]
# gene fields: eta_in, eta_rec, gate, mod_clip, decay, target, homeostasis

class JuryCore:
    __slots__ = ('Wxh', 'Whh', 'Who', 'h', 'nll_ema', 'bits', 'bytes',
                 'row_abs_sum', 'health_bytes', 'near_sat', 'clamp_hits',
                 'gates_pos', 'gates_neg')

jury_core = []
jury_enabled = 0

def core_clampw(w):
    if w > CORE_WCLAMP:
        return CORE_WCLAMP
    if w < -CORE_WCLAMP:
        return -CORE_WCLAMP
    return w

def core_rand(s):
    s[0] = (s[0] + 0x9e3779b97f4a7c15) & MASK64
    z = s[0]
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & MASK64
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & MASK64
    z ^= z >> 31
    return f32(f32(f32(z >> 40) / 8388608.0) - 1.0)

def core_init():
    s = [CORE_INIT_SEED]
    for b in range(ACTIONS):
        row = core_E[b]
        for d in range(CORE_EMBED):
            row[d] = f32(0.5 * core_rand(s))
    for j in range(CORE_HIDDEN):
        row = core_Wxh[j]
        for d in range(CORE_EMBED):
            row[d] = f32(f32(0.3) * core_rand(s))
    for j in range(CORE_HIDDEN):
        row = core_Whh[j]
        for k in range(CORE_HIDDEN):
            row[k] = f32(f32(0.2) * core_rand(s))
    for o in range(ACTIONS):
        row = core_Who[o]
        for j in range(CORE_HIDDEN):
            row[j] = 0.0

def jury_init():
    global jury_core
    jury_core = []
    for g in range(JURY_GENOMES):
        c = JuryCore()
        c.Wxh = [row[:] for row in core_Wxh]
        c.Whh = [row[:] for row in core_Whh]
        c.Who = [row[:] for row in core_Who]
        c.h = [0.0] * CORE_HIDDEN
        c.nll_ema = 8.0
        c.bits = 0.0
        c.bytes = 0
        c.row_abs_sum = [0.0] * CORE_HIDDEN
        c.health_bytes = 0
        c.near_sat = 0
        c.clamp_hits = 0
        c.gates_pos = 0
        c.gates_neg = 0
        jury_core.append(c)

def core_advance(byte):
    global core_h
    E = core_E[byte]
    hn = [0.0] * CORE_HIDDEN
    h = core_h
    for j in range(CORE_HIDDEN):
        a = 0.0
        Wx = core_Wxh[j]
        for d in range(CORE_EMBED):
            a = fmaf(Wx[d], E[d], a)
        Wh = core_Whh[j]
        for k in range(CORE_HIDDEN):
            a = f32(a + f32(Wh[k] * h[k]))
        hn[j] = tanhf(a)
    core_h = hn

def core_warm(isl, start):
    global core_h
    core_h = [0.0] * CORE_HIDDEN
    for p in range(start - CTX, start):
        core_advance(isl.bytes[p])

def core_absorb(truth):
    global core_bits, core_bytes, core_sat_sum, core_sat_n
    global core_nll_ema, core_hebb_pos, core_hebb_neg
    logits = [0.0] * ACTIONS
    mx = f32(-1e30)
    h = core_h
    for o in range(ACTIONS):
        a = 0.0
        Wo = core_Who[o]
        for j in range(CORE_HIDDEN):
            a = f32(a + f32(Wo[j] * h[j]))
        logits[o] = a
        if a > mx:
            mx = a
    denom = 0.0
    for o in range(ACTIONS):
        denom += math.exp(logits[o] - mx)
    p_truth = math.exp(logits[truth] - mx) / denom
    nll = -math.log2(p_truth)
    if not math.isfinite(nll):
        die("netta: core price is not finite\n")
    core_bits += nll
    core_bytes += 1

    h_old = h[:]
    for o in range(ACTIONS):
        p_o = f32(math.exp(logits[o] - mx) / denom)
        err = f32((1.0 if o == truth else 0.0) - p_o)
        Wo = core_Who[o]
        t = f32(CORE_LR_OUT * err)
        for j in range(CORE_HIDDEN):
            Wo[j] = core_clampw(fmaf(t, h_old[j], Wo[j]))

    core_advance(truth)
    sat = 0.0
    for j in range(CORE_HIDDEN):
        sat += abs(core_h[j])
    core_sat_sum += sat / CORE_HIDDEN
    core_sat_n += 1

    surprise = core_nll_ema - nll
    if abs(surprise) > CORE_GATE:
        if surprise > 0.0:
            core_hebb_pos += 1
        else:
            core_hebb_neg += 1
        mod = f32(surprise / 8.0)
        if mod > CORE_CLIP:
            mod = f32(CORE_CLIP)
        if mod < -CORE_CLIP:
            mod = f32(-CORE_CLIP)
        if core_hebb_enabled:
            for j in range(CORE_HIDDEN):
                Wx = core_Wxh[j]
                E = core_E[truth]
                hj = core_h[j]
                for d in range(CORE_EMBED):
                    Wx[d] = core_clampw(fmaf(
                        CORE_DECAY, Wx[d],
                        f32(f32(f32(CORE_LR_IN * mod) * hj) * E[d])))
                Wh = core_Whh[j]
                for k in range(CORE_HIDDEN):
                    Wh[k] = core_clampw(fmaf(
                        CORE_DECAY, Wh[k],
                        f32(f32(f32(CORE_LR_IN * mod) * hj) *
                            h_old[k])))

    core_nll_ema = math.fma(0.82, core_nll_ema, 0.18 * nll)

_FPACK = struct.Struct('<f')
_DPACK = struct.Struct('<d')
_QPACK = struct.Struct('<Q')

def _floats_bytes(rows):
    return b''.join(_FPACK.pack(v) for row in rows for v in row)

def core_state_witness():
    h = FNV_WITNESS_SEED
    h = fnv1a64(_floats_bytes(core_Wxh), h)
    h = fnv1a64(_floats_bytes(core_Whh), h)
    h = fnv1a64(_floats_bytes(core_Who), h)
    h = fnv1a64(_DPACK.pack(core_nll_ema), h)
    h = fnv1a64(_DPACK.pack(core_bits), h)
    h = fnv1a64(_QPACK.pack(core_bytes), h)
    return h

def jury_clampw(value, c):
    if not math.isfinite(value):
        die("netta: jury weight is not finite\n")
    if value > CORE_WCLAMP:
        if c is not None:
            c.clamp_hits += 1
        return CORE_WCLAMP
    if value < -CORE_WCLAMP:
        if c is not None:
            c.clamp_hits += 1
        return -CORE_WCLAMP
    return value

def jury_advance_one(c, byte):
    E = core_E[byte]
    hn = [0.0] * CORE_HIDDEN
    h = c.h
    for j in range(CORE_HIDDEN):
        a = 0.0
        Wx = c.Wxh[j]
        for d in range(CORE_EMBED):
            a = fmaf(Wx[d], E[d], a)
        Wh = c.Whh[j]
        for k in range(CORE_HIDDEN):
            a = f32(a + f32(Wh[k] * h[k]))
        hn[j] = tanhf(a)
    c.h = hn

def jury_warm(isl, start):
    for c in jury_core:
        c.h = [0.0] * CORE_HIDDEN
        for p in range(start - CTX, start):
            jury_advance_one(c, isl.bytes[p])

def jury_absorb(truth):
    for g in range(JURY_GENOMES):
        c = jury_core[g]
        eta_in, eta_rec, gate, mod_clip, decay, target, homeostasis = \
            jury_gene[g]
        logits = [0.0] * ACTIONS
        mx = f32(-1e30)
        h = c.h
        for o in range(ACTIONS):
            a = 0.0
            Wo = c.Who[o]
            for j in range(CORE_HIDDEN):
                a = f32(a + f32(Wo[j] * h[j]))
            logits[o] = a
            if a > mx:
                mx = a
        denom = 0.0
        for o in range(ACTIONS):
            denom += math.exp(logits[o] - mx)
        p_truth = math.exp(logits[truth] - mx) / denom
        nll = -math.log2(p_truth)
        if not math.isfinite(nll):
            die("netta: jury price is not finite\n")
        c.bits += nll
        c.bytes += 1

        h_old = h[:]
        for o in range(ACTIONS):
            p_o = f32(math.exp(logits[o] - mx) / denom)
            err = f32((1.0 if o == truth else 0.0) - p_o)
            Wo = c.Who[o]
            t = f32(CORE_LR_OUT * err)
            for j in range(CORE_HIDDEN):
                Wo[j] = jury_clampw(fmaf(t, h_old[j], Wo[j]), None)

        jury_advance_one(c, truth)
        c.health_bytes += 1
        for j in range(CORE_HIDDEN):
            ah = abs(c.h[j])
            c.row_abs_sum[j] += ah
            if ah >= 0.98:
                c.near_sat += 1

        surprise = c.nll_ema - nll
        gate_open = abs(surprise) > gate
        mod = f32(surprise / 8.0)
        if mod > mod_clip:
            mod = mod_clip
        if mod < -mod_clip:
            mod = -mod_clip
        if gate_open:
            if surprise > 0.0:
                c.gates_pos += 1
            else:
                c.gates_neg += 1
        E = core_E[truth]
        for j in range(CORE_HIDDEN):
            excess = f32(abs(c.h[j]) - target)
            if excess < 0.0:
                excess = 0.0
            scale = fmaf(-homeostasis, excess, f32(1.0 - decay))
            if scale < 0.0:
                scale = 0.0
            hj = c.h[j]
            Wx = c.Wxh[j]
            for d in range(CORE_EMBED):
                add = (f32(f32(f32(eta_in * mod) * hj) * E[d])
                       if gate_open else 0.0)
                Wx[d] = jury_clampw(fmaf(scale, Wx[d], add), c)
            Wh = c.Whh[j]
            for k in range(CORE_HIDDEN):
                add = (f32(f32(f32(eta_rec * mod) * hj) * h_old[k])
                       if gate_open else 0.0)
                Wh[k] = jury_clampw(fmaf(scale, Wh[k], add), c)
        c.nll_ema = math.fma(0.82, c.nll_ema, 0.18 * nll)

def _gene_bytes():
    return b''.join(_FPACK.pack(v) for g in jury_gene for v in g)

def jury_state_witness():
    h = FNV_WITNESS_SEED
    h = fnv1a64(_gene_bytes(), h)
    for c in jury_core:
        h = fnv1a64(_floats_bytes(c.Wxh), h)
        h = fnv1a64(_floats_bytes(c.Whh), h)
        h = fnv1a64(_floats_bytes(c.Who), h)
        h = fnv1a64(_DPACK.pack(c.nll_ema), h)
        h = fnv1a64(_DPACK.pack(c.bits), h)
        h = fnv1a64(_QPACK.pack(c.bytes), h)
        h = fnv1a64(b''.join(_DPACK.pack(v) for v in c.row_abs_sum), h)
        h = fnv1a64(_QPACK.pack(c.health_bytes), h)
        h = fnv1a64(_QPACK.pack(c.near_sat), h)
        h = fnv1a64(_QPACK.pack(c.clamp_hits), h)
        h = fnv1a64(_QPACK.pack(c.gates_pos), h)
        h = fnv1a64(_QPACK.pack(c.gates_neg), h)
    return h

def neural_law():
    return ((1 if core_enabled else 0) |
            (2 if core_hebb_enabled else 0) |
            (4 if jury_enabled else 0))


# ----------------------------------------------------------- the seat

actor_current = 0
actor_lock = -1
move_nav_enabled = 1
ACTOR_NULL = 4
ep_actor = [0, 0, 0, 0, 0]
actor_name = ["uni", "bi", "tri", "mv", "null"]
move_nav_steps = 0
move_nav_unit_anchors = 0

isl_bits = [[0.0, 0.0, 0.0] for _ in range(MAX_REGISTRY)]
isl_lived = [0] * MAX_REGISTRY
isl_mvp_bits = [0.0] * MAX_REGISTRY
isl_mvp_bytes = [0] * MAX_REGISTRY
isl_mvp_ref_bits = [[0.0, 0.0, 0.0] for _ in range(MAX_REGISTRY)]
isl_mvp_ref_bytes = [0] * MAX_REGISTRY
isl_mvlm_bits = [0.0] * MAX_REGISTRY
isl_mvlm_ref_bits = [0.0] * MAX_REGISTRY
isl_mvlm_bytes = [0] * MAX_REGISTRY

present_reg = [0] * MAX_ISLANDS
arrivals_pending = []

def registry_find(digest, witness, ln):
    for r in range(reg_count):
        if (reg_digest[r] == digest and reg_witness[r] == witness and
                reg_len[r] == ln):
            return r
    return -1

def same_island_identity(a, b):
    return (a.digest == b.digest and a.witness == b.witness and
            a.len == b.len)

def registry_add(isl):
    global reg_count
    if reg_count >= MAX_REGISTRY:
        die("netta: island registry full (max %d)\n" % MAX_REGISTRY)
    reg_digest[reg_count] = isl.digest
    reg_witness[reg_count] = isl.witness
    reg_len[reg_count] = isl.len
    arrivals_pending.append(reg_count)
    reg_count += 1
    return reg_count - 1

def island_identity_less(a, b):
    if a.digest != b.digest:
        return a.digest < b.digest
    if a.witness != b.witness:
        return a.witness < b.witness
    return a.len < b.len

def registry_resolve_convoy():
    for i in range(len(islands)):
        present_reg[i] = registry_find(islands[i].digest,
                                       islands[i].witness, islands[i].len)
        for j in range(i):
            if (same_island_identity(islands[i], islands[j]) and
                    (islands[i].len != islands[j].len or
                     islands[i].bytes != islands[j].bytes)):
                die("netta: island identity collision; refusing\n")
    while True:
        best = -1
        for i in range(len(islands)):
            if present_reg[i] < 0 and (
                    best < 0 or
                    island_identity_less(islands[i], islands[best])):
                best = i
        if best < 0:
            break
        r = registry_add(islands[best])
        for i in range(len(islands)):
            if present_reg[i] < 0 and \
                    same_island_identity(islands[i], islands[best]):
                present_reg[i] = r

island_court_enabled = 1
local_probation_enabled = 1
birth_floor_enabled = 1
revocations = 0
null_refusals = 0
unit_death_enabled = 1

ACTOR_MIN_BYTES = 1000
ACTOR_GAIN = 0.1
ACTOR_KEEP = 0.05

atlas_enabled = 0
atlas_decisions = 0

def island_can_hold(i, steps):
    isl = islands[i]
    if isl.len < CTX + 2 or steps > isl.len - CTX - 1:
        return 0
    if fixed_start_set and (
            fixed_start < CTX or fixed_start >= isl.len or
            steps > isl.len - fixed_start):
        return 0
    return 1

def atlas_score(reg):
    best = 8.0
    for c in range(3):
        score = isl_bits[reg][c] / isl_lived[reg]
        if score < best:
            best = score
    return best

def atlas_choose(fallback, steps):
    global atlas_decisions
    candidate = []
    unique_present = 0
    for i in range(len(islands)):
        seen = 0
        for j in range(i):
            if present_reg[j] == present_reg[i]:
                seen = 1
                break
        if seen:
            continue
        unique_present += 1
        if island_can_hold(i, steps):
            candidate.append(i)
    n = len(candidate)
    if unique_present == 1:
        return fallback
    if n == 0:
        return fallback
    if n == 1:
        r = present_reg[candidate[0]]
        bio_append("t\t%d\t%d\teligible\n" % (episode_no + 1, r))
        atlas_decisions += 1
        sys.stdout.write("atlas: episode %d registry %d sole eligible "
                         "shore\n" % (episode_no + 1, r))
        return candidate[0]

    best = -1
    chart = 0
    for i in candidate:
        r = present_reg[i]
        if isl_lived[r] >= ACTOR_MIN_BYTES:
            continue
        if (not chart or isl_lived[r] < isl_lived[present_reg[best]] or
                (isl_lived[r] == isl_lived[present_reg[best]] and
                 r < present_reg[best])):
            best = i
        chart = 1
    if not chart:
        for i in candidate:
            r = present_reg[i]
            if (best < 0 or
                    atlas_score(r) < atlas_score(present_reg[best]) or
                    (atlas_score(r) == atlas_score(present_reg[best]) and
                     r < present_reg[best])):
                best = i

    r = present_reg[best]
    if chart:
        runner = MASK64
        for i in candidate:
            if i != best and isl_lived[present_reg[i]] < runner:
                runner = isl_lived[present_reg[i]]
        bio_append("t\t%d\t%d\tchart\t%d\t%d\n" %
                   (episode_no + 1, r, isl_lived[r], runner))
    else:
        runner = 1e9
        for i in candidate:
            if i != best and atlas_score(present_reg[i]) < runner:
                runner = atlas_score(present_reg[i])
        bio_append("t\t%d\t%d\tearned\t%.6f\t%.6f\t%d\n" %
                   (episode_no + 1, r, atlas_score(r), runner,
                    isl_lived[r]))
    atlas_decisions += 1
    sys.stdout.write("atlas: episode %d registry %d %s " %
                     (episode_no + 1, r, "chart" if chart else "earned"))
    if chart:
        sys.stdout.write("lived %d\n" % isl_lived[r])
    else:
        sys.stdout.write("score %.6f over %d bytes\n" %
                         (atlas_score(r), isl_lived[r]))
    return best

def actor_elect():
    global actor_current
    if actor_lock >= 0:
        actor_current = actor_lock
        return
    if atomic_bytes_lived < ACTOR_MIN_BYTES:
        actor_current = 0
        return
    lv = [atomic_bits_lived / atomic_bytes_lived,
          (bilm_bits / bilm_bytes) if bilm_bytes else 1e9,
          (trilm_bits / trilm_bytes) if trilm_bytes else 1e9]

    byte_seat = actor_current if 0 <= actor_current <= 2 else 0
    if byte_seat != 0 and lv[0] - lv[byte_seat] < ACTOR_KEEP:
        byte_seat = 0
    ch = -1
    for c in (1, 2):
        if c == byte_seat:
            continue
        if lv[0] - lv[c] < ACTOR_GAIN:
            continue
        if ch < 0 or lv[c] < lv[ch]:
            ch = c
    if ch >= 0:
        if byte_seat == 0 or lv[byte_seat] - lv[ch] >= ACTOR_KEEP:
            byte_seat = ch

    if (units_enabled and mvp_bytes >= ACTOR_MIN_BYTES and
            mvp_ref_bytes == mvp_bytes):
        mv = mvp_bits / mvp_bytes
        best_ref = mvp_ref_bits[0] / mvp_ref_bytes
        for c in (1, 2):
            ref = mvp_ref_bits[c] / mvp_ref_bytes
            if ref < best_ref:
                best_ref = ref
        if actor_current == 3:
            actor_current = 3 if best_ref - mv >= ACTOR_KEEP else byte_seat
        else:
            actor_current = 3 if best_ref - mv >= ACTOR_GAIN else byte_seat
    else:
        actor_current = byte_seat

def island_court(isl, episode_steps):
    global revocations, null_refusals
    seated = actor_current
    if not island_court_enabled or actor_lock >= 0:
        return seated

    if not birth_floor_enabled:
        if seated == 0 or isl_lived[isl] < ACTOR_MIN_BYTES:
            return seated
        lu = isl_bits[isl][0] / isl_lived[isl]
        if seated == 3:
            if (isl_mvp_bytes[isl] < ACTOR_MIN_BYTES or
                    isl_mvp_ref_bytes[isl] != isl_mvp_bytes[isl]):
                return seated
            lseat = isl_mvp_bits[isl] / isl_mvp_bytes[isl]
            br = isl_mvp_ref_bits[isl][0] / isl_mvp_ref_bytes[isl]
            for c in (1, 2):
                rr = isl_mvp_ref_bits[isl][c] / isl_mvp_ref_bytes[isl]
                if rr < br:
                    br = rr
            fails = br - lseat < ACTOR_KEEP
        else:
            lseat = isl_bits[isl][seated] / isl_lived[isl]
            fails = lu - lseat < ACTOR_KEEP
        if not fails:
            return seated
        ch = 0
        for c in (1, 2):
            if c == seated:
                continue
            lc = isl_bits[isl][c] / isl_lived[isl]
            if lu - lc < ACTOR_GAIN:
                continue
            if ch == 0 or lc < isl_bits[isl][ch] / isl_lived[isl]:
                ch = c
        revocations += 1
        bio_append("r\t%d\t%d\t%s\t%s\t%.6f\t%.6f\n" %
                   (episode_no, isl, actor_name[seated], actor_name[ch],
                    lu, lseat))
        return ch

    if steps_total == isl_lived[isl]:
        return seated

    lived = isl_lived[isl]
    if lived < ACTOR_MIN_BYTES and episode_steps <= ACTOR_MIN_BYTES - lived:
        return seated
    if lived == 0:
        revocations += 1
        null_refusals += 1
        bio_append("q\t%d\t%d\t%s\tnull\t0\n" %
                   (episode_no, isl, actor_name[seated]))
        return ACTOR_NULL

    lu = isl_bits[isl][0] / isl_lived[isl]
    seat_unpriced = 0
    if seated == 3:
        played = isl_mvp_bytes[isl]
        if played < ACTOR_MIN_BYTES and \
                episode_steps <= ACTOR_MIN_BYTES - played:
            return seated
        if played == 0:
            fails = 1
            seat_unpriced = 1
            lseat = 0.0
        else:
            lseat = isl_mvp_bits[isl] / played
            br = isl_mvp_ref_bits[isl][0] / isl_mvp_ref_bytes[isl]
            for c in (1, 2):
                rr = isl_mvp_ref_bits[isl][c] / isl_mvp_ref_bytes[isl]
                if rr < br:
                    br = rr
            fails = br - lseat < ACTOR_KEEP
    elif seated == 0:
        lseat = lu
        fails = 0
    else:
        lseat = isl_bits[isl][seated] / isl_lived[isl]
        fails = lu - lseat < ACTOR_KEEP

    ch = seated
    if fails:
        ch = 0
        for c in (1, 2):
            if c == seated:
                continue
            lc = isl_bits[isl][c] / isl_lived[isl]
            if lu - lc < ACTOR_GAIN:
                continue
            if ch == 0 or lc < isl_bits[isl][ch] / isl_lived[isl]:
                ch = c

    lhand = lseat if ch == 3 else isl_bits[isl][ch] / isl_lived[isl]
    if 8.0 - lhand < ACTOR_GAIN:
        ch = ACTOR_NULL
    if ch == seated:
        return seated

    revocations += 1
    if ch == ACTOR_NULL:
        null_refusals += 1
    if seat_unpriced:
        bio_append("q\t%d\t%d\t%s\t%s\t0\n" %
                   (episode_no, isl, actor_name[seated], actor_name[ch]))
    else:
        bio_append("r\t%d\t%d\t%s\t%s\t%.6f\t%.6f\t8.000000\n" %
                   (episode_no, isl, actor_name[seated], actor_name[ch],
                    lu, lseat))
    return ch

def truth_move(isl, pos, room):
    best_len = 1
    best_u = -1
    for u in range(len(units)):
        ln = units[u].len
        if (units[u].dead or ln <= best_len or ln > room or
                pos > isl.len - ln):
            continue
        if isl.bytes[pos:pos + ln] == units[u].bytes[:ln]:
            best_len = ln
            best_u = u
    return (ACTIONS + best_u) if best_u >= 0 else isl.bytes[pos]

def move_prob(prev, cur, alive):
    if prev >= 0:
        row = mv_live_out[prev] if tombstone_silence_enabled else \
            mv_out[prev]
        denom = float(row) + float(alive)
        cnt = pair_get(prev, cur)
    else:
        total = move_live_total if tombstone_silence_enabled else move_total
        denom = float(total) + float(alive)
        cnt = move_count[cur]
    return (cnt + 1.0) / denom

def move_logp(prev, cur, alive):
    return math.log(move_prob(prev, cur, alive))

def move_matches(isl, pos, room, move):
    ln = move_len(move)
    if ln > room or ln > isl.len or pos > isl.len - ln:
        return 0
    return isl.bytes[pos:pos + ln] == move_bytes(move)[:ln]

def route_offer(routes_row, move, score):
    ln = move_len(move)
    r = routes_row[ln]
    if (not r[2]) or score > r[1] or (score == r[1] and move < r[0]):
        r[0] = move
        r[1] = score
        r[2] = 1

def move_route_anchor(isl, pos, rng_, alphabet):
    routes = [[[0, 0.0, 0] for _ in range(UNIT_MAX_LEN + 1)]
              for _ in range(CTX + 1)]
    base = pos - CTX

    for m in range(rng_):
        if m >= ACTIONS and units[m - ACTIONS].dead:
            continue
        ln = move_len(m)
        if ln <= CTX and move_matches(isl, base, CTX, m):
            route_offer(routes[ln], m, move_logp(-1, m, alphabet))
    for off in range(1, CTX):
        for pl in range(1, UNIT_MAX_LEN + 1):
            prior = routes[off][pl]
            if not prior[2]:
                continue
            room = CTX - off
            for m in range(rng_):
                if m >= ACTIONS and units[m - ACTIONS].dead:
                    continue
                ln = move_len(m)
                if ln <= room and move_matches(isl, base + off, room, m):
                    route_offer(routes[off + ln], m,
                                prior[1] + move_logp(prior[0], m, alphabet))
    best = None
    for ln in range(1, UNIT_MAX_LEN + 1):
        r = routes[CTX][ln]
        if r[2] and (best is None or r[1] > best[1] or
                     (r[1] == best[1] and r[0] < best[0])):
            best = r
    if best is None:
        die("netta: no exact route through observable wake\n")
    return best[0]

def build_move_dist(prev, rng_, alphabet, search, move_p):
    tombs = rng_ - alphabet
    ssum = 0.0
    for c in range(rng_):
        if c >= ACTIONS and units[c - ACTIONS].dead:
            move_p[c] = 0.0
            continue
        move_p[c] = move_prob(prev, c, alphabet)
        if search:
            best_future = 0.0
            for d in range(rng_):
                if d >= ACTIONS and units[d - ACTIONS].dead:
                    continue
                p = move_prob(c, d, alphabet)
                if p > best_future:
                    best_future = p
            move_p[c] *= best_future
        ssum += move_p[c]
    if search or tombs:
        for c in range(rng_):
            move_p[c] /= ssum
        ssum = 0.0
        for c in range(rng_):
            ssum += move_p[c]
    if abs(ssum - 1.0) > 1e-6:
        die("netta: move policy is not a distribution (sum=%.9f)\n" % ssum)

def emit_move(m, pos, isl, atomic_ref_bits):
    global unitlm_bits, unitlm_bytes, move_total, move_live_total
    global macro_events, macro_bytes, moves_emitted, prev_move
    global mvlm_bits, mvlm_bytes, mvlm_ref_bits, mvlm_ref_bytes
    live_or_history = move_live_total if tombstone_silence_enabled else \
        move_total
    denom = float(live_or_history) + float(ACTIONS + unit_living)
    pm = (move_count[m] + 1.0) / denom
    nll = -math.log2(pm)
    unitlm_bits += nll
    unitlm_bytes += move_len(m)
    move_count[m] += 1
    move_total += 1
    move_live_total += 1
    if m >= ACTIONS:
        u = units[m - ACTIONS]
        u.uses += 1
        u.last_use = steps_total
        macro_events += 1
        macro_bytes += u.len
        bio_append("m\t%d\t%d\t%d\t%d\t%d\t%.6f\n" %
                   (episode_no, isl, pos, m - ACTIONS, u.len, nll))
    moves_emitted += 1
    if prev_move >= 0:
        pv = prev_move
        live_row_or_history = mv_live_out[pv] \
            if tombstone_silence_enabled else mv_out[pv]
        d2 = float(live_row_or_history) + float(ACTIONS + unit_living)
        p2 = (pair_get(pv, m) + 1.0) / d2
        move_bits = -math.log2(p2)
        mvlm_bits += move_bits
        mvlm_bytes += move_len(m)
        mvlm_ref_bits += atomic_ref_bits
        mvlm_ref_bytes += move_len(m)
        isl_mvlm_bits[isl] += move_bits
        isl_mvlm_ref_bits[isl] += atomic_ref_bits
        isl_mvlm_bytes[isl] += move_len(m)
        mv_out[pv] += 1
        mv_live_out[pv] += 1
        pair_feed(pv, m)
    prev_move = m

def matcher_flush_front():
    global mlen, mstart_pos
    best = 1
    best_u = -1
    for k in range(mlen, 1, -1):
        u = unit_find_living(mbuf, k)
        if u >= 0:
            best = k
            best_u = u
            break
    atomic_ref_bits = 0.0
    for j in range(best):
        atomic_ref_bits += mbuf_atomic_bits[j]
    if best_u >= 0:
        emit_move(ACTIONS + best_u, mstart_pos, mstart_isl,
                  atomic_ref_bits)
    else:
        emit_move(mbuf[0], mstart_pos, mstart_isl, atomic_ref_bits)
    mbuf[0:mlen - best] = mbuf[best:mlen]
    mbuf_atomic_bits[0:mlen - best] = mbuf_atomic_bits[best:mlen]
    mlen -= best
    mstart_pos += best

def matcher_feed(truth, pos, isl, atomic_bits):
    global mlen, mstart_pos, mstart_isl
    if mlen == 0:
        mstart_pos = pos
        mstart_isl = isl
    mbuf[mlen] = truth
    mbuf_atomic_bits[mlen] = atomic_bits
    mlen += 1
    while mlen > 0 and (mlen == UNIT_MAX_LEN or
                        not unit_prefix_alive(mbuf, mlen)):
        if mlen < UNIT_MAX_LEN and unit_prefix_alive(mbuf, mlen):
            break
        if mlen == UNIT_MAX_LEN and unit_find_living(mbuf, mlen) >= 0:
            atomic_ref_bits = 0.0
            for j in range(mlen):
                atomic_ref_bits += mbuf_atomic_bits[j]
            emit_move(ACTIONS + unit_find_living(mbuf, mlen),
                      mstart_pos, mstart_isl, atomic_ref_bits)
            mlen = 0
            break
        matcher_flush_front()

def matcher_end_episode():
    global prev_move
    while mlen > 0:
        matcher_flush_front()
    prev_move = -1

# --------------------------------------------------------------- state

counts = [0] * ACTIONS
counts_total = 0

def state_refuse(path, why):
    die("netta: %s is inconsistent (%s); refusing\n" % (path, why))

def checked_add(total, value, path, what):
    if MASK64 - total < value:
        state_refuse(path, what)
    return total + value

def live_mass_add_unit(u):
    global move_live_total
    move = ACTIONS + u
    if MASK64 - move_live_total < move_count[move]:
        die("netta: live move mass overflow\n")
    move_live_total += move_count[move]
    for i in range(PAIR_SLOTS):
        if pair_cnt[i] and (pair_key[i] & 0xFFFFFFFF) == move:
            prev = pair_key[i] >> 32
            if MASK64 - mv_live_out[prev] < pair_cnt[i]:
                die("netta: live row mass overflow\n")
            mv_live_out[prev] += pair_cnt[i]

def live_mass_remove_unit(u):
    global move_live_total
    move = ACTIONS + u
    if move_live_total < move_count[move]:
        die("netta: live move mass underflow\n")
    move_live_total -= move_count[move]
    for i in range(PAIR_SLOTS):
        if pair_cnt[i] and (pair_key[i] & 0xFFFFFFFF) == move:
            prev = pair_key[i] >> 32
            if mv_live_out[prev] < pair_cnt[i]:
                die("netta: live row mass underflow\n")
            mv_live_out[prev] -= pair_cnt[i]

def live_mass_rebuild(path):
    global move_live_total
    move_live_total = 0
    for i in range(ACTIONS + MAX_UNITS):
        mv_live_out[i] = 0
    for m in range(ACTIONS + len(units)):
        if m < ACTIONS or not units[m - ACTIONS].dead:
            move_live_total = checked_add(move_live_total, move_count[m],
                                          path, "live move mass overflow")
    for i in range(PAIR_SLOTS):
        if pair_cnt[i]:
            prev = pair_key[i] >> 32
            cur = pair_key[i] & 0xFFFFFFFF
            if cur < ACTIONS or not units[cur - ACTIONS].dead:
                mv_live_out[prev] = checked_add(
                    mv_live_out[prev], pair_cnt[i], path,
                    "live row mass overflow")

def score_agrees(path, what, local_sum, global_):
    scale = abs(local_sum)
    if abs(global_) > scale:
        scale = abs(global_)
    if scale < 1.0:
        scale = 1.0
    if abs(local_sum - global_) > 1e-9 + 1e-10 * scale:
        state_refuse(path, what)

_UNIT_PACK = struct.Struct('<16sI4x5Q')
_PAIR_PACK = struct.Struct('<QQ')

def state_save(path):
    dirn = os.path.dirname(path) or '.'
    base = os.path.basename(path)
    try:
        fd, tmp = tempfile.mkstemp(prefix=base + '.tmp.', dir=dirn)
    except OSError:
        die("netta: cannot create state sibling for %s\n" % path)
    f = os.fdopen(fd, 'wb')
    w = f.write
    ver = STATE_VER
    law = neural_law()
    nisl = reg_count
    nunits = len(units)
    core_witness = core_state_witness()
    w(STATE_MAGIC)
    w(struct.pack('<II', ver, law))
    w(struct.pack('<QQQQQ', rng_state, episode_no, steps_total,
                  bio_lines, bio_chain))
    w(struct.pack('<Q', counts_total))
    w(struct.pack('<256Q', *counts))
    w(struct.pack('<I', nunits))
    for u in units:
        w(_UNIT_PACK.pack(u.bytes, u.len, u.born_episode,
                          u.support_at_birth, u.uses, u.last_use, u.dead))
    npairs = 0
    for i in range(PAIR_SLOTS):
        if pair_cnt[i]:
            npairs += 1
    w(struct.pack('<Q', npairs))
    for i in range(PAIR_SLOTS):
        if pair_cnt[i]:
            w(_PAIR_PACK.pack(pair_key[i], pair_cnt[i]))
    w(struct.pack('<QQQQ', macro_events, macro_bytes, moves_emitted,
                  births_rejected_cap))
    w(struct.pack('<%dQ' % (ACTIONS + MAX_UNITS), *move_count))
    w(struct.pack('<Q', move_total))
    w(struct.pack('<dQ', unitlm_bits, unitlm_bytes))
    w(struct.pack('<dQ', atomic_bits_lived, atomic_bytes_lived))
    for row in bi_count:
        w(struct.pack('<256Q', *row))
    w(struct.pack('<256Q', *bi_row))
    w(struct.pack('<dQ', bilm_bits, bilm_bytes))
    w(struct.pack('<%dQ' % (ACTIONS + MAX_UNITS), *mv_out))
    w(struct.pack('<dQ', mvlm_bits, mvlm_bytes))
    w(struct.pack('<dQ', mvlm_ref_bits, mvlm_ref_bytes))
    w(struct.pack('<dQ', mvp_bits, mvp_bytes))
    w(struct.pack('<3d', *mvp_ref_bits))
    w(struct.pack('<Q', mvp_ref_bytes))
    w(struct.pack('<i', actor_current))
    w(struct.pack('<5Q', *ep_actor))
    w(struct.pack('<65536Q', *tri_row))
    w(struct.pack('<dQ', trilm_bits, trilm_bytes))
    w(_floats_bytes(core_Wxh))
    w(_floats_bytes(core_Whh))
    w(_floats_bytes(core_Who))
    w(struct.pack('<ddQ', core_nll_ema, core_bits, core_bytes))
    w(struct.pack('<Q', core_witness))
    w(struct.pack('<I', nisl))
    ntri = 0
    for i in range(TRI_SLOTS):
        if tri_cnt[i]:
            ntri += 1
    w(struct.pack('<Q', ntri))
    for i in range(TRI_SLOTS):
        if tri_cnt[i]:
            w(_PAIR_PACK.pack(tri_key[i], tri_cnt[i]))
    for i in range(reg_count):
        w(struct.pack('<QQQ', reg_digest[i], reg_witness[i], reg_len[i]))
        w(struct.pack('<Q', isl_lived[i]))
        w(struct.pack('<3d', *isl_bits[i]))
        w(struct.pack('<dQ', isl_mvp_bits[i], isl_mvp_bytes[i]))
        w(struct.pack('<3d', *isl_mvp_ref_bits[i]))
        w(struct.pack('<Q', isl_mvp_ref_bytes[i]))
        w(struct.pack('<ddQ', isl_mvlm_bits[i], isl_mvlm_ref_bits[i],
                      isl_mvlm_bytes[i]))
    if jury_enabled:
        witness = jury_state_witness()
        w(_gene_bytes())
        for c in jury_core:
            w(_floats_bytes(c.Wxh))
            w(_floats_bytes(c.Whh))
            w(_floats_bytes(c.Who))
            w(struct.pack('<ddQ', c.nll_ema, c.bits, c.bytes))
            w(b''.join(_DPACK.pack(v) for v in c.row_abs_sum))
            w(struct.pack('<QQQQQ', c.health_bytes, c.near_sat,
                          c.clamp_hits, c.gates_pos, c.gates_neg))
        w(struct.pack('<Q', witness))
    f.close()
    try:
        os.rename(tmp, path)
    except OSError:
        die("netta: cannot publish state %s\n" % path)
# ----------------------------------------------------------- state load

def state_load(path):
    global rng_state, episode_no, steps_total, bio_lines, bio_chain
    global counts_total, unit_living, pair_used, tri_used, reg_count
    global macro_events, macro_bytes, moves_emitted, births_rejected_cap
    global move_total, unitlm_bits, unitlm_bytes
    global atomic_bits_lived, atomic_bytes_lived
    global bilm_bits, bilm_bytes, mvlm_bits, mvlm_bytes
    global mvlm_ref_bits, mvlm_ref_bytes, mvp_bits, mvp_bytes
    global mvp_ref_bytes, actor_current, trilm_bits, trilm_bytes
    global core_nll_ema, core_bits, core_bytes
    try:
        f = open(path, 'rb')
    except OSError as e:
        if e.errno == errno_mod.ENOENT:
            return 0
        die("netta: cannot open state %s; refusing\n" % path)
    data = f.read()
    f.close()
    off = [0]

    def take(fmt):
        s = struct.Struct(fmt)
        if off[0] + s.size > len(data):
            die("netta: %s truncated; refusing\n" % path)
        vals = s.unpack_from(data, off[0])
        off[0] += s.size
        return vals

    if data[:8] != STATE_MAGIC:
        die("netta: %s is not NETTA ZERO state; refusing to touch it\n"
            % path)
    off[0] = 8
    (ver,) = take('<I')
    if ver != STATE_VER:
        die("netta: %s has unknown version; refusing\n" % path)
    (law,) = take('<I')
    if law != neural_law():
        state_refuse(path, "neural invocation law changed")
    rng_state, episode_no, steps_total, bio_lines, bio_chain = \
        take('<QQQQQ')
    (counts_total,) = take('<Q')
    counts[:] = take('<256Q')
    (nunits,) = take('<I')
    if nunits > MAX_UNITS:
        die("netta: %s carries %u units (max %d); refusing\n"
            % (path, nunits, MAX_UNITS))
    del units[:]
    for _ in range(nunits):
        ub, ulen, born, support, uses, last_use, dead = \
            take('<16sI4x5Q')
        u = Unit()
        u.bytes = ub
        u.len = ulen
        u.born_episode = born
        u.support_at_birth = support
        u.uses = uses
        u.last_use = last_use
        u.dead = dead
        units.append(u)
    unit_living = 0
    for ui in range(len(units)):
        u = units[ui]
        if u.len == 0 or u.len > UNIT_MAX_LEN:
            die("netta: %s carries a malformed unit; refusing\n" % path)
        if u.born_episode > episode_no or \
                u.support_at_birth < BIRTH_SUPPORT:
            state_refuse(path, "unit provenance")
        if u.dead > 1 or u.last_use > steps_total:
            state_refuse(path, "unit rent record")
        if not u.dead:
            unit_living += 1
        for j in range(u.len):
            if is_ws(u.bytes[j]):
                state_refuse(path, "unit crosses a boundary")
        for v in range(ui):
            if units[v].len == u.len and \
                    units[v].bytes[:u.len] == u.bytes[:u.len]:
                state_refuse(path, "duplicate unit identity")
    (npairs,) = take('<Q')
    if npairs > PAIR_SLOTS:
        die("netta: %s truncated; refusing\n" % path)
    for _ in range(npairs):
        key, cnt = take('<QQ')
        if cnt == 0:
            die("netta: %s truncated; refusing\n" % path)
        prev = key >> 32
        cur = key & 0xFFFFFFFF
        if prev >= ACTIONS + nunits or cur >= ACTIONS + nunits:
            state_refuse(path, "pair names a nonexistent move")
        h = _key_hash(key)
        inserted = 0
        for probe in range(PAIR_SLOTS):
            idx = (h + probe) & (PAIR_SLOTS - 1)
            if pair_cnt[idx] == 0:
                pair_key[idx] = key
                pair_cnt[idx] = cnt
                pair_used += 1
                inserted = 1
                break
            if pair_key[idx] == key:
                state_refuse(path, "duplicate pair key")
        if not inserted:
            state_refuse(path, "pair table has no free slot")
    macro_events, macro_bytes, moves_emitted, births_rejected_cap = \
        take('<QQQQ')
    move_count[:] = take('<%dQ' % (ACTIONS + MAX_UNITS))
    (move_total,) = take('<Q')
    unitlm_bits, unitlm_bytes = take('<dQ')
    atomic_bits_lived, atomic_bytes_lived = take('<dQ')
    for pv in range(ACTIONS):
        bi_count[pv][:] = take('<256Q')
    bi_row[:] = take('<256Q')
    bilm_bits, bilm_bytes = take('<dQ')
    mv_out[:] = take('<%dQ' % (ACTIONS + MAX_UNITS))
    mvlm_bits, mvlm_bytes = take('<dQ')
    mvlm_ref_bits, mvlm_ref_bytes = take('<dQ')
    mvp_bits, mvp_bytes = take('<dQ')
    mvp_ref_bits[:] = take('<3d')
    (mvp_ref_bytes,) = take('<Q')
    (actor_current,) = take('<i')
    ep_actor[:] = take('<5Q')
    tri_row[:] = take('<65536Q')
    trilm_bits, trilm_bytes = take('<dQ')
    for j in range(CORE_HIDDEN):
        core_Wxh[j][:] = take('<%df' % CORE_EMBED)
    for j in range(CORE_HIDDEN):
        core_Whh[j][:] = take('<%df' % CORE_HIDDEN)
    for o in range(ACTIONS):
        core_Who[o][:] = take('<%df' % CORE_HIDDEN)
    core_nll_ema, core_bits, core_bytes = take('<ddQ')
    (stored_core_witness,) = take('<Q')
    (nisl,) = take('<I')
    (ntri,) = take('<Q')
    if ntri > TRI_SLOTS:
        die("netta: %s truncated; refusing\n" % path)
    for _ in range(ntri):
        key, cnt = take('<QQ')
        if cnt == 0:
            die("netta: %s truncated; refusing\n" % path)
        if key > 0xffffff:
            state_refuse(path, "trigram key is outside the byte world")
        h = _key_hash(key)
        inserted = 0
        for probe in range(TRI_SLOTS):
            idx = (h + probe) & (TRI_SLOTS - 1)
            if tri_cnt[idx] == 0:
                tri_key[idx] = key
                tri_cnt[idx] = cnt
                tri_used += 1
                inserted = 1
                break
            if tri_key[idx] == key:
                state_refuse(path, "duplicate trigram key")
        if not inserted:
            state_refuse(path, "trigram table has no free slot")
    if nisl > MAX_REGISTRY:
        state_refuse(path, "registry larger than the constitution")
    reg_count = nisl
    for i in range(reg_count):
        reg_digest[i], reg_witness[i], reg_len[i] = take('<QQQ')
        (isl_lived[i],) = take('<Q')
        isl_bits[i][:] = take('<3d')
        isl_mvp_bits[i], isl_mvp_bytes[i] = take('<dQ')
        isl_mvp_ref_bits[i][:] = take('<3d')
        (isl_mvp_ref_bytes[i],) = take('<Q')
        isl_mvlm_bits[i], isl_mvlm_ref_bits[i], isl_mvlm_bytes[i] = \
            take('<ddQ')
        for r in range(i):
            if (reg_digest[r] == reg_digest[i] and
                    reg_witness[r] == reg_witness[i] and
                    reg_len[r] == reg_len[i]):
                die("netta: %s carries a duplicate island identity; "
                    "refusing\n" % path)
    if jury_enabled:
        stored_gene = take('<%df' % (JURY_GENOMES * 7))
        mine = tuple(v for g in jury_gene for v in g)
        if b''.join(_FPACK.pack(v) for v in stored_gene) != \
                b''.join(_FPACK.pack(v) for v in mine):
            state_refuse(path, "jury genes disagree with the sealed court")
        for c in jury_core:
            for j in range(CORE_HIDDEN):
                c.Wxh[j][:] = take('<%df' % CORE_EMBED)
            for j in range(CORE_HIDDEN):
                c.Whh[j][:] = take('<%df' % CORE_HIDDEN)
            for o in range(ACTIONS):
                c.Who[o][:] = take('<%df' % CORE_HIDDEN)
            c.nll_ema, c.bits, c.bytes = take('<ddQ')
            c.row_abs_sum[:] = take('<%dd' % CORE_HIDDEN)
            (c.health_bytes, c.near_sat, c.clamp_hits,
             c.gates_pos, c.gates_neg) = take('<QQQQQ')
        (stored_jury_witness,) = take('<Q')
        if stored_jury_witness != jury_state_witness():
            state_refuse(path, "jury memory disagrees with its witness")
    if off[0] != len(data):
        state_refuse(path, "trailing or unreadable bytes")

    if actor_current < 0 or actor_current > 3:
        state_refuse(path, "actor seat")
    total = 0
    for b in range(ACTIONS):
        total = checked_add(total, counts[b], path,
                            "atomic count overflow")
    if (total != counts_total or counts_total != steps_total or
            atomic_bytes_lived != steps_total or
            bilm_bytes != steps_total or trilm_bytes != steps_total):
        state_refuse(path, "raw-byte totals disagree")
    if (unitlm_bytes > steps_total or mvlm_bytes > unitlm_bytes or
            mvlm_ref_bytes != mvlm_bytes or
            mvp_bytes > steps_total or mvp_ref_bytes != mvp_bytes or
            macro_bytes > steps_total or macro_events > moves_emitted):
        state_refuse(path, "move totals exceed lived truth")
    total = 0
    for m in range(ACTIONS + MAX_UNITS):
        total = checked_add(total, move_count[m], path,
                            "move count overflow")
    if total != move_total or move_total != moves_emitted:
        state_refuse(path, "move totals disagree")
    for m in range(ACTIONS + len(units), ACTIONS + MAX_UNITS):
        if move_count[m] != 0 or mv_out[m] != 0:
            state_refuse(path, "unborn move carries statistics")
    actor_eps = 0
    for a in range(5):
        actor_eps = checked_add(actor_eps, ep_actor[a], path,
                                "actor episode overflow")
    if actor_eps != episode_no:
        state_refuse(path, "actor episodes disagree")
    isl_lived_sum = 0
    isl_mv_sum = 0
    isl_shadow_sum = 0
    isl_score_sum = [0.0, 0.0, 0.0]
    isl_mv_score_sum = 0.0
    isl_mv_ref_sum = [0.0, 0.0, 0.0]
    isl_shadow_score_sum = 0.0
    isl_shadow_ref_sum = 0.0
    for i in range(reg_count):
        isl_lived_sum = checked_add(isl_lived_sum, isl_lived[i], path,
                                    "island lived overflow")
        isl_mv_sum = checked_add(isl_mv_sum, isl_mvp_bytes[i], path,
                                 "island move overflow")
        isl_shadow_sum = checked_add(isl_shadow_sum, isl_mvlm_bytes[i],
                                     path, "island shadow overflow")
        if (isl_mvp_ref_bytes[i] != isl_mvp_bytes[i] or
                isl_mvp_bytes[i] > isl_lived[i]):
            state_refuse(path, "island move evidence disagrees")
        for c in range(3):
            if (not math.isfinite(isl_bits[i][c]) or
                    isl_bits[i][c] < 0.0 or
                    not math.isfinite(isl_mvp_ref_bits[i][c]) or
                    isl_mvp_ref_bits[i][c] < 0.0):
                state_refuse(path, "non-finite island record")
            else:
                isl_score_sum[c] += isl_bits[i][c]
                isl_mv_ref_sum[c] += isl_mvp_ref_bits[i][c]
        if not math.isfinite(isl_mvp_bits[i]) or isl_mvp_bits[i] < 0.0:
            state_refuse(path, "non-finite island record")
        isl_mv_score_sum += isl_mvp_bits[i]
        if (not math.isfinite(isl_mvlm_bits[i]) or
                isl_mvlm_bits[i] < 0.0 or
                not math.isfinite(isl_mvlm_ref_bits[i]) or
                isl_mvlm_ref_bits[i] < 0.0 or
                isl_mvlm_bytes[i] > isl_lived[i]):
            state_refuse(path, "island shadow record")
        isl_shadow_score_sum += isl_mvlm_bits[i]
        isl_shadow_ref_sum += isl_mvlm_ref_bits[i]
    if (isl_lived_sum != steps_total or isl_mv_sum != mvp_bytes or
            isl_shadow_sum != mvlm_bytes):
        state_refuse(path, "island records disagree with the life")
    score_agrees(path, "island atomic score disagrees",
                 isl_score_sum[0], atomic_bits_lived)
    score_agrees(path, "island bigram score disagrees",
                 isl_score_sum[1], bilm_bits)
    score_agrees(path, "island trigram score disagrees",
                 isl_score_sum[2], trilm_bits)
    score_agrees(path, "island move score disagrees",
                 isl_mv_score_sum, mvp_bits)
    for c in range(3):
        score_agrees(path, "island move reference score disagrees",
                     isl_mv_ref_sum[c], mvp_ref_bits[c])
    score_agrees(path, "island shadow score disagrees",
                 isl_shadow_score_sum, mvlm_bits)
    score_agrees(path, "island shadow reference score disagrees",
                 isl_shadow_ref_sum, mvlm_ref_bits)
    if (not math.isfinite(unitlm_bits) or unitlm_bits < 0.0 or
            not math.isfinite(atomic_bits_lived) or
            atomic_bits_lived < 0.0 or
            not math.isfinite(bilm_bits) or bilm_bits < 0.0 or
            not math.isfinite(mvlm_bits) or mvlm_bits < 0.0 or
            not math.isfinite(mvlm_ref_bits) or mvlm_ref_bits < 0.0 or
            not math.isfinite(mvp_bits) or mvp_bits < 0.0 or
            not math.isfinite(mvp_ref_bits[0]) or mvp_ref_bits[0] < 0.0 or
            not math.isfinite(mvp_ref_bits[1]) or mvp_ref_bits[1] < 0.0 or
            not math.isfinite(mvp_ref_bits[2]) or mvp_ref_bits[2] < 0.0 or
            not math.isfinite(trilm_bits) or trilm_bits < 0.0):
        state_refuse(path, "non-finite model record")
    if (not math.isfinite(core_bits) or core_bits < 0.0 or
            core_bytes > steps_total or
            not math.isfinite(core_nll_ema) or core_nll_ema < 0.0 or
            core_nll_ema > 16.0):
        state_refuse(path, "core record")
    for j in range(CORE_HIDDEN):
        for d in range(CORE_EMBED):
            if not math.isfinite(core_Wxh[j][d]) or \
                    abs(core_Wxh[j][d]) > CORE_WCLAMP:
                state_refuse(path, "core weight out of law")
        for k in range(CORE_HIDDEN):
            if not math.isfinite(core_Whh[j][k]) or \
                    abs(core_Whh[j][k]) > CORE_WCLAMP:
                state_refuse(path, "core weight out of law")
    for b in range(ACTIONS):
        for d in range(CORE_EMBED):
            if not math.isfinite(core_E[b][d]):
                state_refuse(path, "core weight out of law")
        for j in range(CORE_HIDDEN):
            if not math.isfinite(core_Who[b][j]) or \
                    abs(core_Who[b][j]) > CORE_WCLAMP:
                state_refuse(path, "core weight out of law")
    if stored_core_witness != core_state_witness():
        state_refuse(path, "core memory disagrees with its witness")
    if jury_enabled:
        for c in jury_core:
            if (not math.isfinite(c.bits) or c.bits < 0.0 or
                    not math.isfinite(c.nll_ema) or c.nll_ema < 0.0 or
                    c.nll_ema > 16.0 or c.bytes != steps_total or
                    c.health_bytes != c.bytes or
                    c.bytes > MASK64 // CORE_HIDDEN or
                    c.near_sat > c.bytes * CORE_HIDDEN):
                state_refuse(path, "jury record")
            if (c.gates_pos > c.bytes or c.gates_neg > c.bytes or
                    c.gates_pos > c.bytes - c.gates_neg):
                state_refuse(path, "jury gate record")
            for j in range(CORE_HIDDEN):
                if (not math.isfinite(c.row_abs_sum[j]) or
                        c.row_abs_sum[j] < 0.0 or
                        c.row_abs_sum[j] > c.health_bytes + 1e-6):
                    state_refuse(path, "jury health record")
                for d in range(CORE_EMBED):
                    if not math.isfinite(c.Wxh[j][d]) or \
                            abs(c.Wxh[j][d]) > CORE_WCLAMP:
                        state_refuse(path, "jury weight out of law")
                for k in range(CORE_HIDDEN):
                    if not math.isfinite(c.Whh[j][k]) or \
                            abs(c.Whh[j][k]) > CORE_WCLAMP:
                        state_refuse(path, "jury weight out of law")
            for b in range(ACTIONS):
                for j in range(CORE_HIDDEN):
                    if not math.isfinite(c.Who[b][j]) or \
                            abs(c.Who[b][j]) > CORE_WCLAMP:
                        state_refuse(path, "jury weight out of law")
    for pv in range(ACTIONS):
        row = 0
        for b in range(ACTIONS):
            row = checked_add(row, bi_count[pv][b], path,
                              "bigram row overflow")
        if row != bi_row[pv]:
            state_refuse(path, "bigram row total")
    pair_rows = [0] * (ACTIONS + MAX_UNITS)
    tri_rows = [0] * 65536
    for i in range(PAIR_SLOTS):
        if pair_cnt[i]:
            prev = pair_key[i] >> 32
            pair_rows[prev] = checked_add(pair_rows[prev], pair_cnt[i],
                                          path, "pair row overflow")
    for m in range(ACTIONS + MAX_UNITS):
        if pair_rows[m] != mv_out[m]:
            state_refuse(path, "move-bigram row total")
    live_mass_rebuild(path)
    for i in range(TRI_SLOTS):
        if tri_cnt[i]:
            ctx = tri_key[i] >> 8
            tri_rows[ctx] = checked_add(tri_rows[ctx], tri_cnt[i], path,
                                        "trigram row overflow")
    for ctx in range(65536):
        if tri_rows[ctx] != tri_row[ctx]:
            state_refuse(path, "trigram row total")
    return 1

# ---------------------------------------------------------------- game

def policy(p):
    denom = float(counts_total) + float(ACTIONS)
    ssum = 0.0
    for b in range(ACTIONS):
        p[b] = (counts[b] + 1.0) / denom
        ssum += p[b]
    if abs(ssum - 1.0) > 1e-6:
        die("netta: policy is not a distribution (sum=%.9f)\n" % ssum)

def sample(p):
    r = (rng_next() >> 11) * (1.0 / 9007199254740992.0)
    acc = 0.0
    for b in range(ACTIONS):
        acc += p[b]
        if r < acc:
            return b
    return ACTIONS - 1

def build_dists(isl, pos, p_uni, p_bi, p_tri):
    pv = isl.bytes[pos - 1]
    tctx = (isl.bytes[pos - 2] << 8) | pv
    policy(p_uni)
    d2 = float(bi_row[pv]) + float(ACTIONS)
    s2 = 0.0
    row = bi_count[pv]
    for b in range(ACTIONS):
        p_bi[b] = (row[b] + 1.0) / d2
        s2 += p_bi[b]
    if abs(s2 - 1.0) > 1e-6:
        die("netta: bigram row is not a distribution (sum=%.9f)\n" % s2)
    d3 = float(tri_row[tctx]) + float(ACTIONS)
    s3 = 0.0
    for b in range(ACTIONS):
        p_tri[b] = (tri_get(tctx, b) + 1.0) / d3
        s3 += p_tri[b]
    if abs(s3 - 1.0) > 1e-6:
        die("netta: trigram row is not a distribution (sum=%.9f)\n" % s3)

def absorb_truth(isl_id, isl, pos, p_uni, p_bi, p_tri):
    global atomic_bits_lived, atomic_bytes_lived, bilm_bits, bilm_bytes
    global trilm_bits, trilm_bytes, counts_total, steps_total
    truth = isl.bytes[pos]
    pv = isl.bytes[pos - 1]
    tctx = (isl.bytes[pos - 2] << 8) | pv
    atomic_nll = -math.log2(p_uni[truth])
    atomic_bits_lived += atomic_nll
    atomic_bytes_lived += 1
    bilm_bits += -math.log2(p_bi[truth])
    bilm_bytes += 1
    trilm_bits += -math.log2(p_tri[truth])
    trilm_bytes += 1
    isl_bits[isl_id][0] += atomic_nll
    isl_bits[isl_id][1] += -math.log2(p_bi[truth])
    isl_bits[isl_id][2] += -math.log2(p_tri[truth])
    isl_lived[isl_id] += 1
    counts[truth] += 1
    counts_total += 1
    steps_total += 1
    bi_count[pv][truth] += 1
    bi_row[pv] += 1
    tri_add(tctx, truth)
    if core_enabled:
        core_absorb(truth)
    if jury_enabled:
        jury_absorb(truth)
    if units_enabled:
        matcher_feed(truth, pos, isl_id, atomic_nll)

def run_episode(isl_id, steps):
    global episode_no, unit_living, unit_deaths, revocations
    global mvp_bits, mvp_bytes, mvp_ref_bytes, mvc_bits, mvc_bytes
    global move_nav_steps, move_nav_unit_anchors
    isl = islands[isl_id]
    reg = present_reg[isl_id]
    if steps == 0 or isl.len <= CTX or steps > isl.len - CTX - 1:
        die("netta: island %d too small for %d steps\n" % (isl_id, steps))
    span = isl.len - CTX - steps
    if fixed_start_set:
        if (fixed_start < CTX or fixed_start >= isl.len or
                steps > isl.len - fixed_start):
            die("netta: fixed start %d cannot hold %d steps on "
                "island %d\n" % (fixed_start, steps, isl_id))
        start = fixed_start
    else:
        start = CTX + (rng_next() % (span if span else 1))
    if core_enabled:
        core_warm(isl, start)
    if jury_enabled:
        jury_warm(isl, start)
    bits = 0.0
    episode_no += 1
    if units_enabled and unit_death_enabled:
        for u in range(len(units)):
            if units[u].dead or \
                    steps_total - units[u].last_use < UNIT_TTL:
                continue
            live_mass_remove_unit(u)
            units[u].dead = 1
            unit_living -= 1
            unit_deaths += 1
            bio_append("d\t%d\t%d\t%d\t%d\n" %
                       (episode_no, u, units[u].uses,
                        steps_total - units[u].last_use))
    actor_elect()
    acting = island_court(reg, steps)
    probation = 0
    door_move_bits = isl_mvlm_bits[reg] if local_probation_enabled \
        else mvlm_bits
    door_ref_bits = isl_mvlm_ref_bits[reg] if local_probation_enabled \
        else mvlm_ref_bits
    door_bytes = isl_mvlm_bytes[reg] if local_probation_enabled \
        else mvlm_bytes
    if (actor_lock < 0 and units_enabled and 0 <= acting <= 2 and
            episode_no % 8 == 7 and door_bytes >= ACTOR_MIN_BYTES and
            door_ref_bits / door_bytes - door_move_bits / door_bytes >=
            ACTOR_GAIN):
        acting = 3
        probation = 1
    ep_actor[acting] += 1
    bio_append("a\t%d\t%s\n" %
               (episode_no, "mvp" if probation else actor_name[acting]))
    if acting == 3:
        s = 0
        player_prev = -1
        p_uni = [0.0] * ACTIONS
        p_bi = [0.0] * ACTIONS
        p_tri = [0.0] * ACTIONS
        move_p = [0.0] * (ACTIONS + MAX_UNITS)
        while s < steps:
            pos = start + s
            rng_ = ACTIONS + len(units)
            alphabet = ACTIONS + unit_living
            policy_prev = player_prev
            if move_nav_enabled:
                policy_prev = move_route_anchor(isl, pos, rng_, alphabet)
                move_nav_steps += 1
                if policy_prev >= ACTIONS:
                    move_nav_unit_anchors += 1
            build_move_dist(policy_prev, rng_, alphabet,
                            move_nav_enabled, move_p)
            r = (rng_next() >> 11) * (1.0 / 9007199254740992.0)
            acc = 0.0
            m = 0
            for c in range(rng_ - 1, -1, -1):
                if c < ACTIONS or not units[c - ACTIONS].dead:
                    m = c
                    break
            for c in range(rng_):
                acc += move_p[c]
                if r < acc:
                    m = c
                    break
            mb = move_bytes(m)
            L = move_len(m)
            room = steps - s
            matched = 0
            while matched < L and matched < room and \
                    mb[matched] == isl.bytes[pos + matched]:
                matched += 1
            advance = matched if matched else 1
            if advance > room:
                advance = room
            target = truth_move(isl, pos, room)
            nll = -math.log2(move_p[target])
            if move_nav_enabled:
                bio_append("v\t%d\t%d\t%d\t%d\t%d\t%d\t%.6f\t%d\t%d\n" %
                           (episode_no, reg, pos, m, L, advance, nll,
                            target, policy_prev))
            else:
                bio_append("v\t%d\t%d\t%d\t%d\t%d\t%d\t%.6f\t%d\n" %
                           (episode_no, reg, pos, m, L, advance, nll,
                            target))
            bits += nll
            if actor_lock == 3:
                mvc_bits += nll
                mvc_bytes += advance
            else:
                mvp_bits += nll
                mvp_bytes += advance
                isl_mvp_bits[reg] += nll
                isl_mvp_bytes[reg] += advance
            player_prev = m
            for j in range(advance):
                build_dists(isl, pos + j, p_uni, p_bi, p_tri)
                truth = isl.bytes[pos + j]
                if actor_lock == 3:
                    mvc_ref_bits[0] += -math.log2(p_uni[truth])
                    mvc_ref_bits[1] += -math.log2(p_bi[truth])
                    mvc_ref_bits[2] += -math.log2(p_tri[truth])
                else:
                    mvp_ref_bits[0] += -math.log2(p_uni[truth])
                    mvp_ref_bits[1] += -math.log2(p_bi[truth])
                    mvp_ref_bits[2] += -math.log2(p_tri[truth])
                    mvp_ref_bytes += 1
                    isl_mvp_ref_bits[reg][0] += -math.log2(p_uni[truth])
                    isl_mvp_ref_bits[reg][1] += -math.log2(p_bi[truth])
                    isl_mvp_ref_bits[reg][2] += -math.log2(p_tri[truth])
                    isl_mvp_ref_bytes[reg] += 1
                absorb_truth(reg, isl, pos + j, p_uni, p_bi, p_tri)
            s += advance
        if units_enabled:
            matcher_end_episode()
        return bits / steps
    p_uni = [0.0] * ACTIONS
    p_bi = [0.0] * ACTIONS
    p_tri = [0.0] * ACTIONS
    for s in range(steps):
        pos = start + s
        ctx_digest = fnv1a64(isl.bytes[pos - CTX:pos], FNV_SEED)
        rng_before = rng_state
        build_dists(isl, pos, p_uni, p_bi, p_tri)
        p_act = p_tri if acting == 2 else p_bi if acting == 1 else \
            p_uni if acting == 0 else None
        action = (rng_next() >> 56) if acting == ACTOR_NULL \
            else sample(p_act)
        truth = isl.bytes[pos]
        loss = 8.0 if acting == ACTOR_NULL else -math.log2(p_act[truth])
        bio_append("%d\t%d\t%d\t%d\t%016x\t%d\t%d\t%.6f\t%016x\t"
                   "atomic\t1\t%s\n" %
                   (episode_no, s, reg, pos, ctx_digest, action, truth,
                    loss, rng_before, actor_name[acting]))
        absorb_truth(reg, isl, pos, p_uni, p_bi, p_tri)
        bits += loss
    if units_enabled:
        matcher_end_episode()
    return bits / steps

# ---------------------------------------------------------------- main

def parse_u64(flag, s):
    if not s or s[0] == '-':
        die("netta: %s takes a non-negative integer\n" % flag)
    if not s.isascii() or not s.isdigit():
        die("netta: invalid integer for %s: %s\n" % (flag, s))
    v = int(s)
    if v > MASK64:
        die("netta: invalid integer for %s: %s\n" % (flag, s))
    return v

def parse_int(flag, s):
    try:
        v = int(s, 10)
    except ValueError:
        die("netta: invalid integer for %s: %s\n" % (flag, s))
    if not s or v < -2147483648 or v > 2147483647:
        die("netta: invalid integer for %s: %s\n" % (flag, s))
    return v

def same_file(a, b):
    if a == b:
        return 1
    try:
        sa = os.stat(a)
        sb = os.stat(b)
    except OSError:
        return 0
    return sa.st_dev == sb.st_dev and sa.st_ino == sb.st_ino

def fd_is_file(fd, path):
    try:
        sa = os.fstat(fd)
        sb = os.stat(path)
    except OSError:
        return 0
    return sa.st_dev == sb.st_dev and sa.st_ino == sb.st_ino

def fd_writable(fd):
    import fcntl as fcntl_mod
    try:
        flags = fcntl_mod.fcntl(fd, fcntl_mod.F_GETFL)
    except OSError:
        return 0
    return (flags & os.O_ACCMODE) != os.O_RDONLY

def fds_share_framed_sink(a, b):
    try:
        sa = os.fstat(a)
        sb = os.fstat(b)
    except OSError:
        return 0
    if sa.st_dev != sb.st_dev or sa.st_ino != sb.st_ino:
        return 0
    return not stat_mod.S_ISCHR(sa.st_mode)

def file_exists(path):
    try:
        os.stat(path)
        return 1
    except OSError as e:
        if e.errno == errno_mod.ENOENT:
            return 0
        die("netta: cannot inspect %s; refusing\n" % path)

# ------------------------------------------------------------ the mouth

speak_bytes = 0
speak_seed = 1
prompt_path = None
speak_requested = 0
speak_seed_set = 0
speak_laplace = 0
ear_path = None
ear_context_path = None
ear_twin_requested = 0
court_path = None
life_control_named = 0
instrument_only_named = 0
cite_path = None
sign_requested = 0
spoke_digest = 0
spoke_hand = 0
spoke_open2 = 0
spoke_open1 = 0

def question_tail(path, name, law_name):
    try:
        f = open(path, 'rb')
    except OSError:
        die("netta: cannot open %s %s\n" % (name, path))
    try:
        before = os.fstat(f.fileno())
    except OSError:
        f.close()
        die("netta: cannot read %s %s\n" % (name, path))

    if stat_mod.S_ISREG(before.st_mode):
        if before.st_size < 2:
            f.close()
            die("netta: %s needs at least two bytes\n" % law_name)
        failed = 0
        changed = 0
        try:
            f.seek(-2, os.SEEK_END)
            first = f.read(2)
            middle = os.fstat(f.fileno())
            if (len(first) != 2 or before.st_dev != middle.st_dev or
                    before.st_ino != middle.st_ino or
                    before.st_size != middle.st_size):
                failed = 1
            else:
                f.seek(-2, os.SEEK_END)
                second = f.read(2)
                after = os.fstat(f.fileno())
                if (len(second) != 2 or middle.st_dev != after.st_dev or
                        middle.st_ino != after.st_ino or
                        middle.st_size != after.st_size):
                    failed = 1
                elif first != second:
                    changed = 1
        except OSError:
            failed = 1
        f.close()
        if failed:
            die("netta: cannot read %s %s\n" % (name, path))
        if changed:
            die("netta: %s changed while being read\n" % name)
        return first[0], first[1]

    p2 = 0
    p1 = 0
    have = 0
    while True:
        c = f.read(1)
        if not c:
            break
        p2 = p1
        p1 = c[0]
        if have < 2:
            have += 1
    f.close()
    if have < 2:
        die("netta: %s needs at least two bytes\n" % law_name)
    return p2, p1

def speak_next(s):
    s[0] = (s[0] + 0x9e3779b97f4a7c15) & MASK64
    z = s[0]
    z = ((z ^ (z >> 30)) * 0xbf58476d1ce4e5b9) & MASK64
    z = ((z ^ (z >> 27)) * 0x94d049bb133111eb) & MASK64
    return z ^ (z >> 31)

def speak_bounded(s, bound):
    threshold = ((1 << 64) - bound) % bound
    while True:
        r = speak_next(s)
        if r >= threshold:
            return r % bound

def speak_supported(hand, p2, p1, s):
    tctx = (p2 << 8) | p1
    if hand == 2 and tri_row[tctx]:
        used = 2
        total = tri_row[tctx]
    elif hand >= 1 and bi_row[p1]:
        used = 1
        total = bi_row[p1]
    else:
        used = 0
        total = counts_total
    draw = speak_bounded(s, total)
    for b in range(ACTIONS):
        if used == 2:
            n = tri_get(tctx, b)
        elif used == 1:
            n = bi_count[p1][b]
        else:
            n = counts[b]
        if draw < n:
            return b, used
        draw -= n
    die("netta: spoken support totals disagree\n")

def speak_laplace_distribution(hand, p2, p1, p_act):
    if hand == 1:
        d2 = float(bi_row[p1]) + float(ACTIONS)
        for b in range(ACTIONS):
            p_act[b] = (bi_count[p1][b] + 1.0) / d2
    elif hand == 2:
        tctx = (p2 << 8) | p1
        d3 = float(tri_row[tctx]) + float(ACTIONS)
        for b in range(ACTIONS):
            p_act[b] = (tri_get(tctx, b) + 1.0) / d3
    else:
        policy(p_act)

def speak():
    global spoke_digest, spoke_hand, spoke_open2, spoke_open1
    p2 = 0
    p1 = 0
    best = 0
    for pv in range(ACTIONS):
        row = bi_count[pv]
        for b in range(ACTIONS):
            if row[b] > best:
                best = row[b]
                p2 = pv
                p1 = b
    if prompt_path:
        p2, p1 = question_tail(prompt_path, "prompt", "a prompt")
    actor_elect()
    hand = actor_current
    if hand == 3:
        lu = atomic_bits_lived / atomic_bytes_lived
        lb = (bilm_bits / bilm_bytes) if bilm_bytes else 1e9
        lt = (trilm_bits / trilm_bytes) if trilm_bytes else 1e9
        hand = 0
        if lb < lu:
            hand = 1
            lu = lb
        if lt < lu:
            hand = 2
    sys.stderr.write("speak: %d bytes, hand %s, seed %d, law %s\n" %
                     (speak_bytes, actor_name[hand], speak_seed,
                      "laplace-red" if speak_laplace
                      else "supported-backoff"))
    s = [speak_seed]
    support_order = [0, 0, 0]
    open2 = p2
    open1 = p1
    spoken = FNV_SEED
    out = sys.stdout.buffer
    for _ in range(speak_bytes):
        if not speak_laplace:
            b, used = speak_supported(hand, p2, p1, s)
            support_order[used] += 1
        else:
            p_act = [0.0] * ACTIONS
            speak_laplace_distribution(hand, p2, p1, p_act)
            r = (speak_next(s) >> 11) * (1.0 / 9007199254740992.0)
            acc = 0.0
            b = ACTIONS - 1
            for c in range(ACTIONS):
                acc += p_act[c]
                if r < acc:
                    b = c
                    break
        out.write(bytes((b,)))
        p2 = p1
        p1 = b
        spoken = fnv1a64(bytes((p1,)), spoken)
    try:
        out.flush()
    except OSError:
        die("netta: speak flush failed\n")
    if not speak_laplace:
        sys.stderr.write("speak support: uni %d, bi %d, tri %d\n" %
                         tuple(support_order))
    sys.stderr.write(
        "spoke: candidate-digest=%016x bytes=%d seed=%d hand=%s "
        "law=%s opening=%02x%02x lived-bytes=%d episode=%d\n" %
        (spoken, speak_bytes, speak_seed, actor_name[hand],
         "laplace-red" if speak_laplace else "supported-backoff",
         open2, open1, atomic_bytes_lived, episode_no))
    spoke_digest = spoken
    spoke_hand = hand
    spoke_open2 = open2
    spoke_open1 = open1

# -------------------------------------------------------------- the ear

EAR_MAX = 1 << 14
EAR_MATCH_MIN = 16
EAR_SAM_NONE = 0xFFFFFFFF

class EarSam:
    __slots__ = ('cap', 'states', 'last', 'next', 'len', 'prefix', 'best',
                 'heard', 'order', 'count', 'link')

def ear_sam_get(a, state, b):
    v = a.next[state * ACTIONS + b]
    return v - 1 if v else EAR_SAM_NONE

def ear_sam_set(a, state, b, target):
    a.next[state * ACTIONS + b] = target + 1

def ear_sam_build(sp, n):
    a = EarSam()
    a.cap = n * 2
    a.next = [0] * (a.cap * ACTIONS)
    a.len = [0] * a.cap
    a.prefix = [0] * n
    a.best = [0] * a.cap
    a.heard = [0] * a.cap
    a.order = [0] * a.cap
    a.count = [0] * (n + 1)
    a.link = [0] * a.cap
    a.states = 1
    a.last = 0
    a.link[0] = -1
    for i in range(n):
        b = sp[i]
        if a.states >= a.cap:
            die("netta: ear automaton full\n")
        cur = a.states
        a.states += 1
        a.len[cur] = a.len[a.last] + 1
        p = a.last
        while p >= 0 and ear_sam_get(a, p, b) == EAR_SAM_NONE:
            ear_sam_set(a, p, b, cur)
            p = a.link[p]
        if p < 0:
            a.link[cur] = 0
        else:
            q = ear_sam_get(a, p, b)
            if a.len[p] + 1 == a.len[q]:
                a.link[cur] = q
            else:
                if a.states >= a.cap:
                    die("netta: ear automaton full\n")
                clone = a.states
                a.states += 1
                a.len[clone] = a.len[p] + 1
                a.link[clone] = a.link[q]
                a.next[clone * ACTIONS:(clone + 1) * ACTIONS] = \
                    a.next[q * ACTIONS:(q + 1) * ACTIONS]
                while p >= 0 and ear_sam_get(a, p, b) == q:
                    ear_sam_set(a, p, b, clone)
                    p = a.link[p]
                a.link[q] = clone
                a.link[cur] = clone
        a.last = cur
        a.prefix[i] = cur
    for q in range(a.states):
        a.count[a.len[q]] += 1
    for i in range(1, n + 1):
        a.count[i] += a.count[i - 1]
    for q in range(a.states - 1, -1, -1):
        a.count[a.len[q]] -= 1
        a.order[a.count[a.len[q]]] = q
    return a

def ear_sam_match(a, isl, maxend):
    for i in range(a.states):
        a.best[i] = 0
        a.heard[i] = 0
    v = 0
    length = 0
    for i in range(isl.len):
        b = isl.bytes[i]
        to = ear_sam_get(a, v, b)
        while v and to == EAR_SAM_NONE:
            v = a.link[v]
            if length > a.len[v]:
                length = a.len[v]
            to = ear_sam_get(a, v, b)
        if to != EAR_SAM_NONE:
            v = to
            length += 1
        else:
            v = 0
            length = 0
        if length > a.best[v]:
            a.best[v] = length
    for oi in range(a.states - 1, 0, -1):
        q = a.order[oi]
        p = a.link[q]
        carry = a.best[q]
        if carry > a.len[p]:
            carry = a.len[p]
        if carry > a.best[p]:
            a.best[p] = carry
    for oi in range(1, a.states):
        q = a.order[oi]
        p = a.link[q]
        a.heard[q] = a.best[q] if a.best[q] > a.heard[p] else a.heard[p]
    for i in range(a.len[a.last]):
        maxend[i] = a.heard[a.prefix[i]]

def ear_grow(isl):
    global counts_total, tri_used, tri_key, tri_cnt
    for b in range(ACTIONS):
        counts[b] = 0
    counts_total = 0
    for pv in range(ACTIONS):
        row = bi_count[pv]
        for b in range(ACTIONS):
            row[b] = 0
        bi_row[pv] = 0
    tri_key = [0] * TRI_SLOTS
    tri_cnt = [0] * TRI_SLOTS
    for ctx in range(65536):
        tri_row[ctx] = 0
    tri_used = 0
    global steps_total
    data = isl.bytes
    for i in range(isl.len):
        b = data[i]
        counts[b] += 1
        counts_total += 1
        if i >= 1:
            p1 = data[i - 1]
            bi_count[p1][b] += 1
            bi_row[p1] += 1
            if i >= 2:
                tri_add((data[i - 2] << 8) | p1, b)

def ear_price(isl, sp, n, contextual, cp2, cp1):
    ear_grow(isl)
    bits = 0.0
    p2 = cp2
    p1 = cp1
    for i in range(n):
        b = sp[i]
        if not contextual and i == 0:
            p = (counts[b] + 1.0) / (float(counts_total) + float(ACTIONS))
        elif not contextual and i == 1:
            p = (bi_count[p1][b] + 1.0) / \
                (float(bi_row[p1]) + float(ACTIONS))
        else:
            t = (p2 << 8) | p1
            p = (tri_get(t, b) + 1.0) / \
                (float(tri_row[t]) + float(ACTIONS))
        bits += -math.log2(p)
        p2 = p1
        p1 = b
    return bits

EAR_TWIN_SEED = 0x4e45545441475241

def ear_make_twin(isl):
    twin = Island()
    twin.len = isl.len
    tb = bytearray(isl.bytes)
    true_hist = [0] * ACTIONS
    for i in range(isl.len):
        true_hist[isl.bytes[i]] += 1
    s = [EAR_TWIN_SEED]
    for span in range(twin.len, 1, -1):
        i = span - 1
        j = speak_next(s) % span
        tb[i], tb[j] = tb[j], tb[i]
    twin.bytes = bytes(tb)
    changed = 0
    twin_hist = [0] * ACTIONS
    for i in range(twin.len):
        twin_hist[twin.bytes[i]] += 1
        if twin.bytes[i] != isl.bytes[i]:
            changed += 1
    for b in range(ACTIONS):
        if true_hist[b] != twin_hist[b]:
            die("netta: an island twin failed to conserve its census\n")
    twin.digest = fnv1a64(twin.bytes, FNV_SEED)
    twin.witness = 0
    twin.name = ''
    return twin, changed

def ear():
    try:
        f = open(ear_path, 'rb')
    except OSError:
        die("netta: cannot open %s\n" % ear_path)
    sp = f.read(EAR_MAX + 1)
    f.close()
    n = len(sp)
    if n > EAR_MAX:
        die("netta: the ear listens to at most %u bytes at a sitting\n"
            % EAR_MAX)
    if n < 2:
        die("netta: the ear needs at least two bytes\n")
    cp2 = 0
    cp1 = 0
    contextual = 0
    if ear_context_path:
        cp2, cp1 = question_tail(ear_context_path, "ear context",
                                 "ear context")
        contextual = 1
    maxend = [0] * EAR_MAX
    match = ear_sam_build(sp, n)
    out = sys.stdout
    for k in range(len(islands)):
        isl = islands[k]
        bits = ear_price(isl, sp, n, contextual, cp2, cp1)
        ear_sam_match(match, isl, maxend)
        longest = 0
        covered = 0
        painted = 0
        for si in range(n):
            if maxend[si] > longest:
                longest = maxend[si]
            if maxend[si] >= EAR_MATCH_MIN:
                le = si + 1 - maxend[si]
                if le < painted:
                    le = painted
                covered += si + 1 - le
                painted = si + 1
        out.write("ear %d: digest=%016x context=" % (k, isl.digest))
        if contextual:
            out.write("%02x%02x" % (cp2, cp1))
        else:
            out.write("cold")
        out.write(" bytes=%d bits=%.6f bits/byte=%.6f longest-match=%d "
                  "matched16=%.1f%%" %
                  (n, bits, bits / n, longest, 100.0 * covered / n))
        if ear_twin_requested:
            twin, changed = ear_make_twin(isl)
            twin_bits = ear_price(twin, sp, n, contextual, cp2, cp1)
            out.write(" twin-digest=%016x twin-changed=%d/%d "
                      "twin-bits=%.6f twin-bits/byte=%.6f" %
                      (twin.digest, changed, twin.len, twin_bits,
                       twin_bits / n))
        out.write("\n")

# ------------------------------------------------------------ the court

COURT_ORDER_MICRO = 500000

COURT_LAW = ("pattern-court law v2: abstain if changed=0 or gap-micro=0; "
             "replay if 2*matched-bytes>=bytes; order if "
             "gap-micro>=500000; stranger otherwise")
court_law_digest = 0

def court_docket_record(h, record):
    h = fnv1a64(record.encode('latin-1'), h)
    return fnv1a64(b'\n', h)

def llround(x):
    if x >= 0.0:
        return int(math.floor(x + 0.5))
    return int(math.ceil(x - 0.5))

def court():
    global court_law_digest
    court_law_digest = fnv1a64(COURT_LAW.encode('latin-1'), FNV_SEED)
    try:
        f = open(court_path, 'rb')
    except OSError:
        die("netta: cannot open %s\n" % court_path)
    sp = f.read(EAR_MAX + 1)
    f.close()
    n = len(sp)
    if n > EAR_MAX:
        die("netta: the court hears at most %u bytes at a sitting\n"
            % EAR_MAX)
    if n < 2:
        die("netta: the court needs at least two bytes\n")
    cp2 = 0
    cp1 = 0
    contextual = 0
    if ear_context_path:
        cp2, cp1 = question_tail(ear_context_path, "ear context",
                                 "ear context")
        contextual = 1
    ctx = "%02x%02x" % (cp2, cp1) if contextual else "cold"
    candidate_digest = fnv1a64(sp, FNV_SEED)
    out = sys.stdout
    out.write("court law: %s law-digest=%016x\n" %
              (COURT_LAW, court_law_digest))
    sitting = ("court sitting: candidate-digest=%016x bytes=%d "
               "context=%s shores=%d law-digest=%016x" %
               (candidate_digest, n, ctx, len(islands),
                court_law_digest))
    out.write("%s\n" % sitting)
    docket = court_docket_record(FNV_SEED, sitting)
    maxend = [0] * EAR_MAX
    match = ear_sam_build(sp, n)
    for k in range(len(islands)):
        isl = islands[k]
        bits = ear_price(isl, sp, n, contextual, cp2, cp1)
        ear_sam_match(match, isl, maxend)
        covered = 0
        painted = 0
        for si in range(n):
            if maxend[si] >= EAR_MATCH_MIN:
                le = si + 1 - maxend[si]
                if le < painted:
                    le = painted
                covered += si + 1 - le
                painted = si + 1
        twin, changed = ear_make_twin(isl)
        twin_bits = ear_price(twin, sp, n, contextual, cp2, cp1)
        p = bits / n
        q = twin_bits / n
        m = 100.0 * covered / n
        gap_micro = llround(1000000.0 * (twin_bits - bits) / n)
        g = gap_micro / 1000000.0
        verdict = ("abstain" if changed == 0 or gap_micro == 0
                   else "replay" if 2 * covered >= n
                   else "order" if gap_micro >= COURT_ORDER_MICRO
                   else "stranger")
        line = ("court %d: digest=%016x context=%s bytes=%d "
                "P=%.6f Q=%.6f matched16=%.4f%% "
                "matched-bytes=%d/%d G=%.6f gap-micro=%d "
                "changed=%d/%d verdict=%s" %
                (k, isl.digest, ctx, n, p, q, m, covered, n, g,
                 gap_micro, changed, isl.len, verdict))
        sealed = "%s law-digest=%016x" % (line, court_law_digest)
        receipt = fnv1a64(sealed.encode('latin-1'), FNV_SEED)
        warrant = "%s receipt=%016x" % (line, receipt)
        out.write("%s\n" % warrant)
        docket = court_docket_record(docket, warrant)
    out.write("court close: verdicts=%d docket=%016x\n" %
              (len(islands), docket))

# --------------------------------------------------------- the citation

def cite_lower_hex(s, n):
    return bio_lower_hex(s, n)

def cite_context_ok(s):
    return s == "cold" or cite_lower_hex(s, 4)

def cite_token_u64(s):
    if not s or s[0] in '-+':
        return None
    if not s.isascii() or not s.isdigit() or len(s) > 20:
        return None
    v = int(s)
    if v > MASK64:
        return None
    return v

def cite_token_i64(s):
    if not s or s[0] == '+':
        return None
    body = s[1:] if s[0] == '-' else s
    if not body or not body.isascii() or not body.isdigit() or \
            len(body) > 19:
        return None
    v = int(s)
    if not (-(1 << 63) <= v < (1 << 63)):
        return None
    return v

def _cite_parse_sitting(line, law_hex):
    # canonical: court sitting: candidate-digest=H bytes=N context=C
    #            shores=S law-digest=L
    f = line.split(' ')
    if len(f) != 7 or f[0] != 'court' or f[1] != 'sitting:':
        return None
    kv = {}
    order = []
    for tok in f[2:]:
        if '=' not in tok:
            return None
        k, v = tok.split('=', 1)
        kv[k] = v
        order.append(k)
    if order != ['candidate-digest', 'bytes', 'context', 'shores',
                 'law-digest']:
        return None
    digest = kv['candidate-digest']
    nb = cite_token_u64(kv['bytes'])
    ns = cite_token_u64(kv['shores'])
    if (not cite_lower_hex(digest, 16) or nb is None or ns is None or
            not cite_context_ok(kv['context']) or ns < 1 or
            ns > MAX_ISLANDS or nb < 2 or nb > EAR_MAX or
            not cite_lower_hex(kv['law-digest'], 16) or
            kv['law-digest'] != law_hex):
        return None
    canonical = ("court sitting: candidate-digest=%s bytes=%d context=%s "
                 "shores=%d law-digest=%s" %
                 (digest, nb, kv['context'], ns, kv['law-digest']))
    if canonical != line:
        return None
    return digest, nb, kv['context'], ns

def _cite_parse_court(line, law_hex, expected_id, sitting_bytes,
                      sitting_context):
    f = line.split(' ')
    if len(f) != 14 or f[0] != 'court' or not f[1].endswith(':'):
        return 0
    id_token = f[1][:-1]
    kv = []
    for tok in f[2:]:
        if '=' not in tok:
            return 0
        kv.append(tok.split('=', 1))
    keys = [k for k, _ in kv]
    if keys != ['digest', 'context', 'bytes', 'P', 'Q', 'matched16',
                'matched-bytes', 'G', 'gap-micro', 'changed', 'verdict',
                'receipt']:
        return 0
    vals = dict(kv)
    id64 = cite_token_u64(id_token)
    bts = cite_token_u64(vals['bytes'])
    m16 = vals['matched16']
    if not m16.endswith('%'):
        return 0
    m16 = m16[:-1]
    mb = vals['matched-bytes'].split('/')
    ch = vals['changed'].split('/')
    if len(mb) != 2 or len(ch) != 2:
        return 0
    covered = cite_token_u64(mb[0])
    denominator = cite_token_u64(mb[1])
    changed = cite_token_u64(ch[0])
    shore = cite_token_u64(ch[1])
    gap_micro = cite_token_i64(vals['gap-micro'])

    def tok_double(s):
        if not s or s[0] == '+':
            return None
        try:
            v = float(s)
        except ValueError:
            return None
        if not math.isfinite(v):
            return None
        return v

    p = tok_double(vals['P'])
    q = tok_double(vals['Q'])
    matched = tok_double(m16)
    gap = tok_double(vals['G'])
    verdict = vals['verdict']
    receipt = vals['receipt']
    if (id64 is None or id64 > 2147483647 or bts is None or p is None or
            q is None or matched is None or covered is None or
            denominator is None or gap is None or gap_micro is None or
            changed is None or shore is None or id64 != expected_id or
            not cite_lower_hex(vals['digest'], 16) or
            not cite_context_ok(vals['context']) or
            vals['context'] != sitting_context or
            bts != sitting_bytes or denominator != sitting_bytes or
            denominator == 0 or covered > denominator or
            changed > shore or p < 0.0 or q < 0.0 or matched < 0.0 or
            math.copysign(1.0, p) < 0 or math.copysign(1.0, q) < 0 or
            math.copysign(1.0, matched) < 0 or
            (gap_micro == 0 and math.copysign(1.0, gap) < 0) or
            not cite_lower_hex(receipt, 16)):
        return 0

    base = ("court %d: digest=%s context=%s bytes=%d "
            "P=%.6f Q=%.6f matched16=%.4f%% matched-bytes=%d/%d "
            "G=%.6f gap-micro=%d changed=%d/%d verdict=%s" %
            (id64, vals['digest'], vals['context'], bts, p, q, matched,
             covered, denominator, gap, gap_micro, changed, shore,
             verdict))
    canonical = "%s receipt=%s" % (base, receipt)
    if canonical != line:
        return 0
    if ("%.4f" % matched) != ("%.4f" % (100.0 * covered / denominator)):
        return 0
    if ("%.6f" % gap) != ("%.6f" % (gap_micro / 1000000.0)):
        return 0
    if abs((q - p) - gap) > 0.000002:
        return 0
    want = ("abstain" if changed == 0 or gap_micro == 0
            else "replay" if covered >= (denominator + 1) // 2
            else "order" if gap_micro >= COURT_ORDER_MICRO
            else "stranger")
    if verdict != want:
        return 0
    sealed = "%s law-digest=%s" % (base, law_hex)
    mine = "%016x" % fnv1a64(sealed.encode('latin-1'), FNV_SEED)
    return 1 if receipt == mine else 0

def cite_verify():
    global court_law_digest
    court_law_digest = fnv1a64(COURT_LAW.encode('latin-1'), FNV_SEED)
    try:
        f = open(cite_path, 'rb')
    except OSError:
        die("netta: cannot open %s\n" % cite_path)
    data = f.read()
    f.close()
    law_hex = "%016x" % court_law_digest
    stage = 0
    leaf = 0
    ns = 0
    docket = 0
    nb = 0
    cand_hex = ''
    ctx = ''
    pos = 0
    total = len(data)
    while pos < total:
        nl = data.find(b'\n', pos)
        if nl < 0:
            die("netta: the docket record is unsealed\n")
        raw = data[pos:nl]
        pos = nl + 1
        if b'\x00' in raw or len(raw) >= 1023:
            # C fgets + strlen: a NUL truncates the record and an overlong
            # record splits before its newline; both fail the seal
            die("netta: the docket record is unsealed\n")
        line = raw.decode('latin-1')
        if line.endswith('\r'):
            line = line[:-1]
        if stage == 0:
            if line != "NETTA ZERO":
                die("netta: the docket does not open as NETTA ZERO\n")
            stage = 1
        elif stage == 1:
            expect = "court law: %s law-digest=%016x" % (
                COURT_LAW, court_law_digest)
            if line != expect:
                die("netta: the docket's law is not this life's law\n")
            stage = 2
        elif stage == 2:
            got = _cite_parse_sitting(line, law_hex)
            if got is None:
                die("netta: the docket sitting is not canonical\n")
            cand_hex, nb, ctx, ns = got
            docket = court_docket_record(FNV_SEED, line)
            stage = 3
        elif stage == 3 and line.startswith("court close: "):
            if leaf != ns:
                die("netta: the docket closed before its verdicts\n")
            expect = "court close: verdicts=%d docket=%016x" % (ns, docket)
            if line != expect:
                die("netta: the docket close does not match its fold\n")
            stage = 4
        elif stage == 3:
            if not _cite_parse_court(line, law_hex, leaf, nb, ctx):
                die("netta: a docket court record is not canonical\n")
            docket = court_docket_record(docket, line)
            leaf += 1
        else:
            die("netta: the citation takes one sitting at a time\n")
    if stage != 4:
        die("netta: the docket is incomplete\n")
    return int(cand_hex, 16), nb, ctx, ns, docket

def main(argv):
    global fixed_start, fixed_start_set, life_control_named
    global instrument_only_named, speak_requested, speak_bytes
    global speak_seed, speak_seed_set, prompt_path, speak_laplace
    global sign_requested, ear_path, ear_context_path, ear_twin_requested
    global court_path, cite_path, actor_lock, atlas_enabled
    global units_enabled, move_nav_enabled, island_court_enabled
    global birth_floor_enabled, local_probation_enabled
    global unit_death_enabled, tombstone_silence_enabled
    global core_enabled, core_hebb_enabled, jury_enabled
    global rng_state, pair_key, pair_cnt, tri_key, tri_cnt, actor_lock
    global episode_no, bio_lines, bio_chain
    state_path = "netta0.state"
    bio_path = "netta0.bio.tsv"
    seed = 1
    episodes = 1
    steps = 64
    isl_id = 0
    reset = 0
    paths = []

    i = 1
    argc = len(argv)
    while i < argc:
        a = argv[i]
        if a == "--seed" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            seed = parse_u64("--seed", argv[i])
        elif a == "--episodes" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            episodes = parse_u64("--episodes", argv[i])
        elif a == "--steps" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            steps = parse_u64("--steps", argv[i])
        elif a == "--island" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            isl_id = parse_int("--island", argv[i])
        elif a == "--start" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            fixed_start = parse_u64("--start", argv[i])
            fixed_start_set = 1
        elif a == "--state" and i + 1 < argc:
            life_control_named = 1
            i += 1
            state_path = argv[i]
        elif a == "--bio" and i + 1 < argc:
            life_control_named = 1
            i += 1
            bio_path = argv[i]
        elif a == "--reset":
            life_control_named = 1
            instrument_only_named = 1
            reset = 1
        elif a == "--atlas":
            life_control_named = 1
            instrument_only_named = 1
            atlas_enabled = 1
        elif a == "--no-units":
            life_control_named = 1
            instrument_only_named = 1
            units_enabled = 0
        elif a == "--no-mv-nav":
            life_control_named = 1
            instrument_only_named = 1
            move_nav_enabled = 0
        elif a == "--no-island-court":
            life_control_named = 1
            instrument_only_named = 1
            island_court_enabled = 0
        elif a == "--no-birth-floor":
            life_control_named = 1
            instrument_only_named = 1
            birth_floor_enabled = 0
        elif a == "--no-local-probation":
            life_control_named = 1
            instrument_only_named = 1
            local_probation_enabled = 0
        elif a == "--no-unit-death":
            life_control_named = 1
            instrument_only_named = 1
            unit_death_enabled = 0
        elif a == "--keep-dead-mass":
            life_control_named = 1
            instrument_only_named = 1
            tombstone_silence_enabled = 0
        elif a == "--no-core":
            life_control_named = 1
            instrument_only_named = 1
            core_enabled = 0
        elif a == "--core-hebb-v1":
            life_control_named = 1
            instrument_only_named = 1
            core_hebb_enabled = 1
        elif a == "--jury":
            life_control_named = 1
            instrument_only_named = 1
            jury_enabled = 1
        elif a == "--speak" and i + 1 < argc:
            speak_requested = 1
            i += 1
            speak_bytes = parse_u64("--speak", argv[i])
        elif a == "--speak-seed" and i + 1 < argc:
            speak_seed_set = 1
            i += 1
            speak_seed = parse_u64("--speak-seed", argv[i])
        elif a == "--prompt-file" and i + 1 < argc:
            i += 1
            prompt_path = argv[i]
        elif a == "--speak-laplace":
            speak_laplace = 1
        elif a == "--sign":
            instrument_only_named = 1
            sign_requested = 1
        elif a == "--ear" and i + 1 < argc:
            i += 1
            ear_path = argv[i]
        elif a == "--ear-context" and i + 1 < argc:
            i += 1
            ear_context_path = argv[i]
        elif a == "--ear-twin":
            ear_twin_requested = 1
        elif a == "--court" and i + 1 < argc:
            i += 1
            court_path = argv[i]
        elif a == "--cite" and i + 1 < argc:
            i += 1
            cite_path = argv[i]
        elif a == "--actor-lock" and i + 1 < argc:
            life_control_named = 1
            instrument_only_named = 1
            i += 1
            lock = argv[i]
            if lock == "uni":
                actor_lock = 0
            elif lock == "bi":
                actor_lock = 1
            elif lock == "tri":
                actor_lock = 2
            elif lock == "mv":
                actor_lock = 3
            else:
                die("netta: --actor-lock takes uni|bi|tri|mv\n")
        elif a.startswith("-"):
            die("netta: unknown flag %s\n" % a)
        elif len(paths) < MAX_ISLANDS:
            paths.append(a)
        i += 1

    if ear_context_path and not ear_path and not court_path:
        die("netta: --ear-context requires --ear or --court\n")
    if ear_twin_requested and not ear_path:
        die("netta: --ear-twin requires --ear\n")
    if court_path:
        if (ear_path or ear_twin_requested or speak_requested or
                sign_requested or speak_seed_set or prompt_path or
                speak_laplace or life_control_named):
            die("netta: the court accepts only shores, --court, and "
                "--ear-context\n")
        if not paths:
            die("netta: the court needs a shore\n")
    if cite_path:
        if (speak_requested or sign_requested or speak_seed_set or
                prompt_path or speak_laplace or ear_path or
                ear_context_path or ear_twin_requested or court_path):
            die("netta: the citation and the instruments are "
                "separate invocations\n")
        if paths:
            die("netta: the citation is between the life and the "
                "record\n")
        if instrument_only_named:
            die("netta: the citation accepts only --cite, --state, "
                "and --bio\n")
    if ear_path:
        if (speak_requested or sign_requested or speak_seed_set or
                prompt_path or speak_laplace or life_control_named):
            die("netta: the ear accepts only shores, --ear, and "
                "--ear-context/--ear-twin\n")
        if not paths:
            die("netta: the ear needs a shore\n")
    if actor_lock == 3 and not units_enabled:
        die("netta: --actor-lock mv requires units enabled\n")
    if not speak_requested and (speak_seed_set or prompt_path or
                                speak_laplace or sign_requested):
        die("netta: speech flags require --speak\n")
    if speak_requested:
        voice_protected = fd_is_file(2, bio_path) or \
            fd_is_file(2, state_path)
        for p in paths:
            if fd_is_file(2, p):
                voice_protected = 1
        if voice_protected or not fd_writable(2):
            sys.exit(1)
        if not fd_writable(1):
            die("netta: the mouth needs an open stream and voice, both "
                "writable\n")
        stream_protected = fd_is_file(1, bio_path) or \
            fd_is_file(1, state_path)
        for p in paths:
            if fd_is_file(1, p):
                stream_protected = 1
        if stream_protected:
            die("netta: the mouth cannot speak into its own memory or "
                "a shore\n")
        if fds_share_framed_sink(1, 2):
            die("netta: the mouth needs separate stream and voice "
                "sinks\n")
    if speak_requested:
        sys.stderr.write("NETTA ZERO\n")
    else:
        sys.stdout.write("NETTA ZERO\n")
    if not paths and not speak_requested and not cite_path:
        die("usage: netta <island.bytes>... [--seed N] "
            "[--episodes N] [--steps N] [--island N] "
            "[--start OFFSET] [--state P] [--bio P] [--reset] "
            "[--atlas] [--no-core] [--core-hebb-v1] [--jury] "
            "[--speak N] [--speak-seed N] [--prompt-file P] "
            "[--speak-laplace] [--sign] [--ear P] "
            "[--ear-context P] "
            "[--ear-twin] [--court P] [--cite P] "
            "[--no-units] [--no-mv-nav] [--no-island-court] "
            "[--no-birth-floor] [--no-local-probation] "
            "[--no-unit-death] [--keep-dead-mass] "
            "[--actor-lock uni|bi|tri|mv]\n")
    pair_key = [0] * PAIR_SLOTS
    pair_cnt = [0] * PAIR_SLOTS
    tri_key = [0] * TRI_SLOTS
    tri_cnt = [0] * TRI_SLOTS
    for p in paths:
        island_load(p)
    if court_path:
        court()
        return 0
    if ear_path:
        ear()
        return 0
    if same_file(state_path, bio_path):
        die("netta: state and biography must be distinct\n")
    for p in paths:
        if same_file(p, state_path) or same_file(p, bio_path):
            die("netta: an immutable island cannot also be state or "
                "biography\n")
    dest = sys.stderr if speak_requested else sys.stdout
    for k in range(len(islands)):
        dest.write("island %d: %s len=%d digest=%016x\n" %
                   (k, islands[k].name, islands[k].len,
                    islands[k].digest))
    if paths and (isl_id < 0 or isl_id >= len(islands)):
        die("netta: no island %d\n" % isl_id)
    rng_state = seed
    resumed = 0
    core_init()
    jury_init()
    if not reset:
        resumed = state_load(state_path)
    if resumed:
        bio_verify(bio_path)
    if not reset and not resumed and file_exists(bio_path):
        die("netta: biography %s exists without state; use --reset "
            "to begin a new life\n" % bio_path)
    registry_resolve_convoy()
    if speak_requested:
        if not resumed:
            die("netta: the mouth needs a lived state\n")
        if not atomic_bytes_lived or not counts_total:
            die("netta: the mouth needs lived bytes\n")
        if arrivals_pending:
            die("netta: the mouth cannot meet new islands\n")
        if sign_requested and not speak_bytes:
            die("netta: the signature needs spoken bytes\n")
        speak()
        if sign_requested:
            bio_open(bio_path, 0)
            bio_append("s\t%d\t%016x\t%d\t%d\t%s\t%s\t%02x%02x\t%d\n" %
                       (episode_no, spoke_digest, speak_bytes,
                        speak_seed, actor_name[spoke_hand],
                        "laplace-red" if speak_laplace
                        else "supported-backoff",
                        spoke_open2, spoke_open1, atomic_bytes_lived))
            try:
                bio_file.flush()
                bio_file.close()
            except OSError:
                die("netta: biography close failed\n")
            state_save(state_path)
            sys.stderr.write("signed: candidate %016x as biography line "
                             "%d, chain %016x\n" %
                             (spoke_digest, bio_lines, bio_chain))
        return 0
    if cite_path:
        if not resumed:
            die("netta: the citation needs a lived state\n")
        cite_cand, cite_bytes, cite_ctx, cite_shores, cite_docket = \
            cite_verify()
    bio_open(bio_path, reset or not resumed)
    if cite_path:
        bio_append("w\t%d\t%016x\t%d\t%s\t%d\t%016x\t%016x\t%d\n" %
                   (episode_no, cite_cand, cite_bytes, cite_ctx,
                    cite_shores, court_law_digest, cite_docket,
                    cite_shores))
        try:
            bio_file.flush()
            bio_file.close()
        except OSError:
            die("netta: biography close failed\n")
        state_save(state_path)
        sys.stdout.write("cited: docket %016x, %d verdicts on candidate "
                         "%016x\n" %
                         (cite_docket, cite_shores, cite_cand))
        return 0
    for r in arrivals_pending:
        bio_append("i\t%d\t%d\t%016x\t%016x\t%d\n" %
                   (episode_no, r, reg_digest[r], reg_witness[r],
                    reg_len[r]))
        sys.stdout.write("island arrival: registry %d digest %016x "
                         "witness %016x len %d\n" %
                         (r, reg_digest[r], reg_witness[r], reg_len[r]))
    tl_ab = atomic_bits_lived
    tl_bb = bilm_bits
    tl_tb = trilm_bits
    tl_ub = unitlm_bits
    tl_mb = mvlm_bits
    tl_cb = core_bits
    tl_aby = atomic_bytes_lived
    tl_bby = bilm_bytes
    tl_tby = trilm_bytes
    tl_uby = unitlm_bytes
    tl_mby = mvlm_bytes
    tl_cby = core_bytes
    tl_j = []
    for c in jury_core:
        tl_j.append((c.bits, c.bytes, c.health_bytes, c.near_sat,
                     c.clamp_hits, c.gates_pos, c.gates_neg,
                     c.row_abs_sum[:]))
    sys.stdout.write("%s: episode %d, %d lived bytes, %d units\n" %
                     ("resumed" if resumed else "born", episode_no,
                      counts_total, len(units)))

    sum_bits = 0.0
    played_isl_id = isl_id
    for _ in range(episodes):
        played_isl_id = atlas_choose(isl_id, steps) if atlas_enabled \
            else isl_id
        sum_bits += run_episode(played_isl_id, steps)
    try:
        bio_file.flush()
        bio_file.close()
    except OSError:
        die("netta: biography close failed\n")
    state_save(state_path)

    out = sys.stdout
    if episodes:
        out.write("bits per raw byte: %.6f\n" % (sum_bits / episodes))
    if units_enabled and steps_total:
        decisions = steps_total - macro_bytes + macro_events
        out.write("units: %d living of %d born, %d macro events over "
                  "%d bytes, decisions per lived byte %.4f\n" %
                  (unit_living, len(units), macro_events, macro_bytes,
                   decisions / steps_total))
        if unit_deaths:
            out.write("units: %d deaths this run\n" % unit_deaths)
        if len(units) != unit_living:
            ghost_pairs = 0
            live_pairs = 0
            for m in range(ACTIONS + len(units)):
                ghost_pairs += mv_out[m]
                live_pairs += mv_live_out[m]
            out.write("tombstones: %d unigram events and %d transition "
                      "events excluded from living probability\n" %
                      (move_total - move_live_total,
                       ghost_pairs - live_pairs))
        if births_rejected_cap:
            out.write("units: %d births rejected at cap %d\n" %
                      (births_rejected_cap, MAX_UNITS))
        if unitlm_bytes and atomic_bytes_lived:
            out.write("unit-LM bits per raw byte: %.6f (atomic %.6f, "
                      "lived %d bytes)\n" %
                      (unitlm_bits / unitlm_bytes,
                       atomic_bits_lived / atomic_bytes_lived,
                       unitlm_bytes))
    if atomic_bytes_lived:
        out.write("model atomic-uni bits/byte %.6f\n" %
                  (atomic_bits_lived / atomic_bytes_lived))
    if units_enabled and unitlm_bytes:
        out.write("model unit-uni bits/byte %.6f\n" %
                  (unitlm_bits / unitlm_bytes))
    if bilm_bytes:
        out.write("model byte-bi bits/byte %.6f\n" %
                  (bilm_bits / bilm_bytes))
    if units_enabled and mvlm_bytes:
        out.write("model move-bi bits/byte %.6f\n" %
                  (mvlm_bits / mvlm_bytes))
    if units_enabled and isl_mvlm_bytes[present_reg[played_isl_id]]:
        pr = present_reg[played_isl_id]
        out.write("island %d shadow move-bi %.6f, atomic ref %.6f over "
                  "%d bytes\n" %
                  (pr, isl_mvlm_bits[pr] / isl_mvlm_bytes[pr],
                   isl_mvlm_ref_bits[pr] / isl_mvlm_bytes[pr],
                   isl_mvlm_bytes[pr]))
    if trilm_bytes:
        out.write("model byte-tri bits/byte %.6f\n" %
                  (trilm_bits / trilm_bytes))
    if core_enabled and core_bytes:
        degen = 0
        for b in range(ACTIONS):
            nrm = 0.0
            for d in range(CORE_EMBED):
                nrm = fmaf(core_E[b][d], core_E[b][d], nrm)
            if nrm < 1e-6:
                degen += 1
        out.write("model core bits/byte %.6f\n" %
                  (core_bits / core_bytes))
        out.write("core health: mean |h| %.4f, %d degenerate "
                  "embeddings\n" %
                  (core_sat_sum / core_sat_n if core_sat_n else 0.0,
                   degen))
        out.write("core plasticity: v1 gates +%d -%d (%s)\n" %
                  (core_hebb_pos, core_hebb_neg,
                   "active" if core_hebb_enabled else "quarantined"))
    if jury_enabled:
        for g in range(JURY_GENOMES):
            c = jury_core[g]
            row_max = 0.0
            for j in range(CORE_HIDDEN):
                mean = c.row_abs_sum[j] / c.health_bytes \
                    if c.health_bytes else 0.0
                if mean > row_max:
                    row_max = mean
            near = (c.near_sat / (c.health_bytes * CORE_HIDDEN)) \
                if c.health_bytes else 0.0
            out.write("jury genome %d bits/byte %.6f max-row-abs-h %.6f "
                      "near-sat %.9f clamps %d gates +%d -%d\n" %
                      (g, c.bits / c.bytes if c.bytes else 0.0, row_max,
                       near, c.clamp_hits, c.gates_pos, c.gates_neg))
    if mvp_bytes:
        out.write("mv played record: %.6f bits/byte over %d bytes\n" %
                  (mvp_bits / mvp_bytes, mvp_bytes))
    if mvp_ref_bytes:
        out.write("mv matched byte refs: uni %.6f, bi %.6f, tri %.6f "
                  "over %d bytes\n" %
                  (mvp_ref_bits[0] / mvp_ref_bytes,
                   mvp_ref_bits[1] / mvp_ref_bytes,
                   mvp_ref_bits[2] / mvp_ref_bytes, mvp_ref_bytes))
    if mvc_bytes:
        out.write("mv control record: %.6f bits/byte over %d bytes; "
                  "matched refs uni %.6f, bi %.6f, tri %.6f\n" %
                  (mvc_bits / mvc_bytes, mvc_bytes,
                   mvc_ref_bits[0] / mvc_bytes,
                   mvc_ref_bits[1] / mvc_bytes,
                   mvc_ref_bits[2] / mvc_bytes))
    if move_nav_steps:
        out.write("mv navigation: %d searched decisions, %d unit "
                  "anchors\n" % (move_nav_steps, move_nav_unit_anchors))
    out.write("actor episodes: uni %d, bi %d, tri %d, mv %d, "
              "null %d%s\n" %
              (ep_actor[0], ep_actor[1], ep_actor[2], ep_actor[3],
               ep_actor[4], " (locked)" if actor_lock >= 0 else ""))
    if revocations:
        out.write("island court: %d local revocations this run\n" %
                  revocations)
    if null_refusals:
        out.write("island birth floor: %d null refusals this run\n" %
                  null_refusals)
    if atlas_decisions:
        out.write("atlas: %d autonomous choices this run\n" %
                  atlas_decisions)
    if atomic_bytes_lived > tl_aby:
        out.write("this-life model atomic-uni bits/byte %.6f\n" %
                  ((atomic_bits_lived - tl_ab) /
                   (atomic_bytes_lived - tl_aby)))
    if bilm_bytes > tl_bby:
        out.write("this-life model byte-bi bits/byte %.6f\n" %
                  ((bilm_bits - tl_bb) / (bilm_bytes - tl_bby)))
    if trilm_bytes > tl_tby:
        out.write("this-life model byte-tri bits/byte %.6f\n" %
                  ((trilm_bits - tl_tb) / (trilm_bytes - tl_tby)))
    if core_enabled and core_bytes > tl_cby:
        out.write("this-life model core bits/byte %.6f\n" %
                  ((core_bits - tl_cb) / (core_bytes - tl_cby)))
    if jury_enabled:
        for g in range(JURY_GENOMES):
            c = jury_core[g]
            (jb, jby, jhealth, jnear, jclamps, jpos, jneg, jrow) = tl_j[g]
            lived = c.health_bytes - jhealth
            row_max = 0.0
            for j in range(CORE_HIDDEN):
                mean = ((c.row_abs_sum[j] - jrow[j]) / lived) \
                    if lived else 0.0
                if mean > row_max:
                    row_max = mean
            priced = c.bytes - jby
            near = ((c.near_sat - jnear) / (lived * CORE_HIDDEN)) \
                if lived else 0.0
            out.write("this-life jury genome %d bits/byte %.6f "
                      "max-row-abs-h %.6f near-sat %.9f clamps %d "
                      "gates +%d -%d\n" %
                      (g, (c.bits - jb) / priced if priced else 0.0,
                       row_max, near, c.clamp_hits - jclamps,
                       c.gates_pos - jpos, c.gates_neg - jneg))
    if units_enabled and unitlm_bytes > tl_uby:
        out.write("this-life model unit-uni bits/byte %.6f\n" %
                  ((unitlm_bits - tl_ub) / (unitlm_bytes - tl_uby)))
    if units_enabled and mvlm_bytes > tl_mby:
        out.write("this-life model move-bi bits/byte %.6f\n" %
                  ((mvlm_bits - tl_mb) / (mvlm_bytes - tl_mby)))
    for k in range(len(islands)):
        rec = 0
        w = islands[k]
        for u in units:
            if u.dead:
                continue
            L = u.len
            found = 0
            if w.len >= L and u.bytes[:L] in w.bytes:
                found = 1
            rec += found
        out.write("units recognisable on island %d: %d of %d\n" %
                  (k, rec, unit_living))
    out.write("biography: %d lines, chain %016x\n" %
              (bio_lines, bio_chain))
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
