#!/usr/bin/env python3
"""Independent row-level verifier for root_court output."""

import math
import re
import sys
from collections import defaultdict
from pathlib import Path


FNV_SEED = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
LAWS = ("L-byte", "L-u8b")
GOLD = (
    ("שלח", ("שלח", "שליח", "שליחות", "משלוח")),
    ("כתב", ("כתב", "כתיבה", "מכתב", "כתובת")),
    ("ספר", ("ספר", "סיפור", "ספרות", "מספר", "ספריה")),
    ("למד", ("למד", "לימוד", "תלמיד", "תלמוד", "מלמד")),
    ("בנה", ("בנה", "בניין", "מבנה", "בנאי", "תבנית")),
    ("שמר", ("שמר", "שמירה", "משמר", "משמרת", "שמורה")),
    ("פתח", ("פתח", "פתיחה", "מפתח", "פתיחות")),
    ("חבר", ("חבר", "חיבור", "מחברת", "חברה", "חבורה")),
)
FORMS = tuple(word for _, words in GOLD for word in words)
FAMILY = {word: root for root, words in GOLD for word in words}

SUMMARY_RE = re.compile(
    r"^\[(L-byte|L-u8b)\] top1=([^ ]+) MAP=([^ ]+) "
    r"within-cos=([^ ]+) between-cos=([^ ]+) TPR=([^ ]+) "
    r"FPR=([^ ]+) balanced=([^ ]+)$"
)
CORPUS_RE = re.compile(
    r"^corpus .+: 71280 bytes digest 05d2840d282a7f14$"
)
ROWS_RE = re.compile(r"^rows: 2664 sealed, digest ([0-9a-f]{16})$")
RULE_RE = re.compile(
    r"^rule: MAP\(u8b\)-MAP\(byte\) = ([^ ]+) threshold 0\.10$"
)


def refuse(message):
    raise SystemExit(f"root_court_check: {message}")


def parse_float(text, where):
    try:
        value = float(text)
    except ValueError:
        refuse(f"{where}: malformed float")
    if not math.isfinite(value):
        refuse(f"{where}: non-finite float")
    if format(value, ".17g") != text:
        refuse(f"{where}: noncanonical float {text!r}")
    return value


def fnv64(data):
    value = FNV_SEED
    for byte in data:
        value ^= byte
        value = (value * FNV_PRIME) & 0xFFFFFFFFFFFFFFFF
    return value


def close(actual, expected, name):
    if actual != expected:
        refuse(f"{name}: recomputed {actual:.17g}, report says {expected:.17g}")


