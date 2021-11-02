#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class LookAndFeel
    : public Misc::LookAndFeel
    {
    public:
        LookAndFeel() = default;
        ~LookAndFeel() override = default;

        void setColourChart(ColourChart const& colourChart) override;
    };
} // namespace Application

ANALYSE_FILE_END
