#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."

echo '\033[0;34m' "Looking for changes"
echo '\033[0m'

cd $REPO_PATH
modified_files=$(git diff --name-only 2>&1)

for modified_file in $modified_files
do
	filename=$(basename -- "$modified_file")
	extension="${filename##*.}"
	if [ "$extension" = "cpp" ] || [ "$extension" = "h" ]
	then
		clang-format -i -style=file  "$modified_file"
	fi
done
