#!/bin/bash -ex

mkdir -p "ccache"  || true
chmod a+x ./.ci/scripts/linux/docker.sh
docker run -e ENABLE_COMPATIBILITY_REPORTING -e CCACHE_DIR=/citra/ccache -v $(pwd):/citra citraemu/build-environments:linux-fresh /bin/bash /citra/.ci/scripts/linux/docker.sh
