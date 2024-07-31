TOOLCHAIN_VER = v3
LIBC_PATH = ../Library/LibC/

.PHONY: default
default: x86_64

all: x86_64 rv64

x86_64:
	echo "[i] Copying LibC headers to build context"
	cp -v -r $(LIBC_PATH)/include/ ./include/
	echo "[i] Making toolchain image"
	docker build -f Dockerfile . -t ghcr.io/muzuwi/muos-toolchain:$(TOOLCHAIN_VER)

rv64:
	echo "[i] Copying LibC headers to build context"
	cp -v -r $(LIBC_PATH)/include/ ./include/
	echo "[i] Making toolchain image"
	docker build -f Dockerfile.rv64 . -t ghcr.io/muzuwi/muos-toolchain:$(TOOLCHAIN_VER)-rv64
