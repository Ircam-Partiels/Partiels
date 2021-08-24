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
        , comment
        , expand
        , grid_off
        , grid_partial
        , grid_full
        , information
        , layers
        , loading
        , loop
        , music
        , pause
        , photocamera
        , play
        , plus
        , properties
        , question
        , rewind
        , search
        , shrink
        , verified
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
