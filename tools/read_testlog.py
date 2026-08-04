#!/usr/bin/env python3
"""Read and slice the Alonecraft in-game test trace.

The trace is written by src/AlonecraftTestLog.cpp to
<LogsDir>/alonecraft_test.log.  Every traced line looks like:

    2026-08-03 14:22:31 +012345 | [WD-09] WARL.RUIN crit source=... spread=2

and session markers look like:

    2026-08-03 14:22:28 +012340 | ======== BEGIN case=WD-09 char=Bob ========

Usage:
    python read_testlog.py --list                    # which cases were run
    python read_testlog.py --case WD-09              # everything for one case
    python read_testlog.py --case WD-09 --no-state   # hide the STATE.* dump
    python read_testlog.py --tag WARL.SHARD          # one tag across all cases
    python read_testlog.py --rolls                   # PASS/fail rates per tag
    python read_testlog.py --aura-diff WD-09         # before/after aura delta
"""

import argparse
import os
import re
import sys
from collections import Counter, defaultdict

LINE_RE = re.compile(
    r"^(?P<wall>\d{4}-\d\d-\d\d \d\d:\d\d:\d\d) \+(?P<ms>\d+) \| "
    r"(?:\[(?P<case>[^\]]*)\] (?P<tag>\S+) (?P<msg>.*)|(?P<marker>[=\-].*))$"
)

BEGIN_RE = re.compile(r"=+ BEGIN case=(?P<case>\S+) char=(?P<char>\S+) =+")


def default_log_path():
    """Best guess at the trace location for a stock build_and_run.bat setup."""
    for candidate in (
        r"C:\Build\bin\RelWithDebInfo\alonecraft_test.log",
        r"C:\Build\bin\RelWithDebInfo\logs\alonecraft_test.log",
        r"C:\Build\alonecraft_test.log",
    ):
        if os.path.exists(candidate):
            return candidate
    return None


def parse(path):
    rows = []
    with open(path, encoding="utf-8", errors="replace") as handle:
        for raw in handle:
            raw = raw.rstrip("\n")
            match = LINE_RE.match(raw)
            if not match:
                continue

            if match.group("marker"):
                rows.append({
                    "kind": "marker",
                    "wall": match.group("wall"),
                    "ms": int(match.group("ms")),
                    "text": match.group("marker"),
                    "raw": raw,
                })
            else:
                rows.append({
                    "kind": "line",
                    "wall": match.group("wall"),
                    "ms": int(match.group("ms")),
                    "case": match.group("case"),
                    "tag": match.group("tag"),
                    "msg": match.group("msg"),
                    "raw": raw,
                })
    return rows


def cmd_list(rows):
    """One row per case actually opened with `.woatest begin`."""
    sessions = []
    for row in rows:
        if row["kind"] != "marker":
            continue
        begin = BEGIN_RE.search(row["text"])
        if begin:
            sessions.append((begin.group("case"), begin.group("char"), row["wall"]))

    counts = Counter(r["case"] for r in rows if r["kind"] == "line")
    tags = defaultdict(Counter)
    for row in rows:
        if row["kind"] == "line":
            tags[row["case"]][row["tag"]] += 1

    if not sessions:
        print("No BEGIN markers found -- was `.woatest begin <case>` used?")
        return

    for case, char, wall in sessions:
        traced = counts.get(case, 0)
        non_state = sum(n for t, n in tags[case].items() if not t.startswith("STATE"))
        print(f"{case:<12} {char:<14} {wall}  lines={traced:<5} (non-STATE={non_state})")
        top = [f"{t}={n}" for t, n in tags[case].most_common()
               if not t.startswith("STATE")]
        if top:
            print(f"{'':<12} {' '.join(top)}")


def cmd_case(rows, case, show_state, tag_filter):
    hit = False
    for row in rows:
        if row["kind"] == "marker":
            if case in row["text"]:
                hit = True
                print(row["raw"])
            continue

        if row["case"] != case:
            continue
        if not show_state and row["tag"].startswith("STATE"):
            continue
        if tag_filter and not row["tag"].startswith(tag_filter):
            continue

        hit = True
        print(f"  {row['tag']:<22} {row['msg']}")

    if not hit:
        print(f"Nothing recorded for case '{case}'.")


