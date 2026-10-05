#!/usr/bin/env python3
# turns a results csv (made by bench.sh) into markdown tables with speedup
# usage: python3 report.py results.csv [--plot chart.png]
import csv
import sys
from collections import OrderedDict


def load(path):
    rows = OrderedDict()
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            # same variant, config and threads twice: the last one wins
            rows[(r["config"], r["variant"], int(r["threads"]))] = r
    return rows


def by_config(rows):
    configs = OrderedDict()
    for (config, variant, threads), r in rows.items():
        configs.setdefault(config, []).append((variant, threads, r))
    return configs


def speedup(base, r):
    if base is None or not r["mean_ms"] or float(r["mean_ms"]) == 0:
        return None
    return float(base["mean_ms"]) / float(r["mean_ms"])


def print_sequential_only(configs):
    # nothing parallel in the file, so one table is enough
    print("| Test | Time (ms) | Min | Max | Check |")
    print("|---|---|---|---|---|")
    for config, items in configs.items():
        for variant, threads, r in items:
            ok = "ok" if r["ok"] == "1" else "FAILED"
            print(f"| {config} | {float(r['mean_ms']):.2f} | {float(r['min_ms']):.2f} | {float(r['max_ms']):.2f} | {ok} |")


def print_tables(configs):
    if all(v == "seq" for items in configs.values() for v, t, r in items):
        print_sequential_only(configs)
        return
    for config, items in configs.items():
        base = next((r for v, t, r in items if v == "seq" and r["ok"] == "1"), None)
        print(f"\n{config}\n")
        print("| Variant | Threads | Time (ms) | Speedup | Check |")
        print("|---|---|---|---|---|")
        best = None
        for variant, threads, r in items:
            s = speedup(base, r)
            ok = "ok" if r["ok"] == "1" else "FAILED"
            mean = f"{float(r['mean_ms']):.2f}" if r["mean_ms"] else "-"
            print(f"| {variant} | {threads} | {mean} | {'-' if s is None else f'{s:.2f}'} | {ok} |")
            if variant != "seq" and s is not None and r["ok"] == "1":
                if best is None or s > best[0]:
                    best = (s, variant, threads)
        if best:
            print(f"\nBest for {config}: {best[1]} with {best[2]} threads, speedup {best[0]:.2f}")


def plot(configs, path):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib not installed, skipping the chart", file=sys.stderr)
        return
    names = list(configs)
    fig, axes = plt.subplots(1, len(names), figsize=(4.5 * len(names), 3.5), squeeze=False)
    for ax, config in zip(axes[0], names):
        items = configs[config]
        base = next((r for v, t, r in items if v == "seq" and r["ok"] == "1"), None)
        series = OrderedDict()
        for variant, threads, r in items:
            s = speedup(base, r)
            if variant != "seq" and s is not None:
                series.setdefault(variant, []).append((threads, s))
        for variant, pts in series.items():
            pts.sort()
            ax.plot([p for p, _ in pts], [s for _, s in pts], marker="o", label=variant)
        ax.set_title(config)
        ax.set_xlabel("threads")
        ax.set_ylabel("speedup")
        ax.grid(True, alpha=0.3)
        if series:
            ax.legend()
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    print(f"\nchart saved to {path}")


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)
    chart = None
    if "--plot" in args:
        i = args.index("--plot")
        chart = args[i + 1]
        del args[i:i + 2]
    configs = by_config(load(args[0]))
    print_tables(configs)
    if chart and any(v != "seq" for items in configs.values() for v, t, r in items):
        plot(configs, chart)


main()
