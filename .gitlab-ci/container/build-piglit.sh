#!/bin/bash

set -ex

git clone https://gitlab.freedesktop.org/mesa/piglit.git --single-branch --no-checkout /piglit
pushd /piglit
git checkout b2c9d8f56b45d79f804f4cb5ac62520f0edd8988

# TODO: Remove the following patch when piglit commit got past
# 1cd716180cfb6ef0c1fc54702460ef49e5115791
git apply $OLDPWD/.gitlab-ci/piglit/build-piglit_backport-s3-migration.diff

patch -p1 <$OLDPWD/.gitlab-ci/piglit/disable-vs_in.diff
cmake -S . -B . -G Ninja -DCMAKE_BUILD_TYPE=Release $PIGLIT_OPTS $EXTRA_CMAKE_ARGS
ninja $PIGLIT_BUILD_TARGETS
find -name .git -o -name '*ninja*' -o -iname '*cmake*' -o -name '*.[chao]' | xargs rm -rf
rm -rf target_api
if [ "x$PIGLIT_BUILD_TARGETS" = "xpiglit_replayer" ]; then
    find ! -regex "^\.$" \
         ! -regex "^\.\/piglit.*" \
         ! -regex "^\.\/framework.*" \
         ! -regex "^\.\/bin$" \
         ! -regex "^\.\/bin\/replayer\.py" \
         ! -regex "^\.\/templates.*" \
         ! -regex "^\.\/tests$" \
         ! -regex "^\.\/tests\/replay\.py" 2>/dev/null | xargs rm -rf
fi
popd