def cmd_tag(rows, tag):
    for row in rows:
        if row["kind"] == "line" and row["tag"].startswith(tag):
            print(f"[{row['case']:<10}] {row['tag']:<22} {row['msg']}")


def cmd_rolls(rows):
    """Observed PASS/fail rate per (case, tag) -- the point of logging failures."""
    stats = defaultdict(lambda: [0, 0])
    for row in rows:
        if row["kind"] != "line":
            continue
        msg = row["msg"]
        if "roll=PASS" in msg:
            stats[(row["case"], row["tag"])][0] += 1
        elif "roll=fail" in msg or "roll=FAIL" in msg:
            stats[(row["case"], row["tag"])][1] += 1

    if not stats:
        print("No roll= lines recorded.")
        return

    print(f"{'case':<12} {'tag':<22} {'pass':>5} {'fail':>5} {'total':>6} {'rate':>7}")
    for (case, tag), (passes, fails) in sorted(stats.items()):
        total = passes + fails
        rate = 100.0 * passes / total if total else 0.0
        print(f"{case:<12} {tag:<22} {passes:>5} {fails:>5} {total:>6} {rate:>6.1f}%")


def cmd_aura_diff(rows, case):
    """Auras present in the 'after' snapshot but not 'before', and vice versa."""
    phase = None
    snapshots = {"before": {}, "after": {}}

    for row in rows:
        if row["kind"] != "line" or row["case"] != case:
            continue

        if row["tag"] == "STATE" and "---- before ----" in row["msg"]:
            phase = "before"
        elif row["tag"] == "STATE" and "---- after ----" in row["msg"]:
            phase = "after"
        elif row["tag"] == "STATE" and row["msg"].startswith("---- end"):
            phase = None
        elif row["tag"] == "STATE.AURA" and phase:
            key = re.search(r"id=(\d+)", row["msg"])
            if key:
                snapshots[phase][key.group(1)] = row["msg"]

    before, after = snapshots["before"], snapshots["after"]
    if not before and not after:
        print(f"No before/after snapshots for case '{case}'.")
        return

    for aura_id in sorted(set(after) - set(before), key=int):
        print(f"+ GAINED  {after[aura_id]}")
    for aura_id in sorted(set(before) - set(after), key=int):
        print(f"- LOST    {before[aura_id]}")
    for aura_id in sorted(set(before) & set(after), key=int):
        if before[aura_id] != after[aura_id]:
            print(f"~ CHANGED {aura_id}")
            print(f"    before: {before[aura_id]}")
            print(f"    after:  {after[aura_id]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--log", help="path to alonecraft_test.log")
    parser.add_argument("--list", action="store_true", help="list recorded cases")
    parser.add_argument("--case", help="show everything for one case id")
    parser.add_argument("--tag", help="show one tag prefix across all cases")
    parser.add_argument("--rolls", action="store_true", help="PASS/fail rates")
    parser.add_argument("--aura-diff", help="before/after aura delta for a case")
    parser.add_argument("--no-state", action="store_true", help="hide STATE.* lines")
    args = parser.parse_args()

    path = args.log or default_log_path()
    if not path or not os.path.exists(path):
        print("Could not find alonecraft_test.log -- pass --log <path>.\n"
              "In game, `.woatest` prints the resolved path.", file=sys.stderr)
        return 1

    rows = parse(path)
    print(f"# {path} -- {len(rows)} parsed lines\n")

    if args.list:
        cmd_list(rows)
    elif args.case:
        cmd_case(rows, args.case, not args.no_state, args.tag)
    elif args.aura_diff:
        cmd_aura_diff(rows, args.aura_diff)
    elif args.rolls:
        cmd_rolls(rows)
    elif args.tag:
        cmd_tag(rows, args.tag)
    else:
        cmd_list(rows)

    return 0


if __name__ == "__main__":
    sys.exit(main())
