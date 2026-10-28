#!/usr/bin/env bash
# A printed verdict is insufficient: validator crashes must block packaging.
# pluginval's in-process teardown segfaults nondeterministically on Linux CI
# (see #44), so a run that crashes or ends without a verdict is retried, up to
# three attempts. A reported test failure is final on the first attempt.
set -euo pipefail
if [ "$#" -ne 3 ]; then
    echo "usage: $0 <pluginval> <plugin bundle> <log file>" >&2
    exit 2
fi
validator=$1; plugin=$2; log=$3
mkdir -p "$(dirname "$log")"
for attempt in 1 2 3; do
    set +e
    "$validator" --strictness-level 10 --validate-in-process --skip-gui-tests --validate "$plugin" 2>&1 | tee "$log"
    statuses=("${PIPESTATUS[@]}")
    set -e
    if grep -qE '^FAILURE|^\*\*\* FAILED|!!! Test' "$log"; then
        echo "::error::pluginval reported a failure for $plugin" >&2
        exit 1
    fi
    if [ "${statuses[0]}" -eq 0 ] && [ "${statuses[1]}" -eq 0 ] && grep -q '^SUCCESS' "$log"; then
        exit 0
    fi
    echo "::warning::pluginval crashed or gave no verdict for $plugin (attempt $attempt)" >&2
done
echo "::error::pluginval did not pass for $plugin after 3 attempts" >&2
exit 1
