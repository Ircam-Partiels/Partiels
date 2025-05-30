#!/bin/bash
if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]; then
    echo "generate-translation-file.sh <keys_file> <translations_file> <output_file>"
    exit 0
fi

if [ "$#" -ne 3 ]; then
    echo "error: invalid arguments"
    echo "generate-translation-file.sh <keys_file> <translations_file> <output_file>"
    exit 1
fi

key_file=$1
translated_file=$2
output_file=$3

if [[ ! -f "$key_file" ]]; then
    echo "error: $key_file does not exist"
    exit 1
fi
if [[ ! -f "$translated_file" ]]; then
    echo "error: $translated_file does not exist"
    exit 1
fi

# Concatenate the two files line by line: "key" = "translation"
content=$(paste -d '=' "$key_file" "$translated_file" | sed 's/^\(.*\)=\(.*\)$/  "\1" = "\2"/' | sed 's/\\n/\\n  /g')
if [[ $? -ne 0 ]]; then
    echo "error: failed to process files"
    exit 1
fi

if [[ -f "$output_file" ]]; then
    rm "$output_file"
fi

echo "language:" >> "$output_file"
echo "countries:" >> "$output_file"
echo "" >> "$output_file"
echo "$content" >> "$output_file"

