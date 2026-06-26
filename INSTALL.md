# Installing Mojibake

Most projects do not need to build Mojibake from this repository. For application or library use,
the amalgamation is enough. See [README.md](README.md) to see how to do it.

## Building from source

Build from source only if you are developing Mojibake itself, running the test suite, changing the
generated Unicode tables, building the CLI, or producing WebAssembly/release artifacts.

### Requirements

On POSIX systems: macOS, Linux, FreeBSD, OpenBSD

- `CMake`
- `Node.js` and `npm`
- `make` (usually already included in the release)
- `unzip` (usually already included in the release)

On Windows 10/11:

- The MSVC compiler. [Build Tools per Visual Studio](https://visualstudio.microsoft.com/it/downloads/#build-tools-for-visual-studio-2026) is enough
- `Node.js` and `npm`

### POSIX

At the very first run:

```sh
cd utils/generate
npm i
```

Then from root folder:

```sh
make
make test
```

### Windows

At the very first run, from a Visual Studio developer command prompt:

```powershell
cd .\utils\generate
npm i
```

Then from root folder:

```powershell
nmake /F Makefile.nmake
nmake /F Makefile.nmake test
```

## WebAssembly

Build the WASM module and generated site assets with:

```sh
make wasm
```

This writes the current Emscripten output to `build-wasm/src/` and refreshes the generated API
artifacts in `src/api/`.

You can run a local server with:

```sh
make watch-site
# Open http://localhost:6251
```

Run the JavaScript API server with:

```sh
make watch-api
```

## Useful targets

1. `amalgamation` generate the single-file amalgamation
2. `generate` regenerate the source files
3. `wasm` generate the WASM code

See the make help target for all the available targets.
