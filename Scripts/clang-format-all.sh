#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$ThisPath/../Source"
clang-format -i -style=file */*.cpp */*.h
