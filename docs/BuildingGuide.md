# Building Guide

This document  provides instructions  for building Progressia  from source code.
See also
    [Development Setup Guide](DevelopmentSetupGuide.md)
and
    [IDE setup guides](TODO).

MacOS targets are not supported at this moment.

## Short version
Debian/Ubuntu:
```bash
# Install GCC, CMake, Python 3, git, Vulkan, GLFW and GLM
sudo apt update && sudo apt install -y \
    g++ cmake python3 git curl libvulkan-dev libglfw3-dev libglm-dev

# Install glslc
sudo mkdir -p /opt/glslc
( cd /opt/glslc
  sudo curl -LO https://windcorp.ru/other/glslc-v2022.1-6-ga0a247d-static &&
  sudo chmod +x glslc* &&
  sudo ln -s glslc* /usr/local/bin/glslc; )

# Clone project
git clone CLONE-URL
cd Progressia

# Generate build files for release
cmake -S . -B build -DBUILD_ID=MY-1 -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Run
build/progressia
```

Fedora:
```bash
# Install GCC, CMake, Python 3, git, Vulkan, GLFW, GLM and glslc
dnf install -y gcc-c++ cmake python3 git vulkan-devel glfw-devel glm-devel glslc

# Clone project
git clone CLONE-URL
cd Progressia

# Generate build files for release
cmake -S . -B build -DBUILD_ID=MY-1 -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Run
build/progressia
```

Windows: _see [IDE setup guides](TODO)._

## Prerequisites

### C++ compiler

Project explicitly fully supports GCC, MinGW and Clang. Compilation with MSVC is
also supported, but it can't be used for release builds and its use is generally
discouraged.

On Windows,
    [w64devkit](https://github.com/skeeto/w64devkit/releases)
distribution of MinGW was tested.

Cross-compilation  from  Linux  to Windows  is also  explicitly  supported  with
MinGW-w64 as provided by Debian.

### CMake

[CMake](https://cmake.org/) version 3.12 or higher is required.

### Python 3

[Python 3](https://www.python.org/downloads/) is required.

### Vulkan

The following Vulkan components are strictly necessary for builds:
  - Header files
  - Loader static library (`vulkan-1.lib`)
  - `glslc` (standalone downloads
        [here](https://github.com/google/shaderc/blob/main/downloads.md))

However, it is usually  easier to install a complete  Vulkan SDK. An open-source
Vulkan SDK can be downloaded from
    [LunarG](https://www.lunarg.com/vulkan-sdk/)
for all platforms.

Debian users can install this dependency using APT:
```bash
apt install libvulkan-dev
```
However, Debian Bullseye repositories
    [do not include](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=890472)
glslc. It  should be installed  manually; see standalone  link above or use this
script:
```bash
sudo mkdir -p /opt/glslc
( cd /opt/glslc
  sudo curl -LO https://windcorp.ru/other/glslc-v2022.1-6-ga0a247d-static &&
  sudo chmod +x glslc* &&
  sudo ln -s glslc* /usr/local/bin/glslc; )
```

Ubuntu dpkg packages are available from LunarG.

Fedora users can install Vulkan using dnf:
```bash
dnf install vulkan-devel glslc
```

### Other libraries

The following libraries are additionally required:
  - [GLFW](https://www.glfw.org/download.html) version 3.3.2 or higher
  - [GLM](https://glm.g-truc.net/)

## Downloading source code

Clone this git repository.

Command line users: run
```bash
git clone <clone url>
```

## Building

### CMake

Use CMake to generate build files. There are a few options available:
  - **`BUILD_ID`**  enables release  builds and  specifies visible  unique build
    identifier string.
  - `DEV_MODE` enables developer features.
    See [Development Setup Guide](DevelopmentSetupGuide.md).
  - `VULKAN_ERROR_CHECKING` enables Vulkan debug features. This requires Vulkan
    validation    layers   (available    as   part   of   LunarG   Vulkan   SDK,
    `vulkan-validationlayers-dev`  Debian   package  and  `vulkan-devel`  Fedora
    package).

Directory `build` in project root is ignored by git for builders' convenience.

This step is usually performed in the IDE.

Command line users: run
```bash
cd /path/to/project
# Routine (debug) build
cmake -S . -B build
# Release build
cmake -S . -B build -DBUILD_ID=MY-1 -DCMAKE_BUILD_TYPE=Release
```

> **Note**
>
> Use proper  build IDs  if distribution  is expected. Convention  is two-letter
> builder identifier, a dash and a unique ascending build number.
>
> For example, automated builds at windcorp.ru use IDs `WA-1`, `WA-2`, etc.

> **Note**
>
> Release builds with MSVC are not supported.
> The standard library used by MSVC poses a problem:
> - it cannot be statically linked with Progressia due to GPL restrictions,
> - it cannot be bundled with Progressia for the same reason,
> - asking the user to install Visual C++ Runtime manually would introduce
>   unnecessary confusion because official builds do not require it.

### Compiling

This step is usually performed in the IDE.

Command line users: run
```bash
cmake --build build
```

## Running

Executable file will be located directly inside the CMake binary directory.

Directory `run`  in project root  is ignored by git  for builders'  convenience;
using project root as working directory is safe for debug builds.
