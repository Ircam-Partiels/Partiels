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

if [ ! -f "$original_file" ]
then
    echo "$original_file must be a file!"
    exit 2
fi

if [ ! -f "$translated_file" ]
then
    echo "$translated_file must be a file!"
    exit 2
fi

if [ -f "$output_file" ]
then
    read -p "$output_file already exist, do you want to replace it? [yes/No] "
    if [[ "$REPLY" != "yes" ]]
    then
        echo "Action cancelled."
        exit 0
    fi
fi

content="language:
countries:

"$(paste -d '" = "' "$original_file" /dev/null /dev/null /dev/null /dev/null "$translated_file" | sed -e 's/^/\"/' | sed -e 's/$/\"/')

if [[ "$output_file" == "" ]]
then
    echo "$content"
else
    echo "$content" > "$output_file"
    echo "Translation file created, don't forget to fill the lnaguage name and country list."
fi