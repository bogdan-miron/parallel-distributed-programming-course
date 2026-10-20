#!/usr/bin/env bash
# peak memory of the sequential run on the 10000 x 10000 matrix, lab 2 against lab 1 (the lab 1 C++ with new, and its Java)
# needs the input files of both labs, they are made by the benchmark runs
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
base=${PPD_DATA_ROOT:-$HOME/ppd_data}
lab1=$(cd "$lab/../lab1_convolution" && pwd)

# peak <label> <folder> <command...>
peak() {
  local label=$1 dir=$2
  shift 2
  [ -f "$dir/.ready" ] || { echo "$label: no input files in $dir"; return; }
  python3 - "$label" "$dir" "$@" <<'PY'
import resource, subprocess, sys
label, d, cmd = sys.argv[1], sys.argv[2], sys.argv[3:]
subprocess.run(cmd, cwd=d, capture_output=True, check=True)
print(f"{label}: peak memory {resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss / 1024:.0f} MB")
PY
}

peak "lab 2 C++ " "$base/lab2_inplace_convolution/n10000_m10000" "$lab/cpp/build/main" 0
peak "lab 1 C++ " "$base/lab1_convolution/n10000_m10000_k5" "$lab1/cpp/build/main_dynamic" 0
peak "lab 2 Java" "$base/lab2_inplace_convolution/n10000_m10000" java -Xmx3g -cp "$lab/java/build" Main 0
peak "lab 1 Java" "$base/lab1_convolution/n10000_m10000_k5" java -Xmx3g -cp "$lab1/java/build" Main 0
