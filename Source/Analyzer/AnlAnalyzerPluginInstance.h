#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class PluginInstance
    {
    public:
        PluginInstance(Accessor& accessor);
        ~PluginInstance();

    private:
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
    private:
        
        void update();
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstance)
    };
}

ANALYSE_FILE_END
