# Development environment setup

Clone the repository:

```bash
git clone https://github.com/Muzuwi/MuOS.git
cd MuOS/
export MUREPO=${PWD}
```

Further in the documentation, any references to `${MUREPO}` shall refer to the root directory of the cloned repository.

# Preparing the toolchain

The project requires a custom toolchain to be built in order to build the kernel/userland.
To do this, you can either use the [pre-built toolchain Docker image](https://github.com/Muzuwi/MuOS/pkgs/container/muos-toolchain), or compile the toolchain yourself.

# Building the toolchain

## Requirements

To compile GCC/binutils, you need to have the following:

- libmpfr
- libmpc

## Procedure

Compiling and installing the toolchain can be done by doing the following (it is assumed that you're in the repository root already):

```bash
cd Toolchain/src/
./download-archives.sh
./make-binutils.sh
./make-gcc.sh
```

This downloads the required GCC/binutils archives, applies the required patches, and then builds/installs the tools into the proper prefix directory.
The prefix directory is by default set to `/usr/local/muOS`.

For now, changing the prefix directory is only possible by modifying the path in all CMakeLists and the build scripts for binutils/GCC, which is a bit of a hassle.
It is recommended to stick to the default path for now, until a better way of customizing it is implemented.

If you encounter problems during the installation phase, make sure that `/usr/local/muOS` is accessible for the build user (for example: by creating that directory and `chmod`'ing it to belong to the current user).

# Development using an IDE

## VSCode

Open the repository folder in VSCode, and you should get autocompletion automatically.
This requires the following extensions:
- clangd
- CMake

```{note}
When using VSCode for development, the workspace configuration assumes the prefix directory `/usr/local/muOS` contains the toolchain.
```

The CMake configuration for the build utilizes globs, so if something isn't behaving correctly, make sure you perform a `CMake Clean` followed by a `CMake Configure` to pick up new source files.
