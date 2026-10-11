#!/usr/bin/env python3
# short comparison tables between result files (made by bench.sh), used in the docs
#   python3 compare.py best a.csv [b.csv ...]   best parallel variant of each test case
#   python3 compare.py cmp a.csv b.csv          a against b: sequential, best parallel and the ratio over all runs
import csv
import math
import sys
from collections import OrderedDict


def load(path):
    rows = OrderedDict()
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            if r["ok"] == "1" and r["mean_ms"]:
                rows[(r["config"], r["variant"], int(r["threads"]))] = float(r["mean_ms"])
    return rows


def ms(x):
    return f"{x:.4f}" if x < 1 else f"{x:.2f}"


def name(path):
    return path.rsplit("/", 1)[-1][:-4]


def best_of(rows, config):
    seq = rows.get((config, "seq", 0))
    best = None
    for (c, v, t), ms in rows.items():
        if c == config and v != "seq" and (best is None or ms < best[0]):
            best = (ms, v, t)
    return seq, best


def configs_of(rows):
    return list(OrderedDict.fromkeys(c for c, v, t in rows))


def best(paths):
    print("| Test | Implementation | Best variant | Threads | Time (ms) | Speedup |")
    print("|---|---|---|---|---|---|")
    data = [(name(p), load(p)) for p in paths]
    for config in configs_of(data[0][1]):
        for label, rows in data:
            seq, b = best_of(rows, config)
            if b:
                s = f"{seq / b[0]:.2f}" if seq else "-"
                print(f"| {config} | {label} | {b[1]} | {b[2]} | {ms(b[0])} | {s} |")


def cmp(pa, pb):
    a, b = load(pa), load(pb)
    la, lb = name(pa), name(pb)
    print(f"| Test | {la} sequential | {lb} sequential | {la} best parallel | {lb} best parallel | {lb} / {la} (all runs) |")
    print("|---|---|---|---|---|---|")
    for config in configs_of(a):
        sa, ba = best_of(a, config)
        sb, bb = best_of(b, config)
        ratios = [b[k] / a[k] for k in a if k[0] == config and k in b and a[k] > 0]
        mean = math.exp(sum(math.log(r) for r in ratios) / len(ratios)) if ratios else None
        cell = lambda x: ms(x) if x is not None else "-"
        print(f"| {config} | {cell(sa)} | {cell(sb)} | {cell(ba[0] if ba else None)} | {cell(bb[0] if bb else None)} | {cell(mean)} |")


if len(sys.argv) >= 3 and sys.argv[1] == "best":
    best(sys.argv[2:])
elif len(sys.argv) == 4 and sys.argv[1] == "cmp":
    cmp(sys.argv[2], sys.argv[3])
else:
    print(__doc__)
    sys.exit(1)
