# 					Main makefile for MuOs
# To compile the os, run make in the parent directory
# This file contains environment configuration for the system submodules

# Host architecture, actually target architecture
TARGETARCH=i386


PROJECT_MAINDIR=$(shell pwd)

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

.PHONY: all MuOS clean

all: MuOS

MuOS:
	$(MAKE) -C libc/ all
	$(MAKE) -C kernel/ all
	
clean:
	$(MAKE) -C libc/ clean
	$(MAKE) -C kernel/ clean