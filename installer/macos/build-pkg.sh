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

    # Renamed plugin: a preinstall script removes the bundles installed under the old name, so the
    # DAW does not list the plugin twice (same plugin ID, so saved projects load the new one).
    scripts=$(mktemp -d)
    if [ -s "$dir/FORMER_NAMES" ]; then
        former=$(head -n1 "$dir/FORMER_NAMES" | sed 's/[[:space:]]*$//')
        cat > "$scripts/preinstall" <<SCRIPT
#!/bin/sh
rm -rf "/Library/Audio/Plug-Ins/VST3/$former.vst3" \\
       "/Library/Audio/Plug-Ins/Components/$former.component" \\
       "/Library/Audio/Plug-Ins/CLAP/$former.clap"
exit 0
SCRIPT
        chmod +x "$scripts/preinstall"
    fi

    # All three bundles share CFBundleIdentifier com.rommuser.<slug> with the old-named install.
    # By default pkgbuild marks bundles relocatable, so Installer would put the new bundle where
    # it finds the old one (under the old name, or a bundle of another format) instead of at
    # its path in the payload. Install every bundle at its own path, and since alpha builds all
    # carry numeric version 1.0.0, always replace what is there.
    plist="$root.plist"
    pkgbuild --analyze --root "$root" "$plist"
    i=0
    while plutil -extract "$i" xml1 -o /dev/null "$plist" 2>/dev/null; do
        plutil -replace "$i.BundleIsRelocatable" -bool NO "$plist"
        plutil -replace "$i.BundleIsVersionChecked" -bool NO "$plist"
        i=$((i + 1))
    done
    [ "$i" -ge 3 ] || { echo "expected VST3, AU and CLAP bundles in the component plist, got $i" >&2; exit 1; }

    pkgbuild --root "$root" \
             --component-plist "$plist" \
             --scripts "$scripts" \
             --identifier "com.rommuser.$slug" \
             --version "$numeric" \
             --install-location / \
             "$out/${product// /}-$version.pkg"
    rm -rf "$root" "$scripts" "$plist"
done
ls -l "$out"
