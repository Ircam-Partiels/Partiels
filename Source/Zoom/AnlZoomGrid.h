#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class Grid
    : public juce::Component
    {
    public:
        // clang-format off
        enum ColourIds : int
        {
              backgroundColourId = 0x2010000
            , tickColourId //= 0x2010000
            , textColourId
        };
        
        enum Orientation : bool
        {
              vertical
            , horizontal
        };
        // clang-format on
        
        using StringifyFn = std::function<juce::String(double)>;

        Grid(Accessor& accessor, Orientation orientation, StringifyFn toString);
        ~Grid() override;
    
        // Juce::Component
        void paint(juce::Graphics& g) override;
        
        static void paintHorizontal(Accessor const& accessor, juce::Component const& c, StringifyFn stringify, juce::Graphics& g);
        static void paintVertical(juce::Graphics& g, Accessor const& accessor, juce::Rectangle<int> const& bounds, StringifyFn const stringify, juce::Justification justification);

    private:
        Accessor& mAccessor;
        Orientation const mOrientation;
        StringifyFn mToString;
        Accessor::Listener mListener;
    };
} // namespace Zoom

ANALYSE_FILE_END
