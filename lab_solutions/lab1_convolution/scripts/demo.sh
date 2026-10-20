#!/usr/bin/env bash
# short run for the teacher: a 10 x 10 matrix to look at the numbers, then 1000 x 1000 for the times
# every run checks output.txt against output_seq.txt
# LANGS=java runs only the Java program (the default is cpp_static cpp_dynamic java)
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")

langs=${LANGS:-"cpp_static cpp_dynamic java"}

case "$langs" in *cpp*) make --no-print-directory -C "$lab/cpp" || exit 1 ;; esac
case "$langs" in *java*) make --no-print-directory -C "$lab/java" || exit 1 ;; esac

# gen <folder> <n> <m> <k>, makes the input files once
gen() {
  if [ ! -f "$1/.ready" ]; then
    mkdir -p "$1"
    python3 "$lab/scripts/gen_data.py" "$1" "$2" "$3" "$4" && touch "$1/.ready" || exit 1
  fi
}

progs=()
for l in $langs; do
  case $l in
    cpp_static) progs+=("C++ static:$lab/cpp/build/main_static") ;;
    cpp_dynamic) progs+=("C++ dynamic:$lab/cpp/build/main_dynamic") ;;
    java) progs+=("Java:java -Xmx3g -cp $lab/java/build Main") ;;
    *) echo "unknown language $l"; exit 1 ;;
  esac
done

# run <folder> <p> <variant>, prints the time of every implementation and the result of the check
run() {
  local dir=$1 p=$2 v=$3
  for pr in "${progs[@]}"; do
    local label=${pr%%:*} cmd=${pr#*:}
    local t
    t=$(cd "$dir" && $cmd "$p" "$v") || { echo "$label $v p=$p: check failed"; exit 1; }
    printf '%-12s %-10s p=%-2s %10.4f ms\n' "$label" "$v" "$p" "$t"
  done
}

gen "$data/demo_small" 10 10 3
gen "$data/demo" 1000 1000 3

echo "input files are in $data/demo_small"
echo "MatrixF.txt:"; cat "$data/demo_small/MatrixF.txt"
echo "ConvolutionM.txt:"; cat "$data/demo_small/ConvolutionM.txt"
run "$data/demo_small" 0 seq
echo "output_seq.txt:"; cat "$data/demo_small/output_seq.txt"
python3 "$lab/scripts/reference.py" "$data/demo_small" || exit 1
for v in horizontal vertical block linear cyclic; do run "$data/demo_small" 4 $v; done
echo "output.txt is the same as output_seq.txt in every run above (checked inside the programs)"
echo "output.txt of the last run:"; cat "$data/demo_small/output.txt"

echo
echo "1000 x 1000, k = 3"
run "$data/demo" 0 seq
for v in horizontal vertical block linear cyclic; do run "$data/demo" 4 $v; done
