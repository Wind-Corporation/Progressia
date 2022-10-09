#!/bin/bash

me="$(realpath "${BASH_SOURCE[0]}")"
if [ "$(basename "$me")" = 'pre-commit' ]; then
    # i write good shell scripts - Javapony 2022-10-07
    root_dir="$(realpath "$(dirname "$me")/../../")"

    hook_source="$root_dir/tools/git/hook_pre_commit.sh"
    if [ "$hook_source" -nt "$me" ]; then
        if [ -n "${ALREADY_UPDATED+x}" ]; then
            echo >&2 "git pre-commit hook: Attempted recursive hook update. `
                `Something is very wrong."
            exit 1
        fi

        echo ''
        echo "===== tools/git/hook_pre_commit.sh updated; `
            `replacing pre-commit hook ====="
        echo ''

        cp "$hook_source" "$me" &&
        chmod +x "$me" \
        || fail 'Update failed'

        ALREADY_UPDATED=true "$me"
        exit $?
    fi

    source "$root_dir/tools/bashlib.sh"
else
    rsrc="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
    source "$rsrc/../bashlib.sh"
fi

unstaged_changes="`git diff --name-only`"
if [ -n "$unstaged_changes" ]; then
    fail "Please stage all stash all unstaged changes in the following files:
$unstaged_changes"
fi

echo_and_run "$tools_dir/cppcheck/use-cppcheck.sh" \
    || fail "Cppcheck has generated warnings, aborting commit"

echo_and_run "$tools_dir/clang-format/use-clang-format.sh" git \
    || fail "clang-format has failed, aborting commit"

echo_and_run "$tools_dir/build.sh" --dont-generate \
    || fail "Could not build project, aborting commit"

echo 'All checks passed'

