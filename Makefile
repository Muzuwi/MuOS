# 					Main makefile for MuOs
# To compile the os, run make in the parent directory
# This file contains environment configuration for the system submodules

# Host architecture, actually target architecture
TARGETARCH=i386

# Main directory of the project
PROJECT_MAINDIR=$(shell pwd)

# Output object directory
OBJDIR=$(PROJECT_MAINDIR)/obj

PREFIX=/usr/local/muOS
AR=/usr/bin/i686-elf-ar
AS=/usr/bin/i686-elf-as
CC=/usr/bin/i686-elf-gcc
GPP=/usr/bin/i686-elf-g++
NASM?=nasm

# TODO: standard library stuff

# Compiler flags
CFLAGS=-O2 -g
CPPFLAGS=
NASMFLAGS=-felf32

export HOST_ARCH
export PREFIX
export AR
export AS
export CC
export GPP
export NASM
export CFLAGS
export CPPFLAGS
export NASMFLAGS
export PROJECT_MAINDIR
export OBJDIR

.PHONY: all MuOS clean install bootable

all: MuOS bootable

MuOS:
	@echo "Building LibC.."
	$(MAKE) -C LibC/ all
	@echo "Building uKernel.."
	$(MAKE) -C Kernel/ all

clean:
	@echo -e "Cleaning up.."
	@$(MAKE) -C LibC/ clean
	@$(MAKE) -C Kernel/ clean

install:
	@echo -e "Installing MuOS binary to /boot/"
	@sudo cp -f Kernel/muOsKernel.bin /boot/

bootable: MuOS
	@echo "Making bootable image muOS.img"
	@cp -f Kernel/muOsKernel.bin isodir/boot/
	@grub-mkrescue -o muOS.iso isodir/
