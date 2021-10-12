## MuOS

A work-in-progress OS (currently more kernel than OS) for x86_64.

### Requirements

- Relatively recent x86 CPU

### Compiling

Compile the toolchain:

1. `cd Toolchain/src/` and run script `download-archives.sh`. 
Currently supported versions of GCC/binutils are downloaded, extracted and patches are automatically applied.
2. Ensure prefix location is present. The toolchain is installed into a custom prefix (by default ```/usr/local/muOS```). 
Ensure this location is present and writeable by the current user.  
**Alternatively**, change the prefix to the one you wish to use. However this needs to be done in all CMakeLists and 
the build scripts for binutils/GCC, which is a bit of a hassle.. 
3. Compile binutils - run `make-binutils.sh`
4. Compile GCC - run `make-gcc.sh`

The compile scripts automatically install binutils/GCC to the selected prefix.

Compile the kernel:

1. Make a build directory, `cd build/` and run the compile.sh script.
Currently, the toolchain is set via defines in the cmake command, but 
eventually compiling will be as easy as running `cmake ../` in the build directory. 

