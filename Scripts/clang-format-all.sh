#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$ThisPath/.."
find Source -iname *.h -o -iname *.cpp | xargs clang-format -i -style=file
find Dependencies/Misc/Source -iname *.h -o -iname *.cpp | xargs clang-format -i -style=file
find VampPlugins -iname *.h -o -iname *.cpp | xargs clang-format -i -style=file
find Dependencies/ircam-vamp-extension/Source -iname *.h -o -iname *.c | xargs clang-format -i -style=file
find Dependencies/ircam-vamp-extension/Source -iname *.hpp -o -iname *.cpp | xargs clang-format -i -style=file
