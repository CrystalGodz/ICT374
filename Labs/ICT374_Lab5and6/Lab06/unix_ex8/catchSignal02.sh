#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

child_pid=0

handle_sigchld() {
    local cpid
    if ! cpid=$(ps -o pid= --ppid "$$" 2>/dev/null | tr -d ' '); then
        printf "SIGCHLD received, but failed to determine child PID\n" >&2
        return
    fi
    printf "SIGCHLD received: child with PID %s has terminated\n" "$cpid"
}

handle_sigalrm() {
    printf "SIGALRM received: alarm clock expired\n"
}

handle_sigint() {
    printf "SIGINT received: numeric value is %d\n" "$((INT))"
}

setup_traps() {
    # Use trap with specific handlers
    trap handle_sigchld CHLD
    trap handle_sigalrm ALRM
    trap handle_sigint INT
}

create_child() {
    (
        sleep 10
    ) &
    child_pid=$!
    printf "Child created with PID %d\n" "$child_pid"
}

set_alarm() {
    # In Bash, no direct alarm(), emulate with a background sleep+kill
    (
        sleep 20
        kill -ALRM "$$"
    ) &
}

main_loop() {
    local i=0
    while true; do
        i=$((i + 1))
        printf "Parent loop iteration %d (PID=%d)\n" "$i" "$$"
        sleep 1
    done
}

main() {
    setup_traps
    create_child
    set_alarm
    main_loop
}

main "$@"
