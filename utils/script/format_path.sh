

process_file() {
    local original_file_name="$1"
    local temp_file_name="$original_file_name.tmp"

    echo "Processing: $original_file_name"

    # Create a temporary file for processing
    cp "$original_file_name" "$temp_file_name"

    # Check if the file is empty
    if [ ! -s "$temp_file_name" ]; then
        echo "Empty file: $original_file_name"
        rm "$temp_file_name"
        return
    fi

    # Detect the EOL type (Linux: \n, Windows: \r\n)
    eol=$(od -c "$temp_file_name" | head -n 1 | grep -o '\\r  \\n\|\\n')
    if [[ "$eol" == '\r \n' ]]; then
        eol=$'\r\n'
    elif [[ "$eol" == '\n' ]]; then
        eol=$'\n'
    else
        echo "WARNING: Unusual EOL in file: $original_file_name"
        rm "$temp_file_name"
        return
    fi

    # Expand tabs to 4 spaces, remove trailing whitespace, and ensure one empty line before EOF
    cleaned_content=$(expand -t 4 "$temp_file_name" | sed 's/[[:space:]]*$//' | sed -e :a -e '/^\n*$/{$d;N;ba' -e '}' | sed '$a\')

    # Write the cleaned content back to the original file with the original EOL
    echo -n "$cleaned_content" | awk -v eol="$eol" '{printf "%s%s", $0, eol}' > "$original_file_name"

    # Remove the temporary file
    rm "$temp_file_name"
}

# Checks clang-format exists
check_clang_format() {
    if [ ! command -v clang-format &>/dev/null ]; then
        exit_error "clang-format is not installed or not on the PATH"
    fi
}



# Main script
test $# -eq 1 || exit_usage
(
    file_processed=0
    directory="$1"
    check_clang_format
    # Check if the provided argument is a directory
    if [ ! -d "$directory" ]; then
        echo "Error: $directory is not a directory."
        exit 1
    fi
    
    # Find all .c and .h files in the directory recursively
    while IFS= read -r -d '' original_file_name; do
        # Process the file
        process_file "$original_file_name"
        file_processed=$((file_processed + 1))
    done < <(find "$directory" -type f \( -name "*.c" -o -name "*.h" \) -print0)

    echo "$file_processed files processed"
)