#!/bin/false

# Writes a message to stderr.
# Parameters:
#   $@ - the message to display
function error() {
    echo >&2 "`basename "$0"`: $@"
}

# Writes a message to stderr and exits with code 1.
# Parameters:
#   $@ - the message to display
function fail() {
    error "$@"
    exit 1;
}

# Ensures that a variable with name $1 has a valid executable. If it does not,
# this function attempts to find an executable with a name suggested in $2...$n.
# In either way, if the variable does not end up naming an executable, fail() is
# called.
# Parameters:
#   $1             - name of the variable to check and modify
#   $2...$n        - suggested executables (at least one)
#   $FAIL_SILENTLY - if set, don't call exit and don't print anything on failure
function find_cmd() {
    declare -n target="$1"

    if [ -z "${target+x}" ]; then
        for candidate in "${@:2}"; do
            if command -v "$candidate" >/dev/null; then
                target="$candidate"
                break
            fi
        done
    fi

    if ! command -v "$target" >/dev/null; then
        [ -n "${FAIL_SILENTLY+x}" ] && return 1
        fail "Command $2 is not available. Check \$PATH or set \$$1."
    fi

    unset -n target
    return 0
}

# Displays the command and then runs it.
# Parameters:
#   $@ - the command to run
function echo_and_run() {
    echo " > $*"
    command "$@"
}

root_dir="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
source_dir="$root_dir"
build_dir="$root_dir/build"
tools_dir="$root_dir/tools"

# Load private.sh
private_sh="$tools_dir/private.sh"
if [ -f "$private_sh" ]; then
    [ -x "$private_sh" ] \
        || fail 'tools/private.sh exists but it is not executable'
    source "$private_sh"
fi
