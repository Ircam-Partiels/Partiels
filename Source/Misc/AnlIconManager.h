#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class IconManager
{
public:
    
    enum class IconType
    {
          alert
        , cancel
        , checked
        , edit
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
    };
    
    static juce::Image getIcon(IconType const type);
    
    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;
    };
};

ANALYSE_FILE_END
