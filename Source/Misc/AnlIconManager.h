#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

class IconManager
{
public:
    // clang-format off
    enum ColourIds : int
    {
          normalColourId = 0x2000500
        , overColourId
        , downColourId
    };
    // clang-format on

    // clang-format off
    enum class IconType
    {
          alert
        , cancel
        , checked
        , chevron
        , conversation
        , edit
        , expand
        , information
        , loading
        , loop
        , navigate
        , pause
        , photocamera
        , play
        , properties
        , question
        , rewind
        , search
        , share
        , shrink
    };
    // clang-format off

    static juce::Image getIcon(IconType const type);

    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() = default;
        virtual void setButtonIcon(juce::ImageButton& button, IconType const type) = 0;
    };
};

ANALYSE_FILE_END
