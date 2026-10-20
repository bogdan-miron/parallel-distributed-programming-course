#!/usr/bin/env bash
# short run for the teacher: a 10 x 10 matrix to look at the numbers, then 1000 x 1000 for the times
# every run checks output.txt against output_seq.txt
# LANGS=java runs only the Java program (the default is cpp java)
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
langs=${LANGS:-"cpp java"}

case "$langs" in *cpp*) make --no-print-directory -C "$lab/cpp" || exit 1 ;; esac
case "$langs" in *java*) make --no-print-directory -C "$lab/java" || exit 1 ;; esac

# gen <folder> <n> <m>, makes date.txt once
gen() {
  if [ ! -f "$1/.ready" ]; then
    mkdir -p "$1"
    python3 "$lab/scripts/gen_data.py" "$1" "$2" "$3" && touch "$1/.ready" || exit 1
  fi
}

progs=()
for l in $langs; do
  case $l in
    cpp) progs+=("C++:$lab/cpp/build/main") ;;
    java) progs+=("Java:java -Xmx2g -cp $lab/java/build Main") ;;
    *) echo "unknown language $l"; exit 1 ;;
  esac
done

# run <folder> <p>, prints the time of every implementation, the check is done inside the programs
run() {
  local dir=$1 p=$2
  for pr in "${progs[@]}"; do
    local label=${pr%%:*} cmd=${pr#*:}
    local t
    t=$(cd "$dir" && $cmd "$p") || { echo "$label p=$p: check failed"; exit 1; }
    printf '%-5s p=%-2s %10.4f ms\n' "$label" "$p" "$t"
  done
}

gen "$data/demo_small" 10 10
gen "$data/demo" 1000 1000

echo "input file is $data/demo_small/date.txt (n m, the matrix, then the kernel)"
cat "$data/demo_small/date.txt"
run "$data/demo_small" 0
echo "output_seq.txt:"; cat "$data/demo_small/output_seq.txt"
python3 "$lab/scripts/reference.py" "$data/demo_small" || exit 1

echo
echo "10 rows, 10 x 10: p = 2 gives 5 + 5 rows, p = 3 gives 4 + 3 + 3, p = 4 gives 3 + 3 + 2 + 2, p = 7 gives 2 + 2 + 2 + 1 + 1 + 1 + 1"
for p in 2 3 4 7; do run "$data/demo_small" $p; done
echo "output.txt is the same as output_seq.txt in every run above (checked inside the programs)"

echo
echo "1000 x 1000"
run "$data/demo" 0
for p in 2 4 8 16; do run "$data/demo" $p; done
