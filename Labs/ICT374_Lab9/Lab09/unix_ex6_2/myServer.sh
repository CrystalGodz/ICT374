#!/usr/bin/env bash
set -o pipefail
set -u

PORT=5000
USERFILE="./users.db"

declare -A USERS
while IFS=" " read -r user pass; do
    [[ -n $user && -n $pass ]] && USERS["$user"]="$pass"
done < "$USERFILE"

handle_client() {
    local client
    if ! read -r client; then
        printf "Failed to read login\n" >&2
        return
    fi

    local username password
    username=$(printf "%s" "$client" | awk '{print $1}')
    password=$(printf "%s" "$client" | awk '{print $2}')

    if [[ -z ${USERS[$username]+x} || ${USERS[$username]} != "$password" ]]; then
        printf "Incorrect username or password\n"
        return
    fi

    printf "Welcome %s\n" "$username"

    while read -r line; do
        if [[ $line == "quit" ]]; then
            printf "Good-bye!\n"
            break
        fi
        printf "You said %s\n" "$line"
    done
}

while true; do
    nc -l -p "$PORT" | handle_client
done
