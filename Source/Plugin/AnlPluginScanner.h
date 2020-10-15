
#pragma once

#include "../Tools/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class Scanner
    {
    public:
        
        Scanner() = default;
        ~Scanner() = default;
        
        void scan();
        
        static std::vector<std::string> getPluginKeys();
        
    private:
        
        JUCE_LEAK_DETECTOR(Scanner)
    };
}

ANALYSE_FILE_END
