#!/usr/bin/env bash
# in class sessions, one folder per session with the java files inside
#   bash in_class_activity/run.sh new 2026-10-06 [cpp]  makes the folder with a starting Main.java (or main.cpp)
#   bash in_class_activity/run.sh 2026-10-06 [args]     compiles every .java file in it and runs Main,
#                                                       or compiles every .cpp file and runs it with time

base=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

if [ $# -eq 0 ]; then
  echo "usage: bash in_class_activity/run.sh new <name>   or   bash in_class_activity/run.sh <name> [args]"
  ls "$base" | grep -v -e '^_' -e '\.sh$'
  exit 1
fi

if [ "$1" = "new" ]; then
  [ -n "${2:-}" ] || { echo "give a name for the session, for example 2026-10-06"; exit 1; }
  [ -e "$base/$2" ] && { echo "$2 already exists"; exit 1; }
  mkdir -p "$base/$2"
  if [ "${3:-}" = "cpp" ]; then
    cp "$base/_template/main.cpp" "$base/$2/main.cpp"
    echo "created $base/$2/main.cpp"
  else
    cp "$base/_template/Main.java" "$base/$2/Main.java"
    echo "created $base/$2/Main.java"
  fi
  exit 0
fi

dir=$base/$1
[ -d "$dir" ] || { echo "no folder $1"; exit 1; }
shift
mkdir -p "$dir/build"
if ls "$dir"/*.cpp >/dev/null 2>&1; then
  g++ -Wall -pthread "$dir"/*.cpp -o "$dir/build/main" || exit 1
  time "$dir/build/main" "$@"
  exit $?
fi
javac -d "$dir/build" "$dir"/*.java || exit 1
java -cp "$dir/build" Main "$@"
