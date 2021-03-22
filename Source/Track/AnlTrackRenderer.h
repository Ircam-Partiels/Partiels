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
            , range
        };

        static void renderImage(juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Image const& image, Zoom::Accessor const& xZoomAcsr, Zoom::Accessor const& yZoomAcsr);
        
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
        void paintRange(juce::Graphics& g, juce::Rectangle<int> const& bounds, Zoom::Accessor const& timeZoomAcsr);
        
        Accessor& mAccessor;
        Type const mType;
        juce::Image mImage;
    };
}

ANALYSE_FILE_END
