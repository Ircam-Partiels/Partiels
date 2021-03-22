#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Renderer
    {
    public:
        
        enum class Type
        {
              frame
        };
        
        Renderer(Accessor& accessor, Type type);
        ~Renderer() = default;
        
        std::function<void(void)> onUpdated = nullptr;
        
        void prepareRendering();
        void paint(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
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
        Type const mType;
        juce::Image mImage;
    };
}

ANALYSE_FILE_END
