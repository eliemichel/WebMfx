WebMfx
======

*This project is highly WIP!*

## Dependencies

 - [emscripten](https://emscripten.org/docs/getting_started/downloads.html#sdk-download-and-install)
 - [CMake](https://cmake.org/download/#latest) (version **3.24** or newer)
 - `make` or [`ninja`](https://github.com/ninja-build/ninja/releases/latest)

## Building

You must first [install](https://emscripten.org/docs/getting_started/downloads.html#sdk-download-and-install) and **activate** emscripten using:

```
pushd whereever/emsdk/is/installed
emsdk activate latest
popd
```

Then, this project uses [CMake](https://cmake.org/download/#latest) (version **3.24** or newer), start by moving to your out-of-source build directory, then configure the project:

```
mkdir build
cd build
cmake .. --preset=XXX
```

You can call `cmake --list-presets=all .` to list available presets, but more specific examples are listed in the sections bellow. Once this is done, you can build the project with:

```
cmake --build .
```

Only this last command needs to be ran when modifying the source code.

### Linux/MacOS/WSL

Make sure `make` is installed.

```
cmake .. --preset dev-unix
```

### Windows

When building with emscripten, it makes no sens of using Visual Studio's build system (and it won't work actually), so we need another one. We suggest the use of [Ninja](https://github.com/ninja-build/ninja/releases/latest) because it is the most lightweight solution.

If `ninja.exe` is available in your PATH (type `where ninja` to check), you may simply call:

```
cmake .. --preset dev-windows
```

Otherwise, you must specify to CMake where to find `ninja`:

```
cmake .. --preset dev-windows -DCMAKE_MAKE_PROGRAM="C:/Users/me/Downloads/ninja.exe"
```

**NB** If you have `make` (or `mingw32-make`) installed on your Windows and do not want to download Ninja, you can call

```
cmake .. --preset dev -G "Unix Makefile" -DCMAKE_MAKE_PROGRAM=C:/path/to/mingw32-make.exe
```

## Running

If you have Python available in your PATH, you can use this script to start a dev server and open the program in your web browser:

```
run_dev_server
```
