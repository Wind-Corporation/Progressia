# Building guide

At this time, building  is only supported in GNU/Linux  targeting GNU/Linux with
X11/Wayland and Windows (cross-compilation). See also
[Development Setup Guide](DevelopmentSetupGuide.md)
if you want to make git commits.

## Prerequisites

Install the following software:
  - a C++ compiler (GCC or clang preferably),
  - CMake,
  - Python 3,
  - glslc.

Install the following libraries with headers:
  - Vulkan (loader library and headers),
  - GLFW3,
  - GLM,
  - Boost (only core library required).

### Debian

On Debian, you  can run  the following  commands  as root  to install almost all
required software:
```bash
apt-get install \
    g++ \
    cmake \
    python3 &&
apt-get install --no-install-recommends \
    libvulkan-dev \
    libglfw3-dev \
    libglm-dev \
    libboost-dev
```

However, glslc, the shader compiler, is not available as a Debian package at the
moment. You can install it manually from official sources or use the download it
from windcorp.ru by running these commands as root:
```bash
apt-get install wget &&
mkdir -p /opt/glslc &&
wget --output-file=/opt/glslc/glslc \
     'https://windcorp.ru/other/glslc-v2022.1-6-ga0a247d-static' &&
chmod +x /opt/glslc/glslc
```

Alternatively, packages provided by  LunarG are available for Ubuntu. Follow the
instructions   on  [LunarG.com](https://vulkan.lunarg.com/sdk/home)   to install
`vulkan-sdk`.

## Setup

```bash
git clone <clone url>
cd Progressia
chmod +x tools/setup.sh
tools/setup.sh
```

`tools/setup.sh`  will  check  the availability  of  all  required commands  and
libraries.

Build tools  use enviroment  variables `PATH`, `VULKAN_SDK`, `CMAKE_MODULE_PATH`
and `CMAKE_PREFIX_PATH` to  locate the various  components; you  can edit  these
variables  system-wide or use `tools/private.sh` to amend  them for build tools.
(Your changes to `tools/private.sh` are ignored by git.)
For example, of you ran the script to download glslc on Debian, you will need to
add the following line to `tools/private.sh`:
```bash
PATH="$PATH:/opt/glslc"
```

## Building

```bash
tools/build.sh
```

## Running

```bash
tools/build.sh -R
```
