#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

START_TIME=$(date +%s)
CAN_EXIT_AFTER=$((START_TIME + 30))
EXIT_REQUESTED=0

handle_sigint() {
    local now remaining
    now=$(date +%s)
    if [[ $now -ge $CAN_EXIT_AFTER ]]; then
        printf "\nSIGINT received. Exiting now (uptime >= 30s)\n"
        exit 0
    else
        remaining=$((CAN_EXIT_AFTER - now))
        printf "\nSIGINT received but uptime < 30s. Will exit automatically after %d more seconds.\n" "$remaining"
        EXIT_REQUESTED=1
    fi
}

handle_sigquit() {
    local now remaining
    now=$(date +%s)
    if [[ $now -ge $CAN_EXIT_AFTER ]]; then
        printf "\nSIGQUIT received. Exiting now (uptime >= 30s)\n"
        exit 0
    else
        remaining=$((CAN_EXIT_AFTER - now))
        printf "\nSIGQUIT received but uptime < 30s. Will exit automatically after %d more seconds.\n" "$remaining"
        EXIT_REQUESTED=1
    fi
}

setup_traps() {
    trap handle_sigint INT
    trap handle_sigquit QUIT
}

interactive_loop() {
    local input now uptime remaining
    while true; do
        uptime=$(( $(date +%s) - START_TIME ))

        if [[ $EXIT_REQUESTED -eq 1 ]]; then
            now=$(date +%s)
            if [[ $now -ge $CAN_EXIT_AFTER ]]; then
                uptime=$((now - START_TIME))
                printf "30s runtime reached (uptime %ds). Exiting.\n" "$uptime"
                exit 0
            fi
            remaining=$((CAN_EXIT_AFTER - now))
            printf "[Uptime: %ds] Countdown to exit: %ds\n" "$uptime" "$remaining"
            sleep 1
            continue
        fi

        printf "[Uptime: %ds] Type something: " "$uptime"
        if ! IFS= read -r input; then
            continue
        fi
        printf "You typed: %s\n" "$input"
    done
}

main() {
    setup_traps
    interactive_loop
}

main "$@"
