#pragma once

#include "../Group/AnlGroupDirector.h"
#include "../Plugin/AnlPluginListTable.h"
#include "../Track/AnlTrackDirector.h"
#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class Director
    {
    public:
        Director(Accessor& accessor, PluginList::Accessor& pluginListAccessor, PluginList::Scanner& pluginListScanner, juce::AudioFormatManager& audioFormatManager, juce::UndoManager& undoManager);
        ~Director();

        void sanitize(NotificationType const notification);

        void startAction();
        void endAction(juce::String const& name, ActionState state);

        void addTrack(AlertType const alertType, NotificationType const notification);
        bool removeTrack(juce::String const identifier, NotificationType const notification);
        void moveTrack(AlertType const alertType, juce::String const groupIdentifier, juce::String const trackIdentifier, NotificationType const notification);
        bool addGroup(juce::String const& name, size_t position, NotificationType const notification);
        bool removeGroup(juce::String const identifier, NotificationType const notification);

        void moveTrack(juce::String const groupIdentifier, juce::String const trackIdentifier, NotificationType const notification);

    private:
        Accessor& mAccessor;
        juce::AudioFormatManager& mAudioFormatManager;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        PluginList::Table mPluginListTable;
        juce::Component* mModalWindow = nullptr;
        std::vector<std::unique_ptr<Group::Director>> mGroups;
        std::vector<std::unique_ptr<Track::Director>> mTracks;
        double mSampleRate = 44100.0;
        double mDuration = 0.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Document

ANALYSE_FILE_END
