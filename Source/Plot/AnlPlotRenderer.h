#pragma once

#include "AnlPlotModel.h"
#include "../Plugin/AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plot
{
    class Renderer
    {
        
    public:
        static void paint(juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const& colour, Plugin::Output const& output, std::vector<Plugin::Result> const& results, Zoom::Range const& valueRange, double time);
    };
}

ANALYSE_FILE_END
