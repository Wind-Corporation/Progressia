#!/bin/bash

usage=\
"Usage: use-clang-format.sh git
  or:  use-clang-format.sh files FILES...
  or:  use-clang-format.sh raw ARGUMENTS...
In the 1st form, format all files that have changed since last git commit.
In the 2nd form, format all FILES, treating directories recursively.
In the 3rd form, run \`clang-format --style=<style> ARGUMENTS...\`.

Environment variables:
  CLANG_FORMAT       clang-format executable
  CLANG_FORMAT_DIFF  clang-format-diff script"

rsrc="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$rsrc/../bashlib.sh"

case "$1" in
    git )
        find_cmd CLANG_FORMAT_DIFF \
            clang-format-diff-13 \
            clang-format-diff \
            clang-format-diff.py
        ;;
    files | raw )
        find_cmd CLANG_FORMAT \
            clang-format-13 \
            clang-format
        ;;
    -h | --help | '' )
        echo "$usage"
        exit
        ;;
    * )
        fail "Unknown option '$1'"
        ;;
esac

# Generate style argument
style=''
while IFS='' read line; do
    [ -z "$line" ] && continue
    [ "${line:0:1}" = '#' ] && continue
    [ -n "$style" ] && style+=', '
    style+="$line"
done < "$rsrc/clang-format.yml"
style="{$style}" # Not typo

case "$1" in
    git )
        unstaged_changes="`git diff --name-only`"
        if [ -n "$unstaged_changes" ]; then
            fail "Refusing to operate in git repository with unstaged changes:
$unstaged_changes"
        fi

        git diff -U0 --no-color --relative HEAD \
            '*.cpp' \
            '*.h' \
            '*.inl' \
        | command "$CLANG_FORMAT_DIFF" -p1 -style="$style" -i --verbose
        exit_code="$?"
        git add "$root_dir"
        exit "$exit_code"
        ;;

    raw )
        command "$CLANG_FORMAT" -style="$style" "$@"
        ;;

    files )
        files=()

        for input in "${@:2}"; do
            if [ -d "$input" ]; then
                readarray -d '' current_files < <(
                    find "$input" \
                        \( -name '*.cpp' -o -name '*.h' -o -name '*.inl' \) \
                        -type f \
                        -print0 \
                )

                [ "${#current_files[@]}" -eq 0 ] \
                    && fail "No suitable files found in directory $input"

                files+=("${current_files[@]}")
            else
                case "$input" in
                    *.cpp | *.h | *.inl )
                        files+=("$input")
                        ;;
                    * )
                        error "Refusing to format file '$input': `
                            `only .cpp, .h and .inl supported"
                        ;;
                esac
            fi
        done

        [ "${#files[@]}" -eq 0 ] && fail "No files to format"

        command "$CLANG_FORMAT" -style="$style" -i --verbose "${files[@]}"
        ;;
esac
