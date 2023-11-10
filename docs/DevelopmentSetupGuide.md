# Development setup guide

This document  provides instructions for  setting up a development  environment
for Progressia.
See also
    [Building Guide](BuildingGuide.md)
and
    [IDE setup guides](ide_setup).

To make development easier, contributors  should be using a few tools. Included
with the project are configurations and scripts for these tools:
  - [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)  –  performs  static
      code analysis for C++
  - [clang-format](https://clang.llvm.org/docs/ClangFormat.html) – formats  C++
      source code
  - Vulkan validation layers – checks for errors in Vulkan API usage at runtime
      (see below for details)
  - [memcheck](https://valgrind.org/docs/manual/mc-manual.html)
      (part of  [valgrind](https://valgrind.org/))  –  performs runtime  memory
      error detection on Linux

Additionally, a git pre-commit hook prevents  committing code that is formatted
incorrectly, does not compile or produces warnings.

## Basic setup
Follow [Building Guide](BuildingGuide.md) instructions before proceeding.

Debian/Ubuntu:
```bash
# Install clang-tidy and clang-format-diff
sudo apt update && sudo apt install -y \
    clang-tidy clang-format

# Enable DEV_MODE (sets up git pre-commit hook)
cmake -S . -B build -DDEV_MODE=ON
```

Fedora:
```bash
# Install clang-tidy and clang-format-diff
sudo dnf install -y \
    clang-tools-extra clang

# Enable DEV_MODE (sets up git pre-commit hook)
cmake -S . -B build -DDEV_MODE=ON
```

Windows: _see [IDE setup guides](ide_setup)._

## Pre-commit git hook

A
[git pre-commit hook](https://git-scm.com/book/en/v2/Customizing-Git-Git-Hooks)
is installed to  correct formatting  and check for  compilation/linting issues.
This check can be bypassed with
  `git commit --no-verify`
in case of dire need.

The hook runs `tools/pre-commit.py`, which formats  modified files  and ensures
that `cmake --build build` executes without  errors. Several git operations are
performed by `pre-commit.py`; run
  `tools/pre-commit.py restore`
to restore the state of the repository in case the hook crashes.

The list of directories with source code  to format and the list of source code
filename extensions are hard-coded into the Python script.

## Vulkan validation layers

LunarG validation layers are extremely useful when debugging Vulkan code.
The official
[Vulkan tutorial](https://vulkan-tutorial.com/Development_environment)
has detailed instructions for all platforms.

Use CMake  option  `VULKAN_ERROR_CHECKING`  to  enable the  use  of  validation
layers.

Debian/Ubuntu users can install this dependency using APT:
```bash
apt install vulkan-validationlayers-dev
```

Fedora users can install this dependency using dnf:
```bash
dnf install vulkan-validation-layers-devel
```

Windows users can install this dependency when installing LunarG distribution.

## memcheck

`tools/valgrind-memcheck-suppressions.supp` contains useful suppressions for
memcheck.
