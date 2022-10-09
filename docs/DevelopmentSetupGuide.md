# Development setup guide

To make development  easier, contributors should be using a few  tools. Included
with the project are configurations and scripts for these tools:
  - [cppcheck](http://cppcheck.net/) – performs static code analysis for C++
  - [clang-format](https://clang.llvm.org/docs/ClangFormat.html) – automatically
      formats C++ source code
  - [memcheck](https://valgrind.org/docs/manual/mc-manual.html)
      (part of  [valgrind](https://valgrind.org/))  –  performs  runtime  memory
      error detection

Additionally, git  hooks prevent committing code that  is formatted incorrectly,
does  not  compile  or  produces  warnings. You  can  bypass  this  check  using
  `git commit --no-verify`
in case of dire need.

## Prerequisites

Perform the setup described in the [Building Guide](BuildingGuide.md) first.

Install the following software:
  - cppcheck,
  - clang-format (version 13 is recommended)
  - valgrind

### Debian

On Debian, you can run the following commands as root to install all required
software:
```bash
apt-get install \
    cppcheck \
    clang-format-13 \
    valgrind
```

## Setup

```bash
tools/setup.sh --for-development
```

With `--for-development` flag, `tools/setup.sh` will check the development tools
and install git pre-commit hook in addition to its normal duties.

## Notes

Developers will find it useful to read through the following help pages:
```bash
tools/build.sh --help
tools/cppcheck/use-cppcheck.sh --help
tools/clang-format/use-clang-format.sh --help
```

LunarG validation layers are extremely useful when writing and debugging Vulkan.
The official
[Vulkan tutorial](https://vulkan-tutorial.com/Development_environment)
has detailed instructions for all platforms.

In particular, Debian users can run the following command as root:
```bash
apt-get install vulkan-validationlayers-dev
```
