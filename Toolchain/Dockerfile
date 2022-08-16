FROM gcc:12.1.0
COPY ./src/ /workdir/
COPY ./include/* /usr/local/muOS/include/
COPY ./include/* /usr/local/muOS/usr/include/
RUN apt update && apt install --yes \
	texinfo \
	libmpfr-dev \
	libmpfr6 \
	libmpc-dev \
	libmpc3	\
	&& rm -rf /var/lib/apt/lists/* 
RUN cd /workdir/ && \
	bash -c ./download-archives.sh && \
	bash -c ./make-binutils.sh && \
	bash -c ./make-gcc.sh && \
	rm -rf /workdir/
RUN apt update && apt install --yes --no-install-recommends \
	cmake \
	nasm \
	&& rm -rf /var/lib/apt/lists/*
LABEL org.opencontainers.image.source="https://github.com/Muzuwi/MuOS"
