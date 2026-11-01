#!/usr/bin/env bash
# Combines the per-plugin .pkg files into one installer with every plugin, where each plugin can
# still be unticked under Customize. Run after build-pkg.sh.
# Usage: installer/macos/build-bundle.sh <dir with per-plugin pkgs> <out file> <version>
set -euo pipefail

pkgs="$1"; out="$2"; version="$3"
numeric=${version%%-*}
args=()
for p in "$pkgs"/*.pkg; do
    case "$(basename "$p")" in ROMMUSER-Plugins-*) continue ;; esac
    args+=(--package "$p")
done
[ "${#args[@]}" -gt 0 ] || { echo "no per-plugin pkgs in $pkgs" >&2; exit 1; }

dist=$(mktemp)
productbuild --synthesize "${args[@]}" "$dist"
# Installer shows the bundle as "ROMMUSER Plugins" instead of the first package's name.
perl -pi -e 's|(<installer-gui-script[^>]*>)|$1<title>ROMMUSER Plugins</title>|' "$dist"
productbuild --distribution "$dist" --package-path "$pkgs" --identifier com.rommuser.allplugins \
             --version "$numeric" "$out"
rm -f "$dist"
ls -l "$out"
