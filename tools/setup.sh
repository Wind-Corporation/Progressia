#!/bin/bash

usage=\
"Usage: setup.sh [--for-development]
Set up the development environment after \`git clone\`

Options:
      --for-development  perform additional setup only necessary for developers

  -h, --help  display this help and exit"

rsrc="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$rsrc/bashlib.sh" || {
    echo >&2 'Could not load bashlib'
    exit 1
}
cd "$root_dir"



# Parse arguments

for_development=''

for arg in "$@"; do
    case "$arg" in
        -h | --help )
            echo "$usage"
            exit
            ;;
        --for-development )
            for_development=true
            ;;
        * )
            fail "Unknown option '$arg'"
            ;;
    esac
done



# Сreate private.sh

if [ ! -e "$private_sh" ]; then
    echo '#!/bin/bash

# This file is ignored by git. Use it to configure shell scripts in tools/
# for your development environment.

PARALLELISM=1
#PATH="$PATH:/opt/whatever"
' >"$private_sh" &&
    chmod +x "$private_sh" ||
    fail "tools/private.sh was not found; could not create it"

    echo "Created tools/private.sh"
else
    echo "Found and loaded private.sh"
fi



# Check available commands

failed=()

function check_cmd() {
    if FAIL_SILENTLY=true find_cmd found "$@"; then
        echo "Found command $found"
    else
        failed+=("command $1")
        echo "Could not find command $1"
    fi
    unset found
}

check_cmd pkg-config
check_cmd cmake
check_cmd python3
check_cmd glslc

if [ $for_development ]; then
    check_cmd git
    check_cmd cppcheck
    check_cmd clang-format-13 clang-format
    check_cmd clang-format-diff-13 clang-format-diff clang-format-diff.py
    check_cmd valgrind
fi



# Try generating build files

if FAIL_SILENTLY=true find_cmd found_cmake cmake; then
    if CMAKE="$found_cmake" "$tools_dir/build.sh" --dont-build; then
        echo 'CMake did not encounter any problems'
    else
        echo 'Could not generate build files; libraries are probably missing'
        failed+=('some libraries, probably (see CMake messages for details)')
    fi
else
    echo 'Skipping CMake test because cmake was not found'
fi



# Display accumulated errors

[ ${#failed[@]} -ne 0 ] &&
fail "Could not find the following required commands or libraries:

`for f in "${failed[@]}"; do echo "  $f"; done`

You can resolve these errors in the following ways:
  1. Install required software packages. See README for specific instructions.
  2. Edit PATH, PKG_CONFIG_PATH or CMAKE_MODULE_PATH environment variables in
       tools/private.sh to include your installation directories.
"



# Set executable flags

chmod -v +x tools/build.sh \
            tools/embed/embed.py \
|| fail 'Could not make scripts executable'

if [ $for_development ]; then
    chmod -v +x tools/clang-format/use-clang-format.sh \
                tools/cppcheck/use-cppcheck.sh \
    || fail 'Could not make developer scripts executable'
fi



# Set git hook

if [ $for_development ]; then
    mkdir -vp .git/hooks &&
    cp -v tools/git/hook_pre_commit.sh .git/hooks/pre-commit &&
    chmod -v +x .git/hooks/pre-commit \
    || fail 'Could not setup git pre-commit hook'
fi



echo 'Setup complete'
