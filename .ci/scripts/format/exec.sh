#!/bin/bash -ex

chmod a+x ./.ci/scripts/format/docker.sh
docker run -v $(pwd):/citra citraemu/build-environments:linux-clang-format /bin/bash -ex /citra/.ci/scripts/format/docker.sh
