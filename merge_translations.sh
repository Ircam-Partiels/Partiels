#!/bin/bash

if [[ $# -lt 1 ]]
then
    echo "error: missing original sentences file"
    exit 1
elif [[ $# -lt 2 ]]
then
    if [[ "$1" == "-h" || "$1" == "--help" ]]
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
output_file=$3

content=$(paste -d '" = "' "$original_file" /dev/null /dev/null /dev/null /dev/null "$translated_file")
content=$(echo "$content" | sed -e 's/^/\"/' | sed -e 's/$/\"/')

if [[ "$output_file" == ""]]
then
    echo "$content"
else
    echo "$content" > "$output_file"
fi