#!/usr/bin/env bash
# short run for the teacher: the 16 digit numbers from the statement with every variant, then bigger numbers for the times
# every parallel run compares Numar3.txt with output_seq.txt, and the sequential result is compared with the one python computes
# MPIFLAGS changes the options of mpirun
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
flags=${MPIFLAGS:---oversubscribe}

make --no-print-directory -C "$lab/mpi" || exit 1

# gen <folder> <arguments of gen_data.py>
gen() {
  local dir=$1; shift
  if [ ! -f "$dir/.ready" ]; then
    mkdir -p "$dir"
    python3 "$lab/scripts/gen_data.py" "$dir" "$@" && touch "$dir/.ready" || exit 1
  fi
}

# run <folder> <p> <variant>, prints the time, the check is done inside the program
run() {
  local dir=$1 p=$2 v=$3 exe=$lab/mpi/build/main label=$3
  if [ "$v" = "1s" ]; then exe=$lab/mpi/build/main_sync; v=1; fi
  local t
  t=$(cd "$dir" && mpirun $flags -np "$p" "$exe" "$v" 2>/dev/null) || { echo "variant $label p=$p: check failed"; exit 1; }
  printf 'variant %-4s p=%-2s %10.4f ms\n' "$label" "$p" "$t"
}

gen "$data/demo_small" fixed
gen "$data/demo_nines" nines 40
gen "$data/demo" 10000 10000

small=$data/demo_small
echo "Numar1.txt ($(head -1 "$small/Numar1.txt") digits, the least significant first):"; tail -1 "$small/Numar1.txt"
echo "Numar2.txt:"; tail -1 "$small/Numar2.txt"
run "$small" 1 0 >/dev/null
echo "sum made by the sequential version (output_seq.txt):"; cat "$small/output_seq.txt"
if cmp -s "$small/output_seq.txt" "$small/expected.txt"; then echo "same as the sum made by python: OK"; else echo "different from python: FAILED"; exit 1; fi

echo
echo "16 digits, the processes split them as 4 + 4 + 4 + 4 (p = 5), the last process gives the final carry"
run "$small" 5 1
run "$small" 5 1s
run "$small" 5 1.1
run "$small" 5 3
run "$small" 4 2
echo "Numar3.txt:"; cat "$small/Numar3.txt"

echo
echo "99...9 (40 digits) + 1, the carry goes through all the processes"
nines=$data/demo_nines
run "$nines" 1 0 >/dev/null
cmp -s "$nines/output_seq.txt" "$nines/expected.txt" && echo "sequential same as python: OK" || { echo "FAILED"; exit 1; }
for p in 3 5 9; do run "$nines" $p 1s; run "$nines" $p 1.1; run "$nines" $p 3; done
run "$nines" 4 2; run "$nines" 8 2

echo
echo "10000 digits"
run "$data/demo" 1 0
for p in 5 9; do for v in 1 1.1 3; do run "$data/demo" $p $v; done; done
for p in 4 8; do run "$data/demo" $p 2; done
