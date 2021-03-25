#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class IconManager
{
public:
    
    enum ColourIds : int
    {
          normalColourId = 0x2000400
        , overColourId
        , downColourId
    };
    
    enum class IconType
    {
          alert
        , cancel
        , checked
        , edit
        , expand
        , information
        , loading
        , loop
        , navigate
        , pause
        , play
        , properties
        , question
        , rewind
        , search
        , share
        , shrink
    };
    
    static juce::Image getIcon(IconType const type);
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;
        virtual void setButtonIcon(juce::ImageButton& button, IconType const type) = 0;
    };
};

ANALYSE_FILE_END
