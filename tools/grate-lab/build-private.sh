#!/bin/sh
# Host-only build and staging; never deploys or changes device/system libraries.
set -eu
if [ "$#" -ne 4 ]; then
    echo 'usage: build-private.sh CROSS_FILE BUILD_DIR DEVICE_PREFIX STAGE_DIR' >&2
    exit 2
fi
cross_file=$1
build_dir=$2
prefix=$3
stage_dir=$4
case "$prefix" in *"/../"*|*"/.."|*"/./"*|*"/.") echo "Prefix must not contain dot components" >&2; exit 2 ;; esac
case "$prefix" in /home/*/*|/root/*/*) ;; *) echo 'Use a private device home prefix' >&2; exit 2 ;; esac
case "$stage_dir" in /) echo "Do not stage into the filesystem root" >&2; exit 2 ;; esac
case "$stage_dir" in /*) ;; *) echo 'STAGE_DIR must be absolute' >&2; exit 2 ;; esac
meson setup "$build_dir" --cross-file "$cross_file" --prefix="$prefix" \
    --libdir=lib --buildtype=debugoptimized --wrap-mode=nodownload \
    -Db_ndebug=false -Dgallium-drivers=grate,swrast -Dvulkan-drivers= \
    -Dplatforms=x11 -Dglx=dri -Degl=enabled -Dgles1=disabled -Dgles2=enabled \
    -Dgbm=enabled -Dglvnd=false -Dshared-glapi=enabled -Dllvm=disabled \
    -Dgallium-vdpau=disabled -Dgallium-va=disabled -Dgallium-xvmc=disabled \
    -Dgallium-nine=false -Dgallium-opencl=disabled -Dzstd=disabled \
    -Dlibunwind=disabled -Dvalgrind=disabled -Dlmsensors=disabled -Dshader-cache=disabled
ninja -C "$build_dir" -j2
DESTDIR="$stage_dir" meson install --no-rebuild -C "$build_dir"
