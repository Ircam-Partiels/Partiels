#pragma once

#include "AnlApplicationController.h"
#include "AnlApplicationHeader.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    {
    public:
        
        Interface();
        ~Interface() override = default;
        
        // juce::Component
        void resized() override;
        
    private:
        
        Header mHeader;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Interface)
    };
}

ANALYSE_FILE_END

