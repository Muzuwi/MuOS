# 					Main makefile for MuOs
# To compile the os, run make in the parent directory

# Main directory of the project
PROJDIR=$(shell pwd)
export PROJDIR

.PHONY: all MuOS clean install bootable LibC Kernel

all: MuOS bootable

MuOS: LibC Kernel

LibC:
	@echo "Building LibC.."
	$(MAKE) -f LibC/Makefile.libc all

Kernel:
	@echo "Building uKernel.."
	$(MAKE) -f Kernel/Makefile.kernel all

clean:
	@echo -e "Cleaning up.."
	$(MAKE) -f LibC/Makefile.libc clean
	$(MAKE) -f Kernel/Makefile.kernel clean

install:
	@echo -e "Installing MuOS binary to /boot/"
	@sudo cp -f Kernel/uKernel.bin /boot/

bootable: MuOS
	@echo "Making bootable image muOS.img"
	@cp -f Kernel/uKernel.bin isodir/boot/
	@grub-mkrescue -o muOS.iso isodir/
