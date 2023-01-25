#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyGraphicalsSection
    : public juce::Component
    {
    public:
        PropertyGraphicalsSection(Director& director);
        ~PropertyGraphicalsSection() override = default;

        // juce::Component
        void resized() override;

    private:
        void setColourMap(Track::ColourMap const& colourMap);
        void setForegroundColour(juce::Colour const& colour);
        void setBackgroundColour(juce::Colour const& colour);
        void setTextColour(juce::Colour const& colour);
        void setShadowColour(juce::Colour const& colour);
        void setFontName(juce::String const& name);
        void setFontStyle(juce::String const& style);
        void setFontSize(float size);
        void showChannelLayout();
        void updateContent();
        void updateColourMap();
        void updateColours();
        void updateFont();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};

        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyFontName;
        PropertyList mPropertyFontStyle;
        PropertyList mPropertyFontSize;
        PropertyTextButton mPropertyChannelLayout;
        bool mChannelLayoutActionStarted{false};

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
