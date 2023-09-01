#!/bin/sh

if [ -z $1 ]; then
	CONFIGURATION="Debug"
else
  CONFIGURATION=$1
fi

if command -v xcbeautify &> /dev/null; then
  set -o pipefail && NSUnbufferedIO=YES cmake --build build --config $CONFIGURATION  2>&1 | xcbeautify
elif command -v xcpretty &> /dev/null; then
  set -o pipefail && cmake --build build --config $CONFIGURATION | xcpretty
else
  cmake --build build --config $CONFIGURATION
fi
