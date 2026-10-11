#!/usr/bin/env bash
# counts the page faults of the sequential run, static against dynamic, on the 10000 x 10 case
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
dir=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")/n10000_m10_k5
[ -f "$dir/.ready" ] || { echo "run the benchmark first, it makes the input files"; exit 1; }
for b in main_static main_dynamic; do
  python3 - "$lab/cpp/build/$b" "$dir" "$b" <<'PY'
import resource, subprocess, sys
prog, d, name = sys.argv[1:]
out = subprocess.run([prog, "0"], cwd=d, capture_output=True, text=True, check=True).stdout.strip()
print(f"{name}: time {out} ms, page faults {resource.getrusage(resource.RUSAGE_CHILDREN).ru_minflt}")
PY
done
