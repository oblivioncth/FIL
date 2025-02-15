Ensure Qt6 is installed and locatable by CMake (or alternatively use the `qt-cmake` script that comes with Qt in-place of the`cmake` command).

Right now, a static build is required in order for CLIFp to work correctly.

Should work with MSVC, MINGW64, clang, and gcc.

```
# Acquire source
git clone https://github.com/oblivioncth/FIL

# Configure (ninja optional, but recommended)
cmake -S FIL -B build-FIL -G "Ninja Multi-config"

# Build
cmake --build build-FIL

# Install
cmake --install build-FIL

# Run
cd "build-FIL/out/install/bin"
fil
```