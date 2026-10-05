#!/usr/bin/env bash
# short run to show the teacher: one small case, sequential and parallel, with the check
lab=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
data=${PPD_DATA_ROOT:-$HOME/ppd_data}/$(basename "$lab")/demo

make --no-print-directory -C "$lab/cpp" || exit 1
mkdir -p "$data"
# TODO generate a small input in $data, run the sequential version, then the parallel ones
