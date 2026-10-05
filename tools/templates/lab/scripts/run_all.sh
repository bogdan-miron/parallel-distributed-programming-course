#!/usr/bin/env bash
# full test run for __LAB__
# 10 runs for every config, the output is checked against the sequential one on every run
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}

# one entry per test case: "name:arguments for gen_data.py"
cases=( "small:10 10 3" )
threads="2 4 8 16"
variants="horizontal vertical"

# run_lang <csv name> <command...>
# the program gets the thread count (0 = sequential) and the variant as arguments
run_lang() {
  local name=$1; shift
  local csv=$lab/results/$name.csv
  rm -f "$csv"
  for c in "${cases[@]}"; do
    local cname=${c%%:*}
    local dir=$data/$cname
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
  gargs=${c#*:}
  if [ ! -f "$data/$cname/.ready" ]; then
    mkdir -p "$data/$cname"
    python3 "$lab/scripts/gen_data.py" "$data/$cname" $gargs && touch "$data/$cname/.ready" || exit 1
  fi
done

bash "$root/tools/lab.sh" machine > "$lab/results/machine.txt"
make --no-print-directory -C "$lab/cpp" || exit 1
run_lang cpp "$lab/cpp/build/main"

if [ -f "$lab/java/Main.java" ]; then
  make --no-print-directory -C "$lab/java" || exit 1
  run_lang java java -cp "$lab/java/build" Main
fi

# same protocol as the C++ version, only the folder and the compiler change
for extra in omp cuda; do
  src=$lab/$extra/main.cpp; [ $extra = cuda ] && src=$lab/$extra/main.cu
  if [ -f "$src" ]; then
    make --no-print-directory -C "$lab/$extra" || exit 1
    run_lang $extra "$lab/$extra/build/main"
  fi
done
# mpi programs are started with mpirun, so use a separate loop for them:
#   bash "$root/tools/bench.sh" -o "$csv" -v mpi -c "$cname" -t 4 -d "$dir" -- mpirun -np 4 "$lab/mpi/build/main"
