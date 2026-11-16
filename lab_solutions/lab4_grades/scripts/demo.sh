#!/usr/bin/env bash
# short run for the teacher: 100 students and 10 files, sequential first, then the parallel version with different numbers of threads and readers
# every parallel run compares Results.txt with output_seq.txt, and the sequential result is compared with the one python computes
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
exe=$lab/cpp/build/main

make --no-print-directory -C "$lab/cpp" || exit 1

# gen <folder> <students>
gen() {
  if [ ! -f "$1/.ready" ]; then
    mkdir -p "$1"
    python3 "$lab/scripts/gen_data.py" "$1" "$2" && touch "$1/.ready" || exit 1
  fi
}

# run <folder> <p> <pr>, prints the time, the check is done inside the program
run() {
  local t
  t=$(cd "$1" && "$exe" "$2" "$3") || { echo "p=$2 pr=$3: check failed"; exit 1; }
  printf 'p=%-2s pr=%-2s %10.4f ms\n' "$2" "$3" "$t"
}

gen "$data/demo_small" 5
gen "$data/demo" 100
gen "$data/demo_big" 2000

small=$data/demo_small
echo "5 students, project1.txt (id grade):"; cat "$small/project1.txt"
echo "project2.txt:"; cat "$small/project2.txt"
run "$small" 0 0 >/dev/null
echo "Results (id and final grade) from the sequential version:"; cat "$small/output_seq.txt"
cmp -s "$small/output_seq.txt" "$small/expected.txt" && echo "same as the grades made by python: OK" || { echo "different from python: FAILED"; exit 1; }
for pp in "3 2" "6 3"; do run "$small" $pp; done
echo "Results.txt:"; cat "$small/Results.txt"

echo
echo "100 students"
run "$data/demo" 0 0 >/dev/null
cmp -s "$data/demo/output_seq.txt" "$data/demo/expected.txt" && echo "sequential same as python: OK" || { echo "FAILED"; exit 1; }
echo "sequential: $(cd "$data/demo" && "$exe" 0) ms"
for pr in 2 3; do for p in 4 8 16; do run "$data/demo" $p $pr; done; done

echo
echo "2000 students"
echo "sequential: $(cd "$data/demo_big" && "$exe" 0) ms"
for pr in 2 3; do for p in 4 8 16; do run "$data/demo_big" $p $pr; done; done
