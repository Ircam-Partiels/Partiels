#pragma once

#include "AnlDocumentModel.h"
#include "../Track/AnlTrackDirector.h"
#include "../Group/AnlGroupDirector.h"
#include "../Plugin/AnlPluginListTable.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginListAccessor, PluginList::Scanner& pluginListScanner, juce::AudioFormatManager& audioFormatManager);
        ~Director();
        
        void sanitize(NotificationType const notification);
        
        void addTrack(AlertType const alertType, NotificationType const notification);
        void removeTrack(AlertType const alertType, juce::String const identifier, NotificationType const notification);
        void moveTrack(AlertType const alertType, juce::String const groupIdentifier, juce::String const trackIdentifier, NotificationType const notification);
        void addGroup(AlertType const alertType, NotificationType const notification);
        void removeGroup(AlertType const alertType, juce::String const identifier, NotificationType const notification);
        
    private:
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        std::vector<std::unique_ptr<Group::Director>> mGroups;
        double mSampleRate = 44100.0;
        double mDuration = 0.0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
