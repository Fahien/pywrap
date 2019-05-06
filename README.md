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

Then you can proceed to clone [LLVM](https://github.com/llvm-mirror/llvm), [clang](https://github.com/llvm-mirror/clang), custom [clang-tools-extra](https://github.com/Fahien/clang-tools-extra), and pywrap in the right directories.

```bash
# LLVM
git clone -b release_80 https://github.com/llvm-mirror/llvm.git llvm

# Clang
cd llvm/tools
git clone -b release_80 https://github.com/llvm-mirror/clang.git clang

# Clang extra
cd clang/tools
git clone -b release_80 https://github.com/Fahien/clang-tools-extra.git extra

# Pywrap
cd extra
git clone -b release_80 https://github.com/Fahien/pywrap.git pywrap

# Go back
cd ../../../../..
```

## Build

Generate the project with cmake: `cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_RTTI=ON`

Compile it: `cmake --build build --config Release`

Run the executable: `build\bin\pywrap.exe`

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
