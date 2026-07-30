#!/usr/bin/env sh

set -eu

renderer=$1
test_root=$(mktemp -d)

cleanup() {
    rm -rf -- "$test_root"
}
trap cleanup EXIT HUP INT TERM

"$renderer" --help >/dev/null

set +e
"$renderer" --output "$test_root/invalid" --unknown 1 >/dev/null 2>&1
invalid_code=$?
set -e
if [ "$invalid_code" -ne 2 ]; then
    echo "unknown option returned $invalid_code instead of 2" >&2
    exit 1
fi

output="$test_root/reference-frame"
"$renderer" \
    --output "$output" \
    --mass-M 1 \
    --spin 0 \
    --observer-r-M 30 \
    --inclination-deg 85 \
    --fov-y-deg 40 \
    --width 10 \
    --height 8 \
    --escape-r-M 60 \
    --max-affine-M 200 \
    --initial-step-M 0.02 \
    --max-step-M 0.25

test -f "$output/beauty.ppm"
test -f "$output/classification.ppm"
test -f "$output/rays.csv"
test -f "$output/manifest.json"
grep -Eq '"captured":[1-9][0-9]*' "$output/manifest.json"
grep -Eq '"escaped":[1-9][0-9]*' "$output/manifest.json"
grep -Eq '"disk_surface_hits":[1-9][0-9]*' "$output/manifest.json"
grep -q '"failed":0' "$output/manifest.json"

ray_rows=$(awk 'END { print NR - 1 }' "$output/rays.csv")
if [ "$ray_rows" -ne 80 ]; then
    echo "rays.csv contains $ray_rows rays instead of 80" >&2
    exit 1
fi

set +e
"$renderer" \
    --output "$output" \
    --width 10 \
    --height 9 >/dev/null 2>&1
duplicate_code=$?
set -e
if [ "$duplicate_code" -ne 5 ]; then
    echo "duplicate output returned $duplicate_code instead of 5" >&2
    exit 1
fi

axis_output="$test_root/axis-limited"
set +e
"$renderer" \
    --output "$axis_output" \
    --mass-M 1 \
    --spin 0 \
    --observer-r-M 30 \
    --inclination-deg 90 \
    --fov-y-deg 40 \
    --width 9 \
    --height 9 \
    --escape-r-M 60 \
    --max-affine-M 200 \
    --initial-step-M 0.02 \
    --max-step-M 0.25 >/dev/null
diagnostic_code=$?
set -e
if [ "$diagnostic_code" -ne 4 ]; then
    echo "diagnostic frame returned $diagnostic_code instead of 4" >&2
    exit 1
fi
grep -Eq '"unconverged":[1-9][0-9]*' \
    "$axis_output/manifest.json"
grep -q '"status":"diagnostic_failed"' \
    "$axis_output/manifest.json"
