#!/usr/bin/env bash
# one entry point for a lab: new, build, demo, bench, report, doc, status, machine, doctor
# usage: bash tools/lab.sh <action> [lab_name]
# inside a lab folder the lab name can be left out

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
labs="$root/lab_solutions"
action=${1:-help}
lab=${2:-}

if [ -z "$lab" ]; then
  case "$PWD/" in
    "$labs"/*/*) lab=${PWD#"$labs"/}; lab=${lab%%/*} ;;
  esac
fi
dir="$labs/$lab"

need_lab() {
  if [ -z "$lab" ] || [ ! -d "$dir" ]; then
    echo "lab not found, give the folder name: bash tools/lab.sh $action <lab_name>"
    exit 1
  fi
}

case $action in
  new)
    [ -n "$lab" ] || { echo "usage: bash tools/lab.sh new <lab_name> [java] [omp] [cuda] [mpi]"; exit 1; }
    [ -e "$dir" ] && { echo "$lab already exists"; exit 1; }
    cp -r "$root/tools/templates/lab" "$dir"
    grep -rl "__LAB__" "$dir" | xargs sed -i "s/__LAB__/$lab/g"
    mv "$dir/docs/lab.md" "$dir/docs/$lab.md"
    # extra folders: java, omp, cuda, mpi
    for extra in "${@:3}"; do
      if [ "$extra" != "lab" ] && [ -d "$root/tools/templates/$extra" ]; then
        cp -r "$root/tools/templates/$extra" "$dir/$extra"
      else
        echo "no template called $extra"
      fi
    done
    echo "created $dir"
    ;;
  build)
    need_lab
    found=0
    for mk in "$dir"/*/Makefile; do
      [ -f "$mk" ] || continue
      found=1
      make --no-print-directory -C "$(dirname "$mk")" || exit 1
    done
    [ $found = 1 ] || echo "no Makefile found in $lab"
    ;;
  demo)
    need_lab
    bash "$dir/scripts/demo.sh"
    ;;
  bench)
    need_lab
    bash "$dir/scripts/run_all.sh"
    ;;
  report)
    need_lab
    for f in "$dir"/results/*.csv; do
      [ -f "$f" ] || continue
      name=$(basename "$f" .csv)
      echo "## $name"
      python3 "$root/tools/report.py" "$f" --plot "$dir/results/$name.png"
    done
    ;;
  doc)
    need_lab
    command -v pandoc >/dev/null || { echo "pandoc missing: sudo apt install pandoc"; exit 1; }
    python3 "$root/tools/build_doc.py" "$dir"
    ;;
  list)
    ls "$labs"
    ;;
  status)
    for d in "$labs"/*/; do
      [ -d "$d" ] || continue
      langs=""
      for l in cpp java omp cuda mpi; do [ -d "$d$l" ] && langs="$langs$l "; done
      csvs=$(ls "$d"results/*.csv 2>/dev/null | wc -l)
      docs=$(ls "$d"docs/*.docx 2>/dev/null | wc -l)
      printf '%-28s code: %-18s results: %s csv, docs: %s docx\n' "$(basename "$d")" "$langs" "$csvs" "$docs"
    done
    ;;
  machine)
    cpu=$(lscpu 2>/dev/null | sed -n 's/^Model name: *//p' | head -1)
    ram=$(free -g 2>/dev/null | awk '/^Mem:/ {print $2}')
    os=$(. /etc/os-release 2>/dev/null; echo "${PRETTY_NAME:-linux}")
    echo "$cpu, $(nproc) logical cores, ${ram} GB RAM, $os, $(g++ --version 2>/dev/null | head -1)"
    ;;
  doctor)
    for t in g++ make java javac python3 pandoc gdb mpicc mpirun nvcc; do
      if command -v $t >/dev/null; then printf '%-8s ok\n' $t; else printf '%-8s missing\n' $t; fi
    done
    python3 -c "import matplotlib" 2>/dev/null && echo "matplotlib ok" || echo "matplotlib missing"
    echo "machine: $(bash "$0" machine)"
    ;;
  *)
    echo "usage: bash tools/lab.sh <new|build|demo|bench|report|doc|list|status|machine|doctor> [lab_name]"
    ;;
esac
