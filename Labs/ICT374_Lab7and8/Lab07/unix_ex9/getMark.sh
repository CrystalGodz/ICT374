# ===== getMark.sh =====
#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

SHM_FILE="/tmp/student_marks.shm"

# Ensure shared file exists and is empty
: > "$SHM_FILE"

main() {
    local mark
    while true; do
        printf "Enter student mark (or 'end' to finish): "
        if ! IFS= read -r mark; then
            continue
        fi

        if [[ $mark = "end" ]]; then
            printf "Producer: input finished. Signaling average mark calculator.\n"
            printf "END\n" >> "$SHM_FILE"
            break
        fi

        # Validate numeric mark
        if ! printf "%s" "$mark" | grep -Eq '^[0-9]+$'; then
            printf "Invalid mark. Please enter a number or 'end'.\n" >&2
            continue
        fi

        printf "%s\n" "$mark" >> "$SHM_FILE"
        printf "Producer: stored mark %s\n" "$mark"
    done
}

main "$@"
