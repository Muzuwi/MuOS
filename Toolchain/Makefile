TOOLCHAIN_VER = v1
LIBC_PATH = ../LibC/

.PHONY: default
default:
	echo "[i] Copying LibC headers to build context"
	cp -v -r $(LIBC_PATH)/include/ ./include/
	echo "[i] Making toolchain image"
	docker build -f Dockerfile . -t ghcr.io/muzuwi/muos-toolchain:$(TOOLCHAIN_VER)
