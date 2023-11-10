# Building Guide

This document  provides instructions for building Progressia  from source code.
See also
    [Development Setup Guide](DevelopmentSetupGuide.md)
and
    [IDE setup guides](ide_setup).

MacOS targets are not supported at this moment.

## Short version
Debian/Ubuntu:
```bash
# Install GCC, CMake, Python 3, glslc, git, Vulkan, GLFW and GLM
sudo apt update && sudo apt install -y \
    g++ cmake python3 glslc git libvulkan-dev libglfw3-dev libglm-dev

# Clone project
git clone https://github.com/Wind-Corporation/Progressia.git
cd Progressia

# Generate build files for release (MY-1 is the build ID)
cmake -S . -B build -DBUILD_ID=MY-1 -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Run
build/progressia
```

Fedora:
```bash
# Install GCC, CMake, Python 3, glslc, git, Vulkan, GLFW and GLM
sudo dnf install -y \
    gcc-c++ cmake python3 glslc git vulkan-devel glfw-devel glm-devel

# Clone project
git clone https://github.com/Wind-Corporation/Progressia.git
cd Progressia

# Generate build files for release (MY-1 is the build ID)
cmake -S . -B build -DBUILD_ID=MY-1 -DCMAKE_BUILD_TYPE=Release

# Compile
cmake --build build

# Run
build/progressia
```

Windows: _see [IDE setup guides](ide_setup)._

## Prerequisites

### C++ compiler

Project explicitly fully  supports GCC, MinGW and Clang.  Compilation with MSVC
is also  supported, but  it can't  be used for  release builds  and its  use is
generally discouraged.

On Windows,
    [w64devkit](https://github.com/skeeto/w64devkit/releases)
distribution of MinGW was tested.

Cross-compilation  from  Linux to Windows  is also  explicitly  supported  with
MinGW-w64 as provided by Debian.

### CMake

[CMake](https://cmake.org/) version 3.12 or higher is required.

### Python 3

[Python 3](https://www.python.org/downloads/) is required.

### Vulkan

The following Vulkan components are strictly necessary for builds:
  - Header files
  - Loader static library (`vulkan-1.lib`)
  - `glslc` ([standalone downloads](https://github.com/google/shaderc/blob/main/downloads.md))

However, it is usually easier to install a complete  Vulkan SDK. An open-source
Vulkan SDK can be downloaded from
    [LunarG](https://www.lunarg.com/vulkan-sdk/)
for all platforms.

Debian/Ubuntu users can install this dependency using APT:
```bash
apt install libvulkan-dev glslc
```

Fedora users can install this dependency using dnf:
```bash
dnf install vulkan-devel glslc
```

Windows users using vcpkg should install  the LunarG distribution, then install
the `vulkan` vcpkg package:
```cmd
vcpkg install vulkan:x64-mingw-static
```

### Other libraries

The following libraries are additionally required:
  - [GLFW](https://www.glfw.org/download.html) version 3.3.2 or higher
  - [GLM](https://glm.g-truc.net/)

Debian/Ubuntu users can install these dependencies using APT:
```bash
apt install libglfw3-dev libglm-dev
```

Fedora users can install these dependencies using dnf:
```bash
dnf install glfw-devel glm-devel
```

Windows users can install these dependencies using vcpkg:
```cmd
vcpkg install glfw3:x64-mingw-static glm:x64-mingw-static
```

## Downloading source code

Clone this git repository.

Command line users: run
```bash
git clone <clone url>
```

## Building

### CMake

Use CMake to generate build files. There are a few options available:
  - **`BUILD_ID`**  enables release  builds and specifies visible  unique build
    identifier string.
  - `DEV_MODE`, `VULKAN_ERROR_CHECKING`:
    see [Development Setup Guide](DevelopmentSetupGuide.md).
  - `VULKAN_ERROR_CHECKING` enables Vulkan debug features. This requires Vulkan
    validation    layers   (available   as   part   of   LunarG   Vulkan   SDK,
    `vulkan-validationlayers-dev`  Debian  package  and  `vulkan-devel`  Fedora
    package).

Directory `build` in project root is ignored by git for convenience.

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
> Use proper  build IDs if  distribution is expected. Convention  is two-letter
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

Directory `run` in  project  root is  ignored  by git  for  convenience;  using
project root as working directory is safe for debug builds.
