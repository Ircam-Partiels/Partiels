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
        Director(Accessor& accessor, Track::MultiDirector& trackMultiDirector, juce::UndoManager& undoManager);
        ~Director() override;

        Accessor& getAccessor();

        void startAction();
        void endAction(juce::String const& name, ActionState state);

        Track::Director& getTrackDirector(juce::String const& identifier) override;

    private:
        Accessor& mAccessor;
        Track::MultiDirector& mTrackMultiDirector;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Group

ANALYSE_FILE_END
