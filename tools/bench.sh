#!/usr/bin/env bash
# runs a program several times and appends the average time to a csv
#
# usage: bash bench.sh -o results.csv -v variant -t threads [-c config] [-r runs] [-d workdir] -- cmd args...
#
# the program has to print its execution time in ms as the last line of stdout
# and exit with a non zero code when its own output check fails.
# the program runs inside workdir (where the input files are).

runs=10
variant=""
config=""
threads=0
out=""
workdir="."

while getopts "o:v:c:t:r:d:" opt; do
  case $opt in
    o) out=$OPTARG ;;
    v) variant=$OPTARG ;;
    c) config=$OPTARG ;;
    t) threads=$OPTARG ;;
    r) runs=$OPTARG ;;
    d) workdir=$OPTARG ;;
    *) exit 2 ;;
  esac
done
shift $((OPTIND - 1))
[ "${1:-}" = "--" ] && shift

if [ -z "$out" ] || [ $# -eq 0 ]; then
  echo "usage: bash bench.sh -o results.csv -v variant -t threads [-c config] [-r runs] [-d workdir] -- cmd args..."
  exit 2
fi

# the program runs in another folder, so paths that exist here must become absolute
cmd=()
for a in "$@"; do
  if [ -e "$a" ]; then cmd+=("$(realpath "$a")"); else cmd+=("$a"); fi
done
out=$(realpath -m "$out")

mkdir -p "$(dirname "$out")"
[ -f "$out" ] || echo "variant,config,threads,runs,mean_ms,min_ms,max_ms,ok" > "$out"

times=()
ok=1
for i in $(seq 1 "$runs"); do
  res=$(cd "$workdir" && "${cmd[@]}")
  code=$?
  t=$(printf '%s\n' "$res" | tail -n 1 | tr -d '\r ')
  if [ $code -ne 0 ]; then
    echo "run $i failed (exit code $code): $variant $config p=$threads" >&2
    ok=0
    break
  fi
  if ! [[ $t =~ ^[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$ ]]; then
    echo "run $i: last line is not a time: '$t'" >&2
    ok=0
    break
  fi
  times+=("$t")
done

if [ ${#times[@]} -gt 0 ]; then
  stats=$(printf '%s\n' "${times[@]}" | LC_ALL=C awk '
    { s += $1; if (NR == 1 || $1 < mn) mn = $1; if (NR == 1 || $1 > mx) mx = $1 }
    END { printf "%.4f,%.4f,%.4f", s / NR, mn, mx }')
else
  stats=",,"
fi

echo "$variant,$config,$threads,${#times[@]},$stats,$ok" >> "$out"
echo "$variant $config p=$threads: mean/min/max ms = $stats, runs = ${#times[@]}, ok = $ok"
[ $ok -eq 1 ]
