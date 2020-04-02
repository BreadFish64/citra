#!/bin/bash -ex

. .ci/scripts/common/pre-upload.sh

REV_NAME="citra-linux-${GITDATE}-${GITREV}"
ARCHIVE_NAME="${REV_NAME}.tar.xz"
COMPRESSION_FLAGS="-cJvf"

if [ "${RELEASE_NAME}" = "mainline" ]; then
    DIR_NAME="${REV_NAME}"
else
    DIR_NAME="${REV_NAME}_${RELEASE_NAME}"
fi

mkdir -p "$DIR_NAME"

cp build/bin/citra "$REV_NAME"
cp build/bin/citra-qt "$REV_NAME"

. .ci/scripts/common/post-upload.sh
