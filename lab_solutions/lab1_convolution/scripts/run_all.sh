#!/usr/bin/env bash
# full test run for lab 1: C++ (static and dynamic matrices) and Java, 10 runs for every test case,
# variant and number of threads, the output is checked against the sequential one on every run
# LANGS="java" runs only one of the three, RUNS=3 changes the number of runs
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}
langs=${LANGS:-"cpp_static cpp_dynamic java"}

# "name:arguments for gen_data.py (n m k):thread counts"
cases=(
  "n10_m10_k3:10 10 3:4"
  "n1000_m1000_k3:1000 1000 3:2 4 8 16"
  "n10_m10000_k5:10 10000 5:2 4 8 16"
  "n10000_m10_k5:10000 10 5:2 4 8 16"
  "n10000_m10000_k5:10000 10000 5:2 4 8 16"
)
variants=${VARIANTS:-"horizontal vertical block linear cyclic"}

# run_lang <csv name> <command...>
# the program gets the thread count (0 = sequential) and the variant as arguments
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
    for v in $variants; do
      for p in $threads; do
        bash "$root/tools/bench.sh" -o "$csv" -v "$v" -c "$cname" -t "$p" -r "$runs" -d "$dir" -- "$@" "$p" "$v" || return 1
      done
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
make --no-print-directory -C "$lab/cpp" || exit 1
make --no-print-directory -C "$lab/java" || exit 1

for l in $langs; do
  case $l in
    cpp_static) run_lang cpp_static "$lab/cpp/build/main_static" ;;
    cpp_dynamic) run_lang cpp_dynamic "$lab/cpp/build/main_dynamic" ;;
    java) run_lang java java -Xmx3g -cp "$lab/java/build" Main ;;
    *) echo "unknown language $l"; exit 1 ;;
  esac || exit 1
done
