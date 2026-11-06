#!/usr/bin/env bash
# full test run for lab 3: 10 runs for every test and number of processes,
# the output is checked against the sequential one on every run
# RUNS=3 changes the number of runs, MPIFLAGS changes the options of mpirun
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}
mpirun_flags=${MPIFLAGS:---oversubscribe}
csv=$lab/results/mpi.csv

# "name:arguments for gen_data.py"
cases=(
  "n16:fixed"
  "n1000:1000 1000"
  "n10000:10000 10000"
  "n100_100000:100 100000"
)

# "case:variant:processes", the variants and the numbers of processes from the statement
# 1s is variant 1 with MPI_Ssend
tests=(
  "n16:1:5" "n16:1s:5" "n16:1.1:5" "n16:3:5" "n16:2:4"
  "n10000:1:5 9 17" "n10000:1s:5 9 17" "n10000:1.1:5 9 17" "n10000:3:5 9 17"
  "n1000:2:4 8 16"
  "n100_100000:1:5 9 17" "n100_100000:1s:5 9 17" "n100_100000:1.1:5 9 17" "n100_100000:3:5 9 17" "n100_100000:2:4 8 16"
)

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
make --no-print-directory -C "$lab/mpi" || exit 1
rm -f "$csv"

prev=""
for t in "${tests[@]}"; do
  cname=${t%%:*}
  rest=${t#*:}
  variant=${rest%%:*}
  procs=${rest#*:}
  dir=$data/$cname
  # sequential goes first for every case, it writes the output the others are compared with
  if [ "$cname" != "$prev" ]; then
    bash "$root/tools/bench.sh" -o "$csv" -v seq -c "$cname" -t 0 -r "$runs" -d "$dir" -- mpirun $mpirun_flags -np 1 "$lab/mpi/build/main" 0 || exit 1
    prev=$cname
  fi
  for p in $procs; do
    if [ "$variant" = "1s" ]; then
      bash "$root/tools/bench.sh" -o "$csv" -v 1s -c "$cname" -t "$p" -r "$runs" -d "$dir" -- mpirun $mpirun_flags -np "$p" "$lab/mpi/build/main_sync" 1 || exit 1
    else
      bash "$root/tools/bench.sh" -o "$csv" -v "$variant" -c "$cname" -t "$p" -r "$runs" -d "$dir" -- mpirun $mpirun_flags -np "$p" "$lab/mpi/build/main" "$variant" || exit 1
    fi
  done
done

python3 "$root/tools/report.py" "$csv" --plot "$lab/results/mpi.png"
