# Target architecture
TARGETARCH?=i386

# Compiler / linker variables
CFLAGS?=-O2 -g
CPPFLAGS?=-O2 -g
LDFLAGS?=
LIBS?=

# Libc/kernel lib variables
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(PREFIX)/lib

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -masm=intel
CPPFLAGS:=$(CPPFLAGS) -ffreestanding -Wall -Wextra -masm=intel -D__is_kernel -Iinclude -I$(INCLUDEDIR) -I/usr/lib/gcc/i686-elf/9.2.0/include -fno-rtti -fno-exceptions -fno-threadsafe-statics
LDFLAGS:=$(LDFLAGS)
NASMFLAGS?=-felf32
LIBS:=$(LIBS) -nostdlib -L$(LIBDIR) -lkernel -lgcc

# Target architecture directory
ARCHDIR=arch/$(TARGETARCH)

include $(ARCHDIR)/make.config

# Kernel objects to compile
KERNEL_OBJS=			\
$(KERNEL_ARCH_OBJS) 	\
kernel/kernel.o 		\
kernel/VirtualMemManager.o \
kernel/kmalloc.o 		\

# All objects
OBJS=					\
$(ARCHDIR)/crti.o 		\
$(ARCHDIR)/crtbegin.o 	\
$(KERNEL_OBJS) 			\
$(ARCHDIR)/crtend.o 	\
$(ARCHDIR)/crtn.o 		\

# Linker list
LINK_LIST=				\
$(LDFLAGS)				\
$(ARCHDIR)/crti.o 		\
$(ARCHDIR)/crtbegin.o 	\
$(KERNEL_OBJS)			\
$(LIBS)					\
$(ARCHDIR)/crtend.o 	\
$(ARCHDIR)/crtn.o 		\


.PHONY: all clean 
.SUFFIXES: .o .c .cpp .S .asm

all: muOsKernel

muOsKernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@.bin $(CFLAGS) $(LINK_LIST)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	@OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.asm.o:
	$(NASM) $(NASMFLAGS) $< -o $@

.cpp.o:
	$(GPP) -c $< -o $@ $(CPPFLAGS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	@-rm */*.o 2>/dev/null || true
	@-rm */*.d 2>/dev/null || true
	@-rm */*/*.o 2>/dev/null || true
	@-rm */*/*.d 2>/dev/null || true
	@-rm */*/*/*.o 2>/dev/null || true
	@-rm */*/*/*.d 2>/dev/null || true
	@-rm *.o 2>/dev/null || true
	@-rm *.d 2>/dev/null || true
	@-rm muOsKernel.bin 2>/dev/null || true

-include $(OBJS:.o=.d)