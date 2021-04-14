#pragma once

#include "AnlDocumentModel.h"
#include "../Track/AnlTrackDirector.h"
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
        void addGroup(AlertType const alertType, NotificationType const notification);
        void removeGroup(AlertType const alertType, juce::String const identifier, NotificationType const notification);
        
    private:
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        double mSampleRate = 44100.0;
        double mDuration = 0.0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
