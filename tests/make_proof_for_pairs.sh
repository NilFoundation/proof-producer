#!/bin/bash

color_red() { echo -e "\033[31m$1\033[0m"; }
color_green() { echo -e "\033[32m$1\033[0m"; }

get_script_dir() {
    echo "$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
}

make_proof_for_pair() {
    local tbl_file=$1
    local crct_file="${tbl_file%assignment.tbl}circuit.crct"
    local proof_dir=$(dirname "$tbl_file")

    if [ -f "$crct_file" ]; then
        echo -n "Processing $tbl_file and $crct_file: "
        if ${script_dir}/../build/bin/proof-generator/proof-generator -t "$tbl_file" --circuit "$crct_file" --proof "$proof_dir/proof.bin"; then
            color_green "success"
        else
            color_red "failed"
            return 1
        fi
    else
        color_red "${crct_file} file not found for $tbl_file"
        return 1
    fi
}

clean_up() {
    echo "Cleaning up proof files..."
    find . -name 'proof' -type f -exec rm {} +
}

parse_args() {
    base_dir="."
    targets=()

    while [ "$#" -gt 0 ]; do
        case "$1" in
            --base-dir)
                base_dir="$2"
                shift 2
                ;;
            *)
                targets+=("$1")  # Add remaining arguments as targets
                shift
                ;;
        esac
    done
}

exit_code=0
script_dir=$(get_script_dir)
clean=false
parse_args "$@"

# Function to process files in a directory
process_directory() {
    local dir=$1
    find "$dir" -name 'assignment.tbl' | while read tbl_file; do
        if ! make_proof_for_pair "$tbl_file"; then
            exit_code=1
        fi
    done
}

# If targets are specified
if [ ${#targets[@]} -gt 0 ]; then
    for target in "${targets[@]}"; do
        process_directory "$base_dir/$target"
    done
else
    # Process the base directory
    process_directory "$base_dir"
fi

# Clean up if needed
if [ "$clean" = true ]; then
    clean_up
fi

exit $exit_code
