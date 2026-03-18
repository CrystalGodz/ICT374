#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

FIFO_PATH="/tmp/myfifo_$$"

cleanup() {
    rm -f "$FIFO_PATH"
}
trap cleanup EXIT INT QUIT TERM

start_child_fifo() {
    {
        # Child writes message into FIFO
        printf "Hello from child!\n"
        sleep 10

        # Close write end explicitly
        exec 1>&-
        printf "Child: closed write end of FIFO.\n" >&2

        sleep 10
        printf "Child: terminating now.\n" >&2
    } >"$FIFO_PATH" &
    CHILD_PID=$!
    printf "Child started with PID %s\n" "$CHILD_PID"
}

parent_read_fifo() {
    local line
    # Parent reads from FIFO until EOF
    while IFS= read -r line <"$FIFO_PATH"; do
        printf "Parent: received message: %s\n" "$line"
    done
    printf "Parent: detected that child closed the FIFO. Exiting now.\n"
}

main() {
    mkfifo "$FIFO_PATH"
    start_child_fifo
    parent_read_fifo
}

main "$@"
