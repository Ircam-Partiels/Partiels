#pragma once

#include "../Plugin/AnlPluginListModel.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    //! @brief The class manage to save and restore the application properties from files.
    class Properties
    : private juce::ChangeListener
    {
    public:
        static const int sMaxIONumber = 64;

        Properties();
        ~Properties() override;

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        // clang-format off
        enum class PropertyType
        {
              Application = 0
            , PluginList = 1
            , AudioSetup = 2
        };
        // clang-format on

        static juce::File getFile(juce::StringRef const& fileName);
        void saveToFile(PropertyType type);
        void loadFromFile(PropertyType type);

        Accessor::Listener mApplicationListener;
        PluginList::Accessor::Listener mPluginListListener;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Properties)
    };
} // namespace Application

ANALYSE_FILE_END
