#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

OUTPUT="foo"

run_ls() {
    if ! ls -lt >"$OUTPUT" 2>/dev/null; then
        printf "Error: Failed to execute ls -lt\n" >&2
        return 1
    fi
    printf "Output written to %s\n" "$OUTPUT"
}

main() {
    run_ls
}

main "$@"
