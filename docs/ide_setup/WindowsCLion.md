# IDE setup guide: Windows / CLion

> **Note**
>
> This  guide has not been  tested sufficiently  because currently  none of the
> developers use CLion to develop  Progressia. Please let us know if this guide
> requires corrections or updates.

This  document  is an  IDE setup guide  for CLion  with MinGW, the  recommended
compiler for Windows.

Compilation with  MSVC and clang-cl is supported; however, these  compilers may
generate warnings. Additionally, release  builds compiled with MSVC or clang-cl
are strongly discouraged, see [Building Guide](../BuildingGuide.md).

## Installing CLion

Install CLion as usual. Close CLion for the following steps.

> **Note**
>
> Native vcpkg support has  been added to CLion in version  2023.1. At the time
> of writing this is a recent update. Make sure you use the latest version.
>
> Workaround for older versions: add
> `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`
> to CMake options.

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

Uhm...  how do I put it... I  could not  get my hands  on a Windows  install of
CLion in a reasonable  time and so I will have to leave  this blank for now. If
you have  CLion on Windows,  please contact  the devs so  we can  do the  setup
together and this doc can be completed.

In general, from this point you should clone the  git repo and open the project
as a CMake project.

## Developer setup

To enable features useful for  developers, set CMake option `DEV_MODE` to `ON`.
See [Development Setup Guide](../DevelopmentSetupGuide.md) for more details.
