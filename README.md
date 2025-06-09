# IRHash

This is the source for the IRHash tool described in “IRHash: Efficient Multi-Language Compiler Caching by IR-Level”.

IRHash is a compiler cache which hashes LLVM's IR to reduce compilation time.

## Project Structure

More detailed documentation is located in the corresponding directories. For a conceptual overview, please consult the paper.
This project contains three main subdirectories:

- `pass/`: The implementation of IRHash (see [README](pass/README.md) for more information).
- `example/`: An playground for testing IRHash (see [README](example/README.md)).

## Usage

To use IRHash in C and C++ projects, build it with LLVM 18, set the desired cache directory, and add it as a Clang and LLVM pass plugin:

```shell
# building (see pass/ for more info)
cd pass
make LLVM-CONFIG=llvm-config-18 pass-skip.so
# set the desired cache directory
export IRHASH_CACHE=/path/to/cache
mkdir "$IRHASH_CACHE"
# Configure Clang and set the desired flags
# Note: this might vary depending on the build system
export CC=clang-18
export CXX=clang++-18
export CFLAGS="-fplugin=path/to/pass-skip.so -fpass-plugin=path/to/pass-skip.so"
export CXXFLAGS="$CFLAGS"
# Build the project
cd path/to/project
make
```

For other languages, build `pass-no-plugin-skip.so` (this does not depend on Clang) and load it as an LLVM pass plugin.
To ensure the plugin is actually loaded, you can use the `pass-no-plugin-debug.so` which prints debug messages to stderr.
