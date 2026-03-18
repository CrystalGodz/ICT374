#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

LOGFILE="/tmp/daemon.log"
PIDFILE="/tmp/daemon.pid"

start_daemon() {
    (
        # Start new session, detach from terminal
        setsid bash -c '
            umask 0
            cd /
            exec >/dev/null 2>&1
            while true; do
                printf "I have nothing to do\n" >> "'"$LOGFILE"'"
                sleep 10
            done
        ' &
        echo $! > "$PIDFILE"
    )

    sleep 0.2
    if [[ -s "$PIDFILE" ]]; then
        local daemon_pid
        daemon_pid=$(cat "$PIDFILE")
        printf "Daemon started with PID %d\n" "$daemon_pid"
    else
        printf "Error: Failed to start daemon\n" >&2
        return 1
    fi
}

stop_daemon() {
    if [[ -s "$PIDFILE" ]]; then
        local daemon_pid
        daemon_pid=$(cat "$PIDFILE")
        if kill "$daemon_pid" 2>/dev/null; then
            printf "Daemon with PID %d stopped\n" "$daemon_pid"
            rm -f "$PIDFILE"
        else
            printf "Error: Could not kill daemon with PID %d\n" "$daemon_pid" >&2
        fi
    else
        printf "No running daemon found\n" >&2
    fi
}

main() {
    case "${1:-}" in
        start) start_daemon ;;
        stop)  stop_daemon ;;
        *) printf "Usage: %s {start|stop}\n" "$0" >&2; return 1 ;;
    esac
}

main "$@"
