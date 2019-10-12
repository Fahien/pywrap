# Pywrap

Pywrap is a simple tool which implements an LLVM FrontendAction to create [Pyspot](https://github.com/Fahien/pyspot)-based Python bindings for C++ classes, structs, and enums marked with `__attribute__( ( annotate( "pyspot" ) ) )`.

```cpp
#define PYSPOT_EXPORT __attribute__( ( annotate( "pyspot" ) ) )

enum class PYSPOT_EXPORT Color { R, G, B };

// Will export the public interface only
class PYSPOT_EXPORT Test { /* ... */ };
```

The following guide is very much based on [Clang's](https://clang.llvm.org/get_started.html).

## Prerequisites

In order to build Pywrap, you need to fulfill [the requirements](https://llvm.org/docs/GettingStarted.html#requirements), and some tools already installed:
 - [Git](https://git-scm.com/downloads), for versioning;
 - [Python](http://www.python.org/download), because;
 - [CMake](http://www.cmake.org/download), for building.

Then you can proceed to clone [LLVM-project](https://github.com/llvm/llvm-project), and pywrap under the `clang-tools-extra` directory.

```bash
# LLVM
git clone -b release_90 https://github.com/llvm/llvm-project.git

# Pywrap
cd llvm-project/clang-tools-extra
git clone https://github.com/Fahien/pywrap.git pywrap

# Go up
cd ..
```

## Build

Modify `clang-tools-extra/CMakeLists.txt` by adding the following line:

```cmake
add_subdirectory(pywrap)
```

Generate the project with cmake:

```bash
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS=clang;clang-tools-extra
```

Compile it.

```bash
cmake --build build --config Release
```

Run the executable.

```sh
build\bin\pywrap.exe
```

## Command line arguments

Pywrap expects a list of source files as arguments. If it encounters the `--` separator, it passes the subsequent arguments to the compiler.

```bash
pywrap.exe foo.cpp bar.cpp -- -Iinclude -DANSWER=42 -xc++ -std=c++14
```

This will generate two headers and two source files under the current working directory:

- `include/pyspot/Bindings.h`, containing declarations of Python bindings;
- `include/pyspot/Extension.h`, containing declarations of the [Python module](https://docs.python.org/3/extending/building.html);
- `src/pyspot/Bindings.cpp`, definitions of the bindings;
- `src/pyspot/Extension.cpp`, definitions of the module.

## License

Mit License © 2018-2019 [Antonio Caggiano](https://twitter.com/Fahien)
