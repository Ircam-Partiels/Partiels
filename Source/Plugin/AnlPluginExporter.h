#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    class Exporter
    {
    public:
        
        static void toCsv(juce::FileOutputStream& stream, std::vector<Result> const& results);
        
        static void toXml(juce::FileOutputStream& stream, std::vector<Result> const& results);
    };
}

ANALYSE_FILE_END
