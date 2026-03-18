#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

# Globals (allowed by style rules)
CHILD_PID=""
COPROC_RFD=""

start_child_coproc() {
    # Start a coprocess (child). Child writes one message to stdout (the pipe),
    # sleeps 10s, closes its stdout (the write end of the pipe), reports to stderr,
    # sleeps another 10s, then exits.
    if ! coproc {
        printf "Hello from child!\n"
        sleep 10

        # close child's stdout (write end of pipe)
        exec 1>&-
        printf "Child: closed write end of pipe.\n" >&2

        sleep 10
        printf "Child: terminating now.\n" >&2
    }; then
        printf "Error: failed to start coprocess\n" >&2
        return 1
    fi

    local pid; pid=$COPROC_PID
    CHILD_PID=$pid
    COPROC_RFD="${COPROC[0]}"
    printf "Child started with PID %s\n" "$CHILD_PID"
    return 0
}

parent_wait_for_pipe_close() {
    local rfd line
    rfd=$COPROC_RFD

    # Duplicate the coprocess read fd to fd 3 for stable reading
    if ! exec 3<&"$rfd"; then
        printf "Error: failed to dup coproc read fd %s\n" "$rfd" >&2
        return 1
    fi

    # Read lines until EOF (child closes write end)
    while IFS= read -r line <&3; do
        printf "Parent: received message: %s\n" "$line"
    done

    # EOF reached -> child has closed its write end
    printf "Parent: detected that child closed the pipe. Exiting now.\n"

    # Close fd 3 cleanly
    exec 3<&-
    return 0
}

main() {
    if ! start_child_coproc; then
        return 1
    fi

    if ! parent_wait_for_pipe_close; then
        return 1
    fi
    
    return 0
}

main "$@"
