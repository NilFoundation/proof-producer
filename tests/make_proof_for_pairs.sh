#!/bin/bash

color_red() { echo -e "\033[31m$1\033[0m"; }
color_green() { echo -e "\033[32m$1\033[0m"; }

get_script_dir() {
    echo "$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
}

make_proof_for_pair() {
    local tbl_file=$1
    local crct_file="${tbl_file%assignment.tbl}circuit.crct"
    local relative_tbl_path="$(realpath --relative-to="$base_dir" "$tbl_file")"
    local proof_dir="${output_dir}/$(dirname ${relative_tbl_path})"

    local proof_generator_binary="${script_dir}/../build/bin/proof-generator-single-threaded/proof-generator-single-threaded"
    if [ "$use_multithreaded" = true ]; then
        proof_generator_binary="${script_dir}/../build/bin/proof-generator-multi-threaded/proof-generator-multi-threaded"
    fi

    if [ -f "$crct_file" ]; then
        mkdir -p "$proof_dir"  # Ensure the output directory exists
        echo -n "Processing $tbl_file and $crct_file (proof will be at $proof_dir): "
        if $proof_generator_binary -t "$tbl_file" --circuit "$crct_file" --proof "$proof_dir/proof.bin"; then
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
    find "$output_dir" -name 'proof' -type f -exec rm {} +
}

parse_args() {
    base_dir="."
    output_dir=""
    targets=()
    use_multithreaded=false

    while [ "$#" -gt 0 ]; do
        case "$1" in
            --base-dir)
                base_dir="$2"
                shift 2
                ;;
            --output-dir)
                output_dir="$2"
                shift 2
                ;;
            --multithreaded)
                use_multithreaded=true
                shift
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

if [ -z "$output_dir" ]; then
    echo "Output directory not specified. Using default."
    output_dir=$base_dir
fi

process_directory() {
    local dir=$1
    while read tbl_file; do
        if ! make_proof_for_pair "$tbl_file"; then
            exit_code=1
        fi
    done < <(find "$dir" -name 'assignment.tbl')
}

# If targets are specified
if [ ${#targets[@]} -gt 0 ]; then
    for target in "${targets[@]}"; do
        process_directory "$base_dir/$target"
    done
else
    process_directory "$base_dir"
fi

# Clean up if needed
if [ "$clean" = true ]; then
    clean_up
fi

exit $exit_code
