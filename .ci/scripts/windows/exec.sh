#!/bin/bash -ex

mkdir -p "ccache" || true
chmod a+x ./.ci/scripts/windows/docker.sh
docker run -e CCACHE_DIR=/citra/ccache -v $(pwd):/citra citraemu/build-environments:linux-mingw /bin/bash -ex /citra/.ci/scripts/windows/docker.sh $1
