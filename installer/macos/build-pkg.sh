#!/usr/bin/env bash
# Builds one unsigned .pkg per plugin, installing VST3, AU and CLAP into /Library/Audio/Plug-Ins.
# Usage: installer/macos/build-pkg.sh <build dir> <config> <out dir>
set -euo pipefail

build="$1"; config="$2"; out="$3"
mkdir -p "$out"

for dir in plugins/*/; do
    slug=$(basename "$dir")
    version=$(head -n1 "$dir/VERSION" | tr -d '[:space:]')
    numeric=${version%%-*}   # pkgbuild only accepts digits and dots
    art=$(ls -d "$build/plugins/$slug/"*_artefacts | head -n1)/"$config"
    product=$(basename "$art"/VST3/*.vst3 .vst3)

    root=$(mktemp -d)
    mkdir -p "$root/Library/Audio/Plug-Ins/VST3" "$root/Library/Audio/Plug-Ins/Components" "$root/Library/Audio/Plug-Ins/CLAP"
    cp -R "$art/VST3/$product.vst3" "$root/Library/Audio/Plug-Ins/VST3/"
    cp -R "$art/AU/$product.component" "$root/Library/Audio/Plug-Ins/Components/"
    cp -R "$art/CLAP/$product.clap" "$root/Library/Audio/Plug-Ins/CLAP/"

    # Ad-hoc signature so the bundles at least have a valid code signature (not notarized).
    for b in "$root"/Library/Audio/Plug-Ins/*/*; do
        codesign --force --deep --sign - "$b"
    done

    pkgbuild --root "$root" \
             --identifier "com.rommuser.$slug" \
             --version "$numeric" \
             --install-location / \
             "$out/$slug-$version.pkg"
    rm -rf "$root"
done
ls -l "$out"
