#!/bin/bash

# Build the firmware (or the host unit tests) inside the Docker build image,
# so no toolchain has to be installed on the machine.
#
# Usage: ./docker/build.sh [options] [-- extra cmake arguments]
#   -t target       stm32l432kc (default), stm32f401vc or stm32f407vg
#   -b build_type   Debug (default) or Release
#   -o build_dir    build directory (default: build/<target>-<build_type>[-demo])
#   --demo          build the crash diagnostics demo (CRASH_DEMO=ON)
#   --tests         build and run the host unit tests instead of the firmware
#   --shell         open a shell in the build container
#   --rebuild-image rebuild the image even if it exists
#
# Examples:
#   ./docker/build.sh
#   ./docker/build.sh -t stm32f401vc --demo
#   ./docker/build.sh --tests
#
# The image is built on first use and tagged with a hash of the Dockerfile,
# so editing the Dockerfile rebuilds it. The repository is mounted at the same
# path as on the host and the build runs with the host user's UID, so build
# directories, map files and debug info are interchangeable with a native build.
# Flashing stays on the host: see flash_firmware.sh.


set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

TARGET="stm32l432kc"
BUILD_TYPE="Debug"
BUILD_DIR=""
DEMO="OFF"
MODE="firmware"
REBUILD_IMAGE=false
EXTRA_CMAKE_ARGS=()

while [ $# -gt 0 ]; do
    case "$1" in
        -t) TARGET="$2"; shift 2 ;;
        -b) BUILD_TYPE="$2"; shift 2 ;;
        -o) BUILD_DIR="$2"; shift 2 ;;
        --demo) DEMO="ON"; shift ;;
        --tests) MODE="tests"; shift ;;
        --shell) MODE="shell"; shift ;;
        --rebuild-image) REBUILD_IMAGE=true; shift ;;
        --) shift; EXTRA_CMAKE_ARGS=("$@"); break ;;
        *) sed -n '3,24p' "$0"; exit 1 ;;
    esac
done

case "$TARGET" in
    stm32l432kc|stm32f401vc|stm32f407vg) ;;
    *) echo " Unknown target: $TARGET (use stm32l432kc, stm32f401vc or stm32f407vg)"; exit 1 ;;
esac

case "$BUILD_TYPE" in
    Debug|Release) ;;
    *) echo " Unknown build type: $BUILD_TYPE (use Debug or Release)"; exit 1 ;;
esac

if ! command -v docker &> /dev/null; then
    echo " Docker is not installed!"
    exit 1
fi

if [ ! -f "$PROJECT_DIR/third_party/emb_lib/modules/emblib_assert.h" ]; then
    echo " Submodules are missing. Please run: git submodule update --init --recursive"
    exit 1
fi

# image tag follows the Dockerfile content
IMAGE_TAG=$(sha256sum "$SCRIPT_DIR/Dockerfile" | cut -c1-12)
IMAGE="cmake-stm32-template-build:$IMAGE_TAG"

if $REBUILD_IMAGE || ! docker image inspect "$IMAGE" &> /dev/null; then
    echo " Building the build image $IMAGE..."
    docker build -t "$IMAGE" "$SCRIPT_DIR"
fi

DOCKER_ARGS=(
    --rm
    --user "$(id -u):$(id -g)"
    --env HOME=/tmp
    --volume "$PROJECT_DIR:$PROJECT_DIR"
    --workdir "$PROJECT_DIR"
)
if [ -t 0 ] && [ -t 1 ]; then
    DOCKER_ARGS+=(--interactive --tty)
fi

run_in_container() {
    docker run "${DOCKER_ARGS[@]}" "$IMAGE" bash -c "$1"
}

case "$MODE" in
    shell)
        docker run "${DOCKER_ARGS[@]}" --interactive --tty "$IMAGE" bash
        ;;
    tests)
        BUILD_DIR="${BUILD_DIR:-build/tests}"
        echo " Building and running the unit tests in $BUILD_DIR"
        run_in_container "cmake -S tests -B '$BUILD_DIR' -G Ninja && cmake --build '$BUILD_DIR' && ctest --test-dir '$BUILD_DIR' --output-on-failure"
        ;;
    firmware)
        BUILD_DIR_SUFFIX=""
        if [ "$DEMO" = "ON" ]; then
            BUILD_DIR_SUFFIX="-demo"
        fi
        BUILD_DIR="${BUILD_DIR:-build/$TARGET-${BUILD_TYPE,,}$BUILD_DIR_SUFFIX}"
        echo " Building $TARGET ($BUILD_TYPE, CRASH_DEMO=$DEMO) in $BUILD_DIR"
        run_in_container "cmake -S . -B '$BUILD_DIR' -G Ninja \
            -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake \
            -DTARGET_BOARD=$TARGET -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCRASH_DEMO=$DEMO \
            ${EXTRA_CMAKE_ARGS[*]} && cmake --build '$BUILD_DIR'"

        FIRMWARE=$(find "$PROJECT_DIR/$BUILD_DIR" -maxdepth 1 -name "*_firmware" -type f | head -1)
        echo " Firmware: ${FIRMWARE#$PROJECT_DIR/}"
        echo " Flash with: ./flash_firmware.sh -t $TARGET ${FIRMWARE#$PROJECT_DIR/}"
        ;;
esac
