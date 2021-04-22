#pragma once

#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class Director
    {
    public:
        Director(Accessor& accessor, juce::UndoManager& undoManager);
        ~Director();
        
        Accessor& getAccessor();

        void startAction();
        void endAction(juce::String const& name, ActionState state);

    private:
        Accessor& mAccessor;
        juce::UndoManager& mUndoManager;
        Accessor mSavedState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
} // namespace Group

ANALYSE_FILE_END
