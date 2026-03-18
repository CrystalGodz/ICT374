#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

sigint_count=0
sigquit_count=0

handle_sigint() {
    sigint_count=$((sigint_count + 1))
    if [[ $sigint_count -eq 1 ]]; then
        printf "Caught SIGINT (Ctrl+C) once. PID=%d\n" "$$"
    else
        printf "Caught SIGINT again, exiting. PID=%d\n" "$$"
        exit 0
    fi
}

handle_sigquit() {
    sigquit_count=$((sigquit_count + 1))
    if [[ $sigquit_count -eq 1 ]]; then
        printf "Caught SIGQUIT (Ctrl+\\) once. PID=%d\n" "$$"
    else
        printf "Caught SIGQUIT again, exiting. PID=%d\n" "$$"
        exit 0
    fi
}

catch_signals() {
    trap handle_sigint INT
    trap handle_sigquit QUIT
    printf "Catching SIGINT and SIGQUIT. PID=%d\n" "$$"
}

infinite_loop() {
    while true; do
        sleep 1
    done
}

main() {
    catch_signals
    infinite_loop
}

main "$@"
