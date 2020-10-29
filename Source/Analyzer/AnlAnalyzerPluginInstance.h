#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class PluginInstance
    {
    public:
        using Attribute = Model::Attribute;
        using Signal = Model::Signal;
        
        PluginInstance(Accessor& accessor);
        ~PluginInstance();

    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstance)
    };
}

ANALYSE_FILE_END
