#!/usr/bin/env bash

source common.sh

# Globally enable the ca derivations experimental flag
sed -i 's/experimental-features = .*/& ca-derivations ca-references/' "$NIX_CONF_DIR/nix.conf"

export REMOTE_STORE_DIR="$TEST_ROOT/remote_store"
export REMOTE_STORE="file://$REMOTE_STORE_DIR"

ensureCorrectlyCopied () {
    attrPath="$1"
    ## Ensure that the copy went right:
    # 1. The realisation file should exist
    # 2. It should be a valid json with an `outPath` field
    # 3. This field should point to the corresponding narinfo file
    drvOutput=$(basename $(nix eval --raw --file ./content-addressed.nix "$attrPath".drvPath))\!out
    realisationFile=$TEST_ROOT/remote_store/realisations/$drvOutput.doi

    outPathBasename=$(cat $realisationFile | jq -r .outPath)
    outPathHash=$(echo $outPathBasename | cut -d- -f1)

    test -f $TEST_ROOT/remote_store/$outPathHash.narinfo
}

testOneCopy () {
    clearStore
    rm -rf "$REMOTE_STORE_DIR"

    attrPath="$1"
    nix copy --to $REMOTE_STORE "$attrPath" --file ./content-addressed.nix

    ensureCorrectlyCopied "$attrPath"

    # Ensure that we can copy back what we put in the store
    clearStore
    nix copy --from $REMOTE_STORE \
        --file ./content-addressed.nix "$attrPath" \
        --no-check-sigs
}

for attrPath in rootCA dependentCA transitivelyDependentCA dependentNonCA dependentFixedOutput; do
    testOneCopy "$attrPath"
done
