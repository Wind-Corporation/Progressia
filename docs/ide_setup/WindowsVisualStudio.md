# IDE setup guide: Windows / Visual Studio

This  document  is  an  IDE  setup  guide for  Visual  Studio  with  MinGW, the
recommended compiler for Windows.

Compilation with  MSVC and clang-cl is supported; however, these  compilers may
generate warnings. Additionally, release  builds compiled with MSVC or clang-cl
are strongly discouraged, see [Building Guide](../BuildingGuide.md).

## Installing Visual Studio

This guide was  tested with Visual Studio 2022. "Desktop  Development with C++"
workload is  required to work with C++ projects. Launch  Visual Studio at least
once with this configuration and close it for the next steps.

## Installing build tools

### Python 3
Install Python 3 (available from
[the official website](https://www.python.org/downloads/)
and Microsoft Store). Make sure `python` or `python3` is available in PATH:
```cmd
:: This command must work in a fresh CMD:
python3 --version
```
Note that  if running  this command  launches Microsoft  Store, Python was  not
installed correctly.

### MinGW
Install MinGW. There are many distributions  of MinGW available; this guide was
tested with [w64devkit](https://github.com/skeeto/w64devkit).

To install w64devkit, go to the
[Releases](https://github.com/skeeto/w64devkit/releases)
section of the official  repository. Download  the `w64devkit-XXX.zip` file and
extract it into `C:\msys64\mingw64\`. If extracted correctly,
  `C:\msys64\mingw64\bin\gcc.exe`
should exist. Directory
  `C:\msys64\mingw64\bin\`
should be added to system PATH
([instructions for Windows 10](https://stackoverflow.com/a/44272417/4463352)).
Proper installation can be verified like so:
```cmd
:: This command must work in a fresh CMD:
gcc --version
```

## Installing libraries

Several third party libraries are used by the project. With Windows, installing
them manually can be a hassle, so the developers recommend using vcpkg.

A Vulkan  SDK has  to be installed before  vcpkg can  install `vulkan` package.
[LunarG](https://www.lunarg.com/vulkan-sdk/)   distribution   is   recommended:
download  and run the SDK  installer. "Validation layer"  errors are common  on
Windows and can usually be safely  ignored; they are typically caused by third-
party software such as GPU drivers, OBS or Steam.

To install vcpkg, go to the
[Releases](https://github.com/microsoft/vcpkg/releases) section of the official
repository. Download and extract  "Source code" ZIP file to a directory of your
choice. Run the following commands inside the resulting folder:
```cmd
:: Perform initial setup
bootstrap-vcpkg

:: Setup Visual Studio integration
vcpkg integrate install

:: Install libraries
vcpkg install vulkan:x64-mingw-static glfw3:x64-mingw-static glm:x64-mingw-static
```

## Project setup

Start Visual Studio. Use "Clone a Repository"  to download sources and create a
project.  Select   the  project  in  Solution  Explorer  and   wait  for  CMake
initialization to complete.

Next, click on "x64-Debug" in the  toolbar. Click on "Manage Configurations..."
to open CMake Settings. Use the plus  button to add a new configuration; select
"Mingw64-Debug"  when  prompted. Select  the  new  configuration  and  add  the
following parameter to "CMake command arguments":
```
-DVCPKG_TARGET_TRIPLET=x64-mingw-static
```

Remove "x64-Debug" configuration by selecting it and pressing the cross button.

Finally click "▶ Select startup item" in the toolbar and choose progressia.exe.

## Developer setup

To enable features useful for  developers, set CMake option `DEV_MODE` to `ON`.
See [Development Setup Guide](../DevelopmentSetupGuide.md) for more details.

TODO: _include step-by-step instructions for this section._