def verify(path):
    try:
        raw = Path(path).read_bytes()
    except OSError as error:
        refuse(f"cannot read report: {error}")
    if not raw.endswith(b"\n"):
        refuse("report is not newline sealed")

    rows = defaultdict(list)
    row_bytes = bytearray()
    row_sequence = []
    record_order = []
    summaries = {}
    seen_header = seen_corpus = seen_declarations = False
    reported_row_digest = None
    reported_delta = None
    reported_verdict = None

    for number, line_bytes in enumerate(raw[:-1].split(b"\n"), 1):
        binary_line = line_bytes + b"\n"
        try:
            line = line_bytes.decode("utf-8")
        except UnicodeDecodeError:
            refuse(f"line {number}: invalid UTF-8")

        if line == "the root court: subword inheritance (preregistered in ROOT_COURT.md)":
            if seen_header:
                refuse("duplicate court header")
            seen_header = True
            record_order.append("header")
            continue
        if CORPUS_RE.match(line):
            if seen_corpus:
                refuse("duplicate corpus receipt")
            seen_corpus = True
            record_order.append("corpus")
            continue
        if line == (
            "declarations: 8 lines once each; forms 37 distinct; "
            "pairs within=68 between=598"
        ):
            if seen_declarations:
                refuse("duplicate declaration receipt")
            seen_declarations = True
            record_order.append("declarations")
            continue

        match = SUMMARY_RE.match(line)
        if match:
            law = match.group(1)
            if law in summaries:
                refuse(f"duplicate summary for {law}")
            summaries[law] = tuple(
                parse_float(text, f"{law} summary") for text in match.groups()[1:]
            )
            record_order.append(f"summary:{law}")
            continue

        if line.startswith("L-byte\t") or line.startswith("L-u8b\t"):
            fields = line.split("\t")
            if len(fields) != 6:
                refuse(f"line {number}: row arity is not six")
            law, query, rank_text, candidate, score_text, same_text = fields
            try:
                rank = int(rank_text)
            except ValueError:
                refuse(f"line {number}: malformed rank")
            if str(rank) != rank_text:
                refuse(f"line {number}: noncanonical rank")
            if same_text not in ("0", "1"):
                refuse(f"line {number}: same-root is not 0 or 1")
            score = parse_float(score_text, f"line {number} score")
            rows[(law, query)].append((rank, candidate, score, int(same_text)))
            row_sequence.append((law, query, rank))
            record_order.append("row")
            row_bytes.extend(binary_line)
            continue

        match = ROWS_RE.match(line)
        if match:
            if reported_row_digest is not None:
                refuse("duplicate row receipt")
            reported_row_digest = int(match.group(1), 16)
            record_order.append("row-receipt")
            continue
        match = RULE_RE.match(line)
        if match:
            if reported_delta is not None:
                refuse("duplicate rule row")
            reported_delta = parse_float(match.group(1), "rule delta")
            record_order.append("rule")
            continue
        if line.startswith("VERDICT: "):
            if reported_verdict is not None:
                refuse("duplicate verdict")
            reported_verdict = line
            record_order.append("verdict")
            continue
        refuse(f"line {number}: unknown record")

    if not seen_header:
        refuse("missing court header")
    if not seen_corpus or not seen_declarations:
        refuse("missing pinned-corpus receipt")
    if set(summaries) != set(LAWS):
        refuse("missing arm summary")
    if reported_row_digest is None or reported_delta is None or reported_verdict is None:
        refuse("incomplete closing receipt")

    expected_keys = [
        (law, query, rank)
        for law in LAWS
        for query in FORMS
        for rank in range(1, 37)
    ]
    if row_sequence != expected_keys or sum(map(len, rows.values())) != 2664:
        refuse("row coverage or arm/query/rank order differs from the contract")
    if set(rows) != {(law, query) for law in LAWS for query in FORMS}:
        refuse("foreign arm or query")
    expected_order = [
        "header",
        "corpus",
        "declarations",
        "summary:L-byte",
        "summary:L-u8b",
        *(["row"] * 2664),
        "row-receipt",
        "rule",
        "verdict",
    ]
    if record_order != expected_order:
        refuse("report records are not in canonical order")

    row_digest = fnv64(row_bytes)
    if row_digest != reported_row_digest:
        refuse(
            f"row digest {row_digest:016x} differs from receipt "
            f"{reported_row_digest:016x}"
        )

    score_by_pair = {}
    maps = {}
    for law in LAWS:
        top1 = 0
        ap_sum = 0.0
        for query in FORMS:
            ranked = rows[(law, query)]
            candidates = [candidate for _, candidate, _, _ in ranked]
            if len(set(candidates)) != 36 or set(candidates) != set(FORMS) - {query}:
                refuse(f"{law}/{query}: candidate coverage differs from the gold")
            relevant = sum(
                candidate != query and FAMILY[candidate] == FAMILY[query]
                for candidate in FORMS
            )
            rel_seen = 0
            ap = 0.0
            previous = None
            for rank, candidate, score, marked_same in ranked:
                same = int(FAMILY[candidate] == FAMILY[query])
                if marked_same != same:
                    refuse(f"{law}/{query}/{candidate}: false family label")
                if previous is not None:
                    prev_score, prev_candidate = previous
                    ordered = prev_score > score or (
                        prev_score == score
                        and prev_candidate.encode("utf-8") < candidate.encode("utf-8")
                    )
                    if not ordered:
                        refuse(f"{law}/{query}: score or tie order broke at rank {rank}")
                previous = (score, candidate)
                score_by_pair[(law, query, candidate)] = score
                if same:
                    rel_seen += 1
                    ap += rel_seen / rank
            if rel_seen != relevant:
                refuse(f"{law}/{query}: relevant count drifted")
            top1 += ranked[0][3]
            ap_sum += ap / relevant

        within_sum = between_sum = 0.0
        within_n = between_n = 0
        tp = fp = fn = tn = 0
        for left_index, left in enumerate(FORMS):
            for right in FORMS[left_index + 1 :]:
                score = score_by_pair[(law, left, right)]
                reverse = score_by_pair[(law, right, left)]
                if score != reverse:
                    refuse(f"{law}/{left}/{right}: asymmetric cosine rows")
                same = FAMILY[left] == FAMILY[right]
                if same:
                    within_sum += score
                    within_n += 1
                    if score >= 0.5:
                        tp += 1
                    else:
                        fn += 1
                else:
                    between_sum += score
                    between_n += 1
                    if score >= 0.5:
                        fp += 1
                    else:
                        tn += 1

        root_map = ap_sum / len(FORMS)
        maps[law] = root_map
        tpr = tp / (tp + fn)
        fpr = fp / (fp + tn)
        recomputed = (
            top1 / len(FORMS),
            root_map,
            within_sum / within_n,
            between_sum / between_n,
            tpr,
            fpr,
            0.5 * (tpr + 1.0 - fpr),
        )
        for name, actual, reported in zip(
            ("top1", "MAP", "within", "between", "TPR", "FPR", "balanced"),
            recomputed,
            summaries[law],
        ):
            close(actual, reported, f"{law} {name}")

    delta = maps["L-u8b"] - maps["L-byte"]
    close(delta, reported_delta, "rule delta")
    loses = delta >= 0.10
    expected_verdict = (
        "VERDICT: the byte law LOSES subword inheritance; body 1 bumps "
        "to body1-utf8-or-byte-v2 and every prior body is remeasured"
        if loses
        else "VERDICT: the byte law holds subword inheritance within the "
        "sealed margin; the parliament may open on L-byte with both cone "
        "limits recorded"
    )
    if reported_verdict != expected_verdict:
        refuse("printed verdict does not follow from independently recomputed MAP")

    print(
        f"root court verified: rows=2664 digest={row_digest:016x} "
        f"MAP-byte={maps['L-byte']:.17g} MAP-u8b={maps['L-u8b']:.17g} "
        f"delta={delta:.17g} verdict={'LOSES' if loses else 'HOLDS'}"
    )


def main(argv):
    if len(argv) != 2:
        refuse("usage: root_court_check.py <root-court-output>")
    verify(argv[1])


if __name__ == "__main__":
    main(sys.argv)
