#!/usr/bin/env bash
# full test run for lab 4: 10 runs for every test, number of threads and number of readers,
# the output is checked against the sequential one on every run
# RUNS=3 changes the number of runs
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}
csv=$lab/results/cpp.csv

# "name:number of students", n100 is the test from the statement, n2000 is one more with longer files
cases=( "n100:100" "n2000:2000" )
threads="4 8 16"
readers="2 3"

for c in "${cases[@]}"; do
  cname=${c%%:*}
  gargs=${c#*:}
  if [ ! -f "$data/$cname/.ready" ]; then
    mkdir -p "$data/$cname"
    echo "generating $cname"
    python3 "$lab/scripts/gen_data.py" "$data/$cname" $gargs && touch "$data/$cname/.ready" || exit 1
  fi
done

mkdir -p "$lab/results"
bash "$root/tools/lab.sh" machine > "$lab/results/machine.txt"
make --no-print-directory -C "$lab/cpp" || exit 1
rm -f "$csv"

for c in "${cases[@]}"; do
  cname=${c%%:*}
  dir=$data/$cname
  echo "$cname"
  # sequential goes first, it writes the output the others are compared with
  bash "$root/tools/bench.sh" -o "$csv" -v seq -c "$cname" -t 0 -r "$runs" -d "$dir" -- "$lab/cpp/build/main" 0 || exit 1
  for pr in $readers; do
    for p in $threads; do
      bash "$root/tools/bench.sh" -o "$csv" -v "pr$pr" -c "$cname" -t "$p" -r "$runs" -d "$dir" -- "$lab/cpp/build/main" "$p" "$pr" || exit 1
    done
  done
done

python3 "$root/tools/report.py" "$csv" --plot "$lab/results/cpp.png"
