#!/bin/bash
if [[ $# -lt 2 ]]
then
    if [[ $# -lt 1 || "$1" == "-h" || "$1" == "--help" ]]
    then
        echo "merge_translations.sh <keys_file> <translations_file> [output_file]"
        exit 0
    else
        echo "error: missing translated sentences file"
        exit 1
    fi
fi

original_file=$1
translated_file=$2

content=$(paste -d '" = "' "$original_file" /dev/null /dev/null /dev/null /dev/null "$translated_file")

if [[ $? -ne 0 ]]
then
    exit 2
fi

content=$(echo "$content" | sed -e 's/^/\"/' | sed -e 's/$/\"/')

echo "language:
countries:

$content"
