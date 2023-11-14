#!/bin/sh

if [ -z $3 ]; then
  ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  REPO_PATH="$ThisPath/.."
  BUILD_PATH="$REPO_PATH/release"
  DSYM_PATH=$BUILD_PATH/Partiels_artefacts/Debug/Partiels.app.dSYM
else
  DSYM_PATH=$1
fi

atos -arch arm64 -o $DSYM_PATH/Contents/Resources/DWARF/Partiels -l $1 $2
