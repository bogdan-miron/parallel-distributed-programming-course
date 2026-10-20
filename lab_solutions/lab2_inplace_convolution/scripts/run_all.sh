#!/usr/bin/env bash
# full test run for lab 2: C++ and Java, 10 runs for every test case and number of threads,
# the output is checked against the sequential one on every run
# LANGS="java" runs only one of them, RUNS=3 changes the number of runs
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}
langs=${LANGS:-"cpp java"}

# "name:arguments for gen_data.py (n m):thread counts"
cases=(
  "n10_m10:10 10:2"
  "n1000_m1000:1000 1000:2 4 8 16"
  "n10000_m10000:10000 10000:2 4 8 16"
)

# run_lang <csv name> <command...>
# the program gets the thread count as argument, 0 is the sequential version
run_lang() {
  local name=$1; shift
  local csv=$lab/results/$name.csv
  rm -f "$csv"
  for c in "${cases[@]}"; do
    local cname=${c%%:*}
    local threads=${c##*:}
    local dir=$data/$cname
    echo "$name $cname"
    # sequential goes first, it writes the output the others are compared with
    bash "$root/tools/bench.sh" -o "$csv" -v seq -c "$cname" -t 0 -r "$runs" -d "$dir" -- "$@" 0 || return 1
    for p in $threads; do
      bash "$root/tools/bench.sh" -o "$csv" -v rows -c "$cname" -t "$p" -r "$runs" -d "$dir" -- "$@" "$p" || return 1
    done
  done
  python3 "$root/tools/report.py" "$csv"
}

for c in "${cases[@]}"; do
  cname=${c%%:*}
  rest=${c#*:}
  gargs=${rest%%:*}
  if [ ! -f "$data/$cname/.ready" ]; then
    mkdir -p "$data/$cname"
    echo "generating $cname"
    python3 "$lab/scripts/gen_data.py" "$data/$cname" $gargs && touch "$data/$cname/.ready" || exit 1
  fi
done

bash "$root/tools/lab.sh" machine > "$lab/results/machine.txt"
case "$langs" in *cpp*) make --no-print-directory -C "$lab/cpp" || exit 1 ;; esac
case "$langs" in *java*) make --no-print-directory -C "$lab/java" || exit 1 ;; esac

for l in $langs; do
  case $l in
    cpp) run_lang cpp "$lab/cpp/build/main" ;;
    java) run_lang java java -Xmx2g -cp "$lab/java/build" Main ;;
    *) echo "unknown language $l"; exit 1 ;;
  esac || exit 1
done
