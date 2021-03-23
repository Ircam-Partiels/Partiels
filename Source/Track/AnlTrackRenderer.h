#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Renderer
    {
    public:
        Renderer(Accessor& accessor);
        ~Renderer() = default;
        
        std::function<void(void)> onUpdated = nullptr;
        
        void prepareRendering();
        void paint(juce::Graphics& g, juce::Rectangle<int> const& bounds);
        
    private:
        enum class DisplayMode
        {
              unsupported
            , surface
            , bar
            , segment
            , matrix
        };
        
        void paintFrame(juce::Graphics& g, juce::Rectangle<int> const& bounds);
        
        Accessor& mAccessor;
        juce::Image mImage;
    };
}

ANALYSE_FILE_END
