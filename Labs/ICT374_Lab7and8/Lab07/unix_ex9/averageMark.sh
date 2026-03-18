# ===== averageMark.sh =====
#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

SHM_FILE="/tmp/student_marks.shm"

main() {
    local sum=0 count=0 mark

    printf "Consumer: waiting for marks...\n"

    # Read file continuously until END marker appears
    while true; do
        if ! IFS= read -r mark; then
            sleep 1
            continue
        fi

        if [[ $mark = "END" ]]; then
            break
        fi

        if [[ -n $mark ]]; then
            sum=$((sum + mark))
            count=$((count + 1))
            printf "Consumer: received mark %s\n" "$mark"
        fi
    done < <(tail -n 0 -f "$SHM_FILE")

    if [[ $count -gt 0 ]]; then
        avg=$((sum / count))
        printf "Consumer: average mark = %d (from %d students)\n" "$avg" "$count"
    else
        printf "Consumer: no marks received.\n"
    fi
}

main "$@"
