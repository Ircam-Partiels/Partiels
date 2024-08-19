# Misc

A set of miscellaneous C++ classes based on the Juce framework.

#### Usage

In the CMakeLists.txt file of your project:
```cmake
include(path/to/submodule/Misc.cmake)
target_sources(MyTarget PRIVATE ${MiscSource})
target_link_libraries(MyTarget PRIVATE MiscData)
```
In your code:
```cpp
#include "path/to/submodule/Misc/Misc.h"
```

#### Credits

This project has been developed at [Ircam](https://www.ircam.fr/) by Pierre Guillot.
