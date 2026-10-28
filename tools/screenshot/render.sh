#!/usr/bin/env bash
# Renders every plugin's editor to docs/screenshots/<slug>.png for the README gallery.
#
#     tools/screenshot/render.sh [build-dir] [scale]
#
# Needs the same build dependencies as the plugins. On Linux it runs under xvfb-run when there
# is no display. The slug matches the release tag (ClipReady -> clip-ready).
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
build="${1:-$root/build-screenshots}"
scale="${2:-1.5}"
out="$root/docs/screenshots"

cmake -S "$root" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    -DMUSIC_PLUGINS_BUILD_TESTS=OFF -DMUSIC_PLUGINS_BUILD_SCREENSHOTS=ON
targets=()
for dir in "$root"/plugins/*/; do
    targets+=("$(basename "$dir")_Screenshot")
done
cmake --build "$build" --config Release --parallel 4 --target "${targets[@]}"

run=()
if [[ "$(uname)" == "Linux" && -z "${DISPLAY:-}" ]]; then
    run=(xvfb-run -a)
fi

mkdir -p "$out"
for dir in "$root"/plugins/*/; do
    name="$(basename "$dir")"
    slug="$(echo "$name" | sed -E 's/([a-z0-9])([A-Z])/\1-\2/g' | tr '[:upper:]' '[:lower:]')"
    exe="$(find "$build/plugins/$name" -type f -name "${name}_Screenshot*" \( -perm -u+x -o -name '*.exe' \) | head -n 1)"
    "${run[@]}" "$exe" "$out/$slug.png" "$scale"
done
