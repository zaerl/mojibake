# Installing

Most projects do not need to build Mojibake from this repository. For application or library use,
the amalgamation is enough. See [README.md](README.md) to see how to do it.

You can find instructions of how to compile Mojibake in the [CONTRIBUTING.md](CONTRIBUTING.md) file.

To install Mojibake as a CMake package:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

To install a dynamic library, configure with `-DBUILD_SHARED=ON`. Downstream CMake projects can then
use `find_package(Mojibake CONFIG REQUIRED)` and link `Mojibake::mojibake`.
