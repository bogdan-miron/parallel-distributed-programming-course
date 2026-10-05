#!/usr/bin/env bash
# short run for the teacher: a tiny case to look at the numbers, then n = 1000000 for the times
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")

make --no-print-directory -C "$lab/cpp" || exit 1
make --no-print-directory -C "$lab/java" || exit 1

# gen <folder> <n>, makes the input files once
gen() {
  if [ ! -f "$1/.ready" ]; then
    mkdir -p "$1"
    python3 "$lab/scripts/gen_data.py" "$1" "$2" && touch "$1/.ready" || exit 1
  fi
}

# run <label> <folder> <command...>, runs the program inside the folder with the input files
run() {
  local label=$1 dir=$2
  shift 2
  local t
  t=$(cd "$dir" && "$@") || { echo "$label: check failed"; exit 1; }
  t=$(printf '%.4f' "$t")
  echo "$label: time = $t ms, output.txt is the same as expected.txt"
}

cpp=$lab/cpp/build/main
java=(java -cp "$lab/java/build" Main)

gen "$data/demo_small" 10
gen "$data/demo" 1000000

echo "input files are in $data/demo_small"
run "C++  n = 10" "$data/demo_small" "$cpp"
echo "A, B and output side by side (first line of each file is n):"
paste "$data/demo_small/A.txt" "$data/demo_small/B.txt" "$data/demo_small/output.txt"
run "Java n = 10" "$data/demo_small" "${java[@]}"
echo
run "C++  n = 1000000" "$data/demo" "$cpp"
run "Java n = 1000000" "$data/demo" "${java[@]}"
