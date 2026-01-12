FROM debian:stable AS builder

RUN apt-get update && \
    apt-get install -y build-essential wget bison flex \
                       libgmp-dev libmpfr-dev libmpc-dev texinfo

ENV CROSS_PREFIX=/cross
ENV CROSS_TARGET=i686-elf

ENV MAKE_JOBS=4

WORKDIR $CROSS_PREFIX

RUN wget https://ftp.gnu.org/gnu/binutils/binutils-2.45.tar.xz && \
    tar -xf binutils-2.45.tar.xz && \
    mkdir -p build-binutils && \
    cd build-binutils && \
    ../binutils-2.45/configure --target=$CROSS_TARGET --prefix="$CROSS_PREFIX" --with-sysroot --disable-nls --disable-werror && \
    make -j${MAKE_JOBS} && \
    make install -j${MAKE_JOBS} && \
    cd .. && \
    rm binutils-2.45.tar.xz && \
    rm -rf binutils-2.45

RUN wget https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz && \
    tar -xf gcc-15.2.0.tar.xz && \
    mkdir -p build-gcc && \
    cd build-gcc && \
    ../gcc-15.2.0/configure --target=$CROSS_TARGET --prefix="$CROSS_PREFIX" --disable-nls \
    --enable-languages=c --without-headers && \
    make all-gcc -j${MAKE_JOBS} && \
    make install-gcc -j${MAKE_JOBS} && \
    cd .. && \
    rm gcc-15.2.0.tar.xz && \
    rm -rf gcc-15.2.0

FROM debian:stable

COPY --from=builder /cross /opt/cross

RUN apt-get update && apt-get install -y make bear gawk nasm libmpc-dev

ENV PATH="/opt/cross/bin:$PATH"
ENV BUILD_TYPE=

WORKDIR /src
