# Building the kernel

To build the kernel, you must have the proper toolchain already built and installed.
Please refer to the [Development setup](dev_setup.md) chapter for instructions on how to do this.

```{warning}
In order to build a bootable ISO, `grub-mkrescue` is required.
Otherwise, the build will only produce a kernel executable that cannot be directly launched by QEMU/bare-metal!
```

## Building from VSCode

Provided VSCode workspace settings should allow you to simply press the `Build` button on the bottom status bar to build the kernel, as long as the toolchain was built and installed successfully.

## Building the kernel manually

For a reference implementation, you can refer to the [GitHub Action](../../.github/workflows/build-kernel.yml) that performs the build automatically.

### Example build

For example, to build the kernel for `x86_64`, run the following:

```bash
mkdir -p build/ && cd build/
cmake ../ -DCMAKE_MODULE_PATH=$(pwd)/../Toolchain/Modules -DMU_MACHINE=x86_64 -DCMAKE_INSTALL_PREFIX=/usr/local/muOS/
make -j$(nproc)
```

Please note that this assumes the current directory is the MuOS repository (`${MUREPO}`).

### Build configuration

The kernel utilizes CMake as the build system, but additional configuration options are **required** to be passed for a successful build.
These options are:
- `-DCMAKE_MODULE_PATH` - for now, this **must** be set to the absolute path to the  `Toolchain/Modules/` folder of the cloned MuOS repository
- `-DCMAKE_INSTALL_PREFIX` - for now, this **must** be set to `/usr/local/muOS`
- `-DMU_MACHINE` - this defines the architecture to build for. Currently, only `x86_64` is supported.

Some additional options that you can modify:
- `-DCMAKE_BUILD_TYPE=<type>` - where type can be as described [here](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html). Debug builds are better suited for development, but it's also beneficial to test your implementation on other build types, to make sure nothing is broken on higher optimization levels.

Out-of-tree builds are preferred.

### Build output

The kernel executable is placed in `${MUREPO}/Root/uKernel.bin` after the build, regardless of how you build the kernel (VSCode/manual).
Additionally, `${MUREPO}/Root/MuOS.iso` is created, which can be used to test-drive the kernel in QEMU/bare-metal (provided `grub-mkrescue` is present).
