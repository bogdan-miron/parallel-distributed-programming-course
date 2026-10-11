#!/usr/bin/env python3
# builds the lab documentation (docx) from docs/<lab>.md
# the markdown can use {{table:cpp}} and {{chart:cpp}} (filled from results/cpp.csv)
# {{best:a:b:c}} and {{compare:a:b}} (short tables made by compare.py from results/a.csv, ...)
# and {{machine}} (filled from results/machine.txt)
# usage: python3 build_doc.py <lab_dir>
import os
import re
import subprocess
import sys
import tempfile

lab = os.path.abspath(sys.argv[1])
tools = os.path.dirname(os.path.abspath(__file__))
docs = os.path.join(lab, "docs")
results = os.path.join(lab, "results")


def table(name):
    csv_file = os.path.join(results, name + ".csv")
    if not os.path.exists(csv_file):
        sys.exit(f"missing {csv_file}, run the benchmark first")
    png = os.path.join(results, name + ".png")
    out = subprocess.run(["python3", os.path.join(tools, "report.py"), csv_file, "--plot", png],
                         capture_output=True, text=True, check=True).stdout
    # keep only the table lines, the chart is added with {{chart:name}}
    return "\n".join(l for l in out.splitlines() if not l.startswith("chart saved"))


def compare(mode, names):
    files = [os.path.join(results, n + ".csv") for n in names]
    for f in files:
        if not os.path.exists(f):
            sys.exit(f"missing {f}, run the benchmark first")
    return subprocess.run(["python3", os.path.join(tools, "compare.py"), mode] + files,
                          capture_output=True, text=True, check=True).stdout.strip()


for md in sorted(f for f in os.listdir(docs) if f.endswith(".md")):
    text = open(os.path.join(docs, md), encoding="utf-8").read()
    text = re.sub(r"\{\{table:(\w+)\}\}", lambda m: table(m.group(1)), text)
    text = re.sub(r"\{\{best:([\w:]+)\}\}", lambda m: compare("best", m.group(1).split(":")), text)
    text = re.sub(r"\{\{compare:(\w+):(\w+)\}\}", lambda m: compare("cmp", [m.group(1), m.group(2)]), text)
    machine = os.path.join(results, "machine.txt")
    text = text.replace("{{machine}}", open(machine).read().strip() if os.path.exists(machine) else "unknown machine")
    text = re.sub(r"\{\{chart:(\w+)\}\}", lambda m: f"![](../results/{m.group(1)}.png)", text)
    with tempfile.NamedTemporaryFile("w", suffix=".md", delete=False, encoding="utf-8") as t:
        t.write(text)
    target = os.path.join(docs, md[:-3] + ".docx")
    try:
        subprocess.run(["pandoc", t.name, "-o", target, "--resource-path=" + docs], check=True)
    finally:
        os.remove(t.name)
    print("wrote", target)
