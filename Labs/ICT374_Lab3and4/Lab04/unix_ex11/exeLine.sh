#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

read_command() {
    local cmd
    printf "%% " >&2   # print prompt to stderr so it doesn’t pollute the command
    if ! IFS= read -r cmd; then
        printf "Error: Failed to read command\n" >&2
        return 1
    fi
    printf "%s" "$cmd"
}

execute_command() {
    local cmd=$1
    if [[ -z "$cmd" ]]; then
        return 0
    fi
    if [[ "$cmd" == "bye" ]]; then
        return 2
    fi

    if command -v "$cmd" >/dev/null 2>&1; then
        "$cmd"
    else
        # try to handle arguments (e.g., "ls -l")
        eval "$cmd" || {
            printf "Error: Failed to execute command '%s'\n" "$cmd" >&2
            exit 127
        }
    fi
}

main() {
    local cmd child_pid status
    while true; do
        cmd=$(read_command) || continue
        if [[ "$cmd" == "bye" ]]; then
            break
        fi

        ( execute_command "$cmd" ) &
        child_pid=$!
        if ! wait "$child_pid"; then
            status=$?
            printf "Child terminated with status %d\n" "$status" >&2
        fi
    done
}

main
