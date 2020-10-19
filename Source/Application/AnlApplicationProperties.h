#pragma once

#include "AnlApplicationModel.h"
#include "../Plugin/AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Properties
    {
    public:
        
        Properties();
        ~Properties();
        
    private:
        
        enum class PropertyType
        {
            Application = 0,
            PluginList = 1
        };
        
        static juce::File getFile(juce::StringRef const& fileName);
        void saveToFile(PropertyType type);
        void loadFromFile(PropertyType type);
        
        Accessor::Listener mApplicationListener;
        PluginList::Accessor::Listener mPluginListListener;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Properties)
    };
}

ANALYSE_FILE_END

