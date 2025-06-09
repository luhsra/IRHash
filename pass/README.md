# `pass`

This directory contains the implementation of the IRHash plugin.
The main LLVM pass can be found in `pass.cpp`.

## Building

Currently, IRHash only works on Linux.
We provide a Docker container in `example/` which builds IRHash on Ubuntu.

**Requirements**

- `make`
- LLVM, libllvm, Clang, and libclang 18 (Debian and Ubuntu: `clang-18 libclang-18-dev llvm-18-dev`)
- libxxhash (Debian and Ubuntu: `libxxhash-dev`)

To tell `make` about LLVM 18 (if it's not the default), set `LLVM-CONFIG=llvm-config-18`.

The main pass plugin can be built with `make pass-skip.so`.
Make provides the following targets:

- `pass-skip.so`: The main LLVM pass plugin which stops the compilation if object files are used from the cache. It also includes a Clang plugin to gracefully stop compilation.
- `pass-no-plugin-skip.so`: Same as `pass-skip.so` but without a Clang plugin.
- `pass-debug.so`: The plugin with additional debug logging.
- `pass0.so`: Validation pass (used for the evaluation). This outputs a `.llvmhash` which includes timestamps about the current compilation and the hashing. This doesn't stop the compilation after computing the hash and finding an object file. `time.so` must be `LD_PRELOADED` for this to work correctly.
- `pass0-plugin.so`: Same as `pass0.so` except that this will also act as a Clang plugin.
- `time.so`: A dynamic library used during evaluation to time the compilation.
