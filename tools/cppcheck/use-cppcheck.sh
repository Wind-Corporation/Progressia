#!/bin/bash

usage=\
"Usage: use-cppcheck.sh
Run cppcheck with correct options.

Environment variables:
  PARALLELISM  threads to use, default is 1

  CPPCHECK  cppcheck executable
  CMAKE     cmake executable"

rsrc="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$rsrc/../bashlib.sh"

find_cmd CPPCHECK cppcheck
find_cmd CMAKE cmake

case "$1" in
    -h | --help )
        echo "$usage"
        exit
        ;;
esac

# Generate compile database for CppCheck
command "$CMAKE" \
    -B "$build_dir" \
    -S "$source_dir" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

compile_database="$build_dir/compile_commands.json"

mkdir -p "$build_dir/cppcheck"

options=()

while IFS='' read -r line; do
    [ -z "$line" ] && continue
    [ "${line:0:1}" = '#' ] && continue

    option="$(
        CMAKE_SOURCE_DIR="$source_dir" \
        CMAKE_BINARY_DIR="$build_dir" \
        envsubst <<<"$line"
    )"

    options+=("$option")
done < "$tools_dir/cppcheck/options.txt"

[ -n "${PARALLELISM+x}" ] && options+=(-j "$PARALLELISM")

errors="`
    echo_and_run "$CPPCHECK" \
        --project="$compile_database" \
        -D__CPPCHECK__ \
        "${options[@]}" \
    2>&1 >/dev/fd/0 # Store stderr into variable, pass stdout to our stdout
`"

exit_code="$?"
if [ "$exit_code" -eq 2 ]; then
    less - <<<"$errors"
    exit "$exit_code"
fi
