#!/usr/bin/env bash
# full test run for lab 0: 10 runs for every size and language, the output is checked on every run
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
root=$(cd "$lab/../.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")
runs=${RUNS:-10}

bash "$root/tools/lab.sh" machine > "$lab/results/machine.txt"
make --no-print-directory -C "$lab/cpp" || exit 1
make --no-print-directory -C "$lab/java" || exit 1
rm -f "$lab/results/cpp.csv" "$lab/results/java.csv"

for n in 100000 1000000 10000000; do
  dir=$data/n$n
  if [ ! -f "$dir/.ready" ]; then
    mkdir -p "$dir"
    python3 "$lab/scripts/gen_data.py" "$dir" $n && touch "$dir/.ready" || exit 1
  fi
  echo "C++ n = $n"
  bash "$root/tools/bench.sh" -o "$lab/results/cpp.csv" -v seq -c "n=$n" -t 0 -r "$runs" -d "$dir" -- "$lab/cpp/build/main" || exit 1
  echo "Java n = $n"
  bash "$root/tools/bench.sh" -o "$lab/results/java.csv" -v seq -c "n=$n" -t 0 -r "$runs" -d "$dir" -- java -cp "$lab/java/build" Main || exit 1
done

echo
echo "C++"
python3 "$root/tools/report.py" "$lab/results/cpp.csv"
echo
echo "Java"
python3 "$root/tools/report.py" "$lab/results/java.csv"
