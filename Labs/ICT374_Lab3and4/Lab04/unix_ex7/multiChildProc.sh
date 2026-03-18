#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

N="$1"

validate_input() {
    if [[ ! "$N" =~ ^[0-9]+$ ]] || [[ "$N" -le 0 ]]; then
        printf "Usage: %s <positive_integer>\n" "$0" >&2
        return 1
    fi
}

create_child() {
    local i pid ppid minutes
    i="$1"
    pid=$$
    ppid=$(ps -o ppid= -p "$pid" | tr -d ' ')
    if [[ -z "$ppid" ]]; then
        printf "Error: Could not retrieve PPID for PID %d\n" "$pid" >&2
        return 1
    fi
    printf "Child %d: PID=%d, PPID=%d\n" "$i" "$pid" "$ppid"
    minutes=$((i * 60))
    sleep "$minutes"
    exit 0
}

main() {
    if ! validate_input; then
        return 1
    fi

    local i child_pid
    for ((i=1; i<=N; i++)); do
        if (create_child "$i") & then
            child_pid=$!
            if [[ -z "$child_pid" ]]; then
                printf "Error: Failed to retrieve PID of child %d\n" "$i" >&2
                continue
            fi
        else
            printf "Error: Failed to fork child %d\n" "$i" >&2
        fi
    done

    wait
}

main "$@"
