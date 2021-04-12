#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentGroupSection.h"
#include "AnlDocumentFileInfoPanel.h"
#include "../Track/AnlTrackSection.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class GroupStrechableSection
    : public juce::Component
    , private GroupContainer<std::unique_ptr<Track::Section>>
    {
    public:
        GroupStrechableSection(Accessor& accessor);
        ~GroupStrechableSection() override;
        
        std::function<void(juce::String const& identifier)> onRemoveTrack = nullptr;
        
        // juce::Component
        void resized() override;
        
    private:
        // GroupContainer<std::unique_ptr<Track::Section>>
        void updateEnded() override;
        void removeFromGroup(std::unique_ptr<Track::Section>& value) override;
        std::unique_ptr<Track::Section> createForGroup(Track::Accessor& trackAccessor) override;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        GroupSection mGroupSection {mAccessor};
        DraggableTable mDraggableTable;
        ConcertinaTable mConcertinaTable {"", false};
        BoundsListener mBoundsListener;
    };
}

ANALYSE_FILE_END
