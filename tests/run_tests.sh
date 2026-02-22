#!/usr/bin/env bash
# Run Lua test suites against a running engine instance (http://127.0.0.1:7777).
#
# Usage (from repo root):
#   bash tests/run_tests.sh              # run all test_*.lua suites
#   bash tests/run_tests.sh test_math    # run a specific suite by name prefix
#
# Requirements:
#   - Engine must be running in Dev or Debug mode (build/launch_dev.bat)
#   - curl and python must be on PATH

set -euo pipefail

ENGINE_URL="http://127.0.0.1:7777"
LOG_WAIT=4          # seconds to wait after posting a suite for results to appear
TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
FRAMEWORK_FILE="$TESTS_DIR/framework.lua"

# ── Helpers ───────────────────────────────────────────────────────────────────

die() { echo "ERROR: $*" >&2; exit 1; }

check_engine() {
    curl -sf "$ENGINE_URL/" > /dev/null 2>&1 \
        || die "Engine debug server not reachable at $ENGINE_URL — launch build/launch_dev.bat first"
}

get_log_path() {
    local rel
    rel=$(curl -sf "$ENGINE_URL/logfile" | python -c "import sys,json; print(json.load(sys.stdin)['path'])")
    # rel is like "../stats/log_02_22_26_01_20_00.log"; resolve to absolute path
    local build_dir
    build_dir="$(cd "$TESTS_DIR/../build" && pwd)"
    echo "$build_dir/stats/${rel##*/}"
}

run_suite() {
    local test_file="$1"
    local suite_name
    suite_name=$(basename "$test_file" .lua)

    echo ""
    echo "── $suite_name ──────────────────────────────"

    # Inline the framework then the test body (strip the dofile line from the test).
    local framework_body test_body full_payload
    framework_body=$(cat "$FRAMEWORK_FILE")
    test_body=$(sed '/dofile.*framework/d' "$test_file")
    full_payload="${framework_body}
${test_body}"

    # Post a sentinel so we know exactly where this suite's output begins in the log.
    local sentinel="SENTINEL_${suite_name}_$$"
    curl -sf -X POST "$ENGINE_URL/exec" --data-raw "print('$sentinel')" > /dev/null
    curl -sf -X POST "$ENGINE_URL/exec" --data-binary "$full_payload" > /dev/null

    sleep "$LOG_WAIT"

    local log_path
    log_path=$(get_log_path)

    # Extract output after the sentinel.
    local output
    output=$(awk "/$sentinel/{found=1; next} found && /SUITE_RESULT/{print; exit} found{print}" "$log_path" \
             | grep "_LUA:" | sed 's/.*|_LUA: //')

    if [[ -z "$output" ]]; then
        echo "  WARNING: No output found. Engine may be slow or crashed — check $log_path"
        return 1
    fi

    local passed=0 failed=0
    while IFS= read -r line; do
        echo "  $line"
        if [[ "$line" == "[PASS]"* ]]; then ((passed++)) || true
        elif [[ "$line" == "[FAIL]"* ]]; then ((failed++)) || true
        fi
    done <<< "$output"

    if [[ $failed -eq 0 ]]; then
        return 0
    else
        return 1
    fi
}

# ── Main ──────────────────────────────────────────────────────────────────────

check_engine

total_pass=0
total_fail=0
failed_names=()

if [[ $# -gt 0 ]]; then
    mapfile -d '' test_files < <(
        for pattern in "$@"; do
            find "$TESTS_DIR" -name "${pattern}*.lua" ! -name "framework.lua" -print0
        done | sort -zu)
else
    mapfile -d '' test_files < <(find "$TESTS_DIR" -name "test_*.lua" -print0 | sort -z)
fi

[[ ${#test_files[@]} -gt 0 ]] || die "No test files found in $TESTS_DIR"

for f in "${test_files[@]}"; do
    if run_suite "$f"; then
        ((total_pass++)) || true
    else
        ((total_fail++)) || true
        failed_names+=("$(basename "$f" .lua)")
    fi
done

echo ""
echo "══════════════════════════════════════"
echo "Suites: $((total_pass + total_fail))  Passed: $total_pass  Failed: $total_fail"
[[ ${#failed_names[@]} -eq 0 ]] || echo "Failed: ${failed_names[*]}"
echo "══════════════════════════════════════"

[[ $total_fail -eq 0 ]]
