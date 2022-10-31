#!/bin/bash

usage=\
"Usage: build.sh [OPTIONS...]
Build and run the game.

Options:
      --debug            make a debug build (default)
      --release          make a release build
      --build-id=ID      set the build ID. Default is dev.
      --cmake-gen=ARGS   pass additional arguments to pass to cmake when
                           generating build files. ARGS is the ;-separated list.
      --dont-generate    don't generate build instructions; use existing
                           configuration if building
      --dont-build       don't build; run existing binaries or generate build
                           instructions only
      --debug-vulkan     enable Vulkan validation layers from LunarG
  -R, --run              run the game after building
      --memcheck[=ARGS]  run the game using valgrind's memcheck dynamic memory
                           analysis tool. Implies -R. ARGS is the ;-separated
                           list of arguments to pass to valgrind/memcheck.

  -h, --help  display this help and exit

Environment variables:
  PARALLELISM  threads to use, default is 1

  CMAKE     cmake executable
  VALGRIND  valgrind executable

private.sh variables:
  private_cmake_gen_args  array of additional arguments to pass to cmake when
                            generating build files

See also: tools/cppcheck/use-cppcheck.sh --help
          tools/clang-format/use-clang-format.sh --help
          tools/setup.sh --help"

rsrc="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
source "$rsrc/bashlib.sh"



# Parse arguments

build_type=Debug
do_generate=true
cmake_gen_args=()
do_build=true

run_type=Normal
do_run=''

debug_vulkan=''
memcheck_args=()

for arg in "$@"; do
    case "$arg" in
        -h | --help )
            echo "$usage"
            exit
            ;;
        --debug )
            build_type=Debug
            ;;
        --release )
            build_type=Release
            ;;
        --build-id )
            fail "Option --build-id=ID requires a parameter"
            ;;
        --build-id=* )
            build_id="${arg#*=}"
            ;;
        --cmake-gen )
            fail "Option --cmake-gen=ARGS requires a parameter"
            ;;
        --cmake-gen=* )
            readarray -t -d ';' new_cmake_gen_args <<<"${arg#*=};"
            unset new_cmake_gen_args[-1]
            cmake_gen_args+=("${new_cmake_gen_args[@]}")
            unset new_cmake_gen_args
            ;;
        --debug-vulkan )
            debug_vulkan=true
            ;;
        -R | --run )
            do_run=true
            ;;
        --memcheck )
            do_run=true
            run_type=memcheck
            ;;
        --memcheck=* )
            do_run=true
            run_type=memcheck
            readarray -t -d ';' new_memcheck_args <<<"${arg#*=};"
            unset new_memcheck_args[-1]
            memcheck_args+=("${new_memcheck_args[@]}")
            unset new_memcheck_args
            ;;
        --dont-generate )
            do_generate=''
            ;;
        --dont-build )
            do_build=''
            ;;
        * )
            fail "Unknown option '$arg'"
            ;;
    esac
done

if [ -z "$do_build" -a -z "$do_generate" -a ${#cmake_gen_args[@]} != 0 ]; then
    fail "CMake arguments are set, but no build is requested. Aborting"
fi

if [ -z "$do_build" -a -z "$do_generate" -a -z "$do_run" ]; then
    fail "Nothing to do"
fi



# Generate build files

find_cmd CMAKE cmake
if [ $do_generate ]; then

    cmake_gen_managed_args=(
        -DCMAKE_BUILD_TYPE=$build_type
        -DVULKAN_ERROR_CHECKING=`[ $debug_vulkan ] && echo ON || echo OFF`
        -UBUILD_ID
    )

    [ -n "${build_id+x}" ] && cmake_gen_managed_args+=(
        -DBUILD_ID="$build_id"
    )

    echo_and_run "$CMAKE" \
        -B "$build_dir" \
        -S "$source_dir" \
        "${cmake_gen_managed_args[@]}" \
        "${private_cmake_gen_args[@]}" \
        "${cmake_gen_args[@]}" \
    || fail "Could not generate build files"
fi



# Build

find_cmd CMAKE cmake
if [ $do_build ]; then
    options=()

    [ -n "${PARALLELISM+x}" ] && options+=(-j "$PARALLELISM")

    echo_and_run "$CMAKE" \
        --build "$build_dir" \
        "${options[@]}" \
    || fail "Build failed"

    unset options
fi



# Run

if [ $do_run ]; then

    run_command=()

    if [ $run_type == memcheck ]; then
        find_cmd VALGRIND valgrind

        run_command+=(
            "$VALGRIND"
            --tool=memcheck
            --suppressions="$tools_dir"/memcheck/suppressions.supp
            "${memcheck_args[@]}"
            --
        )
    fi

    run_command+=(
        "$build_dir/progressia"
    )

    run_dir="$root_dir/run"
    mkdir -p "$run_dir"

    (
        cd "$run_dir"
        echo_and_run "${run_command[@]}"
        echo "Process exited with code $?"
    )

fi
