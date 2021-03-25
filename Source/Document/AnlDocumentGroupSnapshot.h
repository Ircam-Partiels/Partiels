#pragma once

#include "AnlDocumentModel.h"
#include "../Track/AnlTrackSnapshot.h"


ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupSnapshot
    : public juce::Component
    {
    public:
        GroupSnapshot(Accessor& accessor);
        ~GroupSnapshot() override;
        
        // juce::Component
        void resized() override;
        
    private:
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        std::map<juce::String, std::unique_ptr<Track::Snapshot>> mSnapshots;
    };
}

ANALYSE_FILE_END
