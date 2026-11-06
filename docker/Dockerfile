# Build environment for the firmware and the host unit tests.
#
# The ARM toolchain is the official Arm GNU Toolchain (the version the
# firmware is validated with), checked against its SHA-256. It is installed
# at the default path of cmake/arm-gcc-toolchain.cmake, so CMake finds it
# without any environment variable.
#
# Use docker/build.sh rather than building this image by hand.

FROM ubuntu:24.04

ARG ARM_TOOLCHAIN_VERSION=14.2.rel1
ARG ARM_TOOLCHAIN_SHA256=62a63b981fe391a9cbad7ef51b17e49aeaa3e7b0d029b36ca1e9c3b2a9b78823

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        ninja-build \
        make \
        g++ \
        git \
        python3 \
        wget \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

ENV ARM_GCC_TOOLCHAIN_PATH=/opt/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi

RUN wget -q -O /tmp/arm-toolchain.tar.xz \
        "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-x86_64-arm-none-eabi.tar.xz" \
    && echo "${ARM_TOOLCHAIN_SHA256}  /tmp/arm-toolchain.tar.xz" | sha256sum -c - \
    && tar -xJf /tmp/arm-toolchain.tar.xz -C /opt \
    && rm /tmp/arm-toolchain.tar.xz \
    && rm -rf "${ARM_GCC_TOOLCHAIN_PATH}/share/doc" \
    && "${ARM_GCC_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc" --version

ENV PATH=${ARM_GCC_TOOLCHAIN_PATH}/bin:${PATH}

# The repository is mounted with the host user's UID, which has no account
# in the image: let git read it anyway (version.h uses git describe)
RUN git config --system --add safe.directory '*'
