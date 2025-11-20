#pragma once

#include "../Track/AnlTrackDirector.h"
#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Director
    : public Track::MultiDirector
    {
    public:
        using HierarchyManager = Track::HierarchyManager;
        Director(Accessor& accessor, Track::MultiDirector& trackMultiDirector, HierarchyManager& hierarchyManager, juce::UndoManager& undoManager);
        ~Director() override;

        Accessor& getAccessor();
        HierarchyManager& getHierarchyManager();

        void updateTracks(NotificationType notification);

        bool hasChanged(bool includeTracks) const;
        void startAction(bool includeTracks);
        void endAction(bool includeTracks, ActionState state, juce::String const& name = {});
        std::function<Accessor&()> getSafeAccessorFn();
        void setSafeAccessorRetriever(std::function<Accessor&()> const& sar);

        Track::Director const& getTrackDirector(juce::String const& identifier) const override;
        Track::Director& getTrackDirector(juce::String const& identifier) override;

        std::function<void(NotificationType notification)> onIdentifierUpdated = nullptr;
        std::function<void(NotificationType notification)> onNameUpdated = nullptr;
        std::function<void(NotificationType notification)> onLayoutUpdated = nullptr;
        std::function<void(NotificationType notification)> onFocusUpdated = nullptr;

    private:
        Accessor& mAccessor;
        Track::MultiDirector& mTrackMultiDirector;
        HierarchyManager& mHierarchyManager;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;
        std::function<Accessor&()> mSafeAccessorRetrieverFn = nullptr;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Group

ANALYSE_FILE_END
