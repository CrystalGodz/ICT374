#!/usr/bin/env bash
set -o pipefail
set -u

SERVER=127.0.0.1
PORT=5000

if [[ $# -ne 2 ]]; then
    printf "Usage: %s <username> <password>\n" "$0" >&2
    exit 1
fi

USERNAME=$1
PASSWORD=$2

{
    printf "%s %s\n" "$USERNAME" "$PASSWORD"
    while read -r input; do
        printf "%s\n" "$input"
        [[ $input == "quit" ]] && break
    done
} | nc "$SERVER" "$PORT"
