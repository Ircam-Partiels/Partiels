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

        static void askToRestoreDefaultAudioSettings(juce::String const& error);
        static juce::File getFile(juce::StringRef const& fileName);

        static std::optional<Plugin::State> getDefaultPreset(Plugin::Key const& key);
        static void setDefaultPreset(Plugin::Key const& key, Plugin::State const& state);
        static void removeDefaultPreset(Plugin::Key const& key);

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        // clang-format off
        enum class PropertyType
        {
              Application = 0
            , PluginList = 1
            , AudioSetup = 2
            , DefaultPresets = 3
        };
        // clang-format on

        void saveToFile(PropertyType type);
        void loadFromFile(PropertyType type);

        static std::map<Plugin::Key, Plugin::State>& getDefaultPresets();
        static void saveDefaultPresetsToFile();
        static void loadDefaultPresetsFromFile();

        Accessor::Listener mApplicationListener{typeid(*this).name()};
        PluginList::Accessor::Listener mPluginListListener{typeid(*this).name()};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Properties)
    };
} // namespace Application

ANALYSE_FILE_END
