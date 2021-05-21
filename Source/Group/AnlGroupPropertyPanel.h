#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupModel.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        PropertyPanel(Director& director);
        ~PropertyPanel() override;
        
        // juce::Component
        void resized() override;
        
    private:
        void showChannelLayout();
        
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener;
        
        PropertyText mPropertyName;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyTextButton mPropertyChannelLayout;
        
        bool mChannelLayoutActionStarted{false};
        static auto constexpr sInnerWidth = 300;
    };
} // namespace Group

ANALYSE_FILE_END
