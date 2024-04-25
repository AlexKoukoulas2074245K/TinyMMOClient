#!/bin/bash
TOP_LEVEL_DIR=$(git rev-parse --show-toplevel)

pushd .

cd $TOP_LEVEL_DIR

for assetFilename in assets/*/*.{png,vs}; do
    f="$(basename -- $assetFilename)"
    extension="${f##*.}"
    filename="${f%.*}"

    if ! [[ $(git grep $f) ]] && ! [[ $(git grep $filename) ]]; then
        echo "Found unused file: $f"
    fi
done

popd
