# Makefile for building the system C library and libKernel

# Default arch is x86
TARGETARCH?=i386

# Compiler / linker variables
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(PREFIX)/lib

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -masm=intel
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude
NASMFLAGS?=-felf32
LIBKERNEL_CFLAGS:=$(CFLAGS) -I$(PROJECT_MAINDIR)/kernel/include
LIBKERNEL_CPPFLAGS:=$(CPPFLAGS) -D__is_libkernel

ARCHDIR=arch/$(TARGETARCH)
include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(ARCH_CPPFLAGS)
LIBKERNEL_CFLAGS:=$(LIBKERNEL_CFLAGS) $(KERNEL_ARCH_CFLAGS)
LIBKERNEL_CPPFLAGS:=$(LIBKERNEL_CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)

# Freestanding std lib objects
FREEOBJS=			\
$(ARCH_FREEOBJS)	\
string/strlen.o 	\
stdio/printf.o 		\
stdio/puts.o 		\
stdio/putchar.o 	\

HOSTEDOBJS=				\
$(ARCH_HOSTEDOBJS)		\

# All objects to build
OBJS=			\
$(FREEOBJS)		\
$(HOSTEDOBJS)	\

# List of objects for libkernel
LIBKERNEL_OBJS=$(FREEOBJS:.o=.libkernel.o)
BINARIES=libkernel.a

.PHONY: all clean
.SUFFIXES: .o .libkernel.o .c .S

all: $(BINARIES) install-headers install-libraries

libc.a: $(OBJS)
		$(AR) rcs $@ $(OBJS)

libkernel.a: $(LIBKERNEL_OBJS)
		$(AR) rcs $@ $(LIBKERNEL_OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.c.S:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.c.libkernel.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(LIBKERNEL_CFLAGS) $(LIBKERNEL_CPPFLAGS)

.S.libkernel.o:
	$(CC) -MD -c $< -o $@ $(LIBKERNEL_CFLAGS) $(LIBKERNEL_CPPFLAGS)

install-headers:
	@echo "Copying libC headers into INCLUDEDIR $(INCLUDEDIR)"
	@cp -R --preserve=timestamps "include/." "$(INCLUDEDIR)"

install-libraries:
	@echo "Copying libkernel into LIBDIR $(LIBDIR)"
	@cp "libkernel.a" "$(LIBDIR)"

clean:
	@-rm */*.o 2>/dev/null || true
	@-rm */*.d 2>/dev/null || true
	@-rm */*/*.o 2>/dev/null || true
	@-rm */*/*.d 2>/dev/null || true
	@-rm *.o 2>/dev/null || true
	@-rm *.d 2>/dev/null || true
	@-rm libkernel.a 2>/dev/null || true

-include $(OBJS:.o=.d)
-include $(LIBK_OBJS:.o=.d)