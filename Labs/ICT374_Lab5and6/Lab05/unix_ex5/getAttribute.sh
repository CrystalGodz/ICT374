#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

FILE="$1"

validate_input() {
    if [[ -z "$FILE" ]]; then
        printf "Usage: %s <filename>\n" "$0" >&2
        return 1
    fi
    if [[ ! -e "$FILE" ]]; then
        printf "Error: File '%s' does not exist\n" "$FILE" >&2
        return 1
    fi
}

report_file_info() {
    local ftype perms inode size
    if ! ftype=$(stat -c %F -- "$FILE" 2>/dev/null); then
        printf "Error: Failed to determine file type\n" >&2
        return 1
    fi
    if ! perms=$(stat -c %A -- "$FILE" 2>/dev/null); then
        printf "Error: Failed to get permissions\n" >&2
        return 1
    fi
    if ! inode=$(stat -c %i -- "$FILE" 2>/dev/null); then
        printf "Error: Failed to get inode number\n" >&2
        return 1
    fi
    if ! size=$(stat -c %s -- "$FILE" 2>/dev/null); then
        printf "Error: Failed to get file size\n" >&2
        return 1
    fi

    printf "File: %s\n" "$FILE"
    printf "Type: %s\n" "$ftype"
    printf "Permissions: %s\n" "$perms"
    printf "I-node: %s\n" "$inode"
    printf "Size: %s bytes\n" "$size"
}

main() {
    if ! validate_input; then
        return 1
    fi
    report_file_info
}

main "$@"
