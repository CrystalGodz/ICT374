#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

DIR="$1"

validate_input() {
    if [[ -z "$DIR" ]]; then
        printf "Usage: %s <directory>\n" "$0" >&2
        return 1
    fi
    if [[ ! -d "$DIR" ]]; then
        printf "Error: '%s' is not a directory\n" "$DIR" >&2
        return 1
    fi
}

list_inodes() {
    local entry inode
    if ! cd -- "$DIR"; then
        printf "Error: Could not enter directory '%s'\n" "$DIR" >&2
        return 1
    fi

    for entry in . .. *; do
        if [[ ! -e "$entry" ]]; then
            continue
        fi
        if ! inode=$(ls -id -- "$entry" 2>/dev/null | awk '{print $1}'); then
            printf "Error: Failed to get inode for %s\n" "$entry" >&2
            continue
        fi
        printf "Name: %-30s Inode: %s\n" "$entry" "$inode"
    done
}

main() {
    if ! validate_input; then
        return 1
    fi
    list_inodes
}

main "$@"
