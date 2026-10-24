#!/usr/bin/env bash
# A printed verdict is insufficient: validator crashes must block packaging.
set -euo pipefail
if [ "$#" -ne 3 ]; then
    echo "usage: $0 <pluginval> <plugin bundle> <log file>" >&2
    exit 2
fi
validator=$1; plugin=$2; log=$3
mkdir -p "$(dirname "$log")"
set +e
"$validator" --strictness-level 10 --validate-in-process --skip-gui-tests --validate "$plugin" 2>&1 | tee "$log"
statuses=("${PIPESTATUS[@]}")
set -e
if [ "${statuses[0]}" -ne 0 ] || [ "${statuses[1]}" -ne 0 ]; then
    echo "::error::pluginval or log capture failed for $plugin" >&2
    exit 1
fi
if grep -qE '^FAILURE|^\*\*\* FAILED|!!! Test' "$log" || ! grep -q '^SUCCESS' "$log"; then
    echo "::error::pluginval did not pass for $plugin" >&2
    exit 1
fi
