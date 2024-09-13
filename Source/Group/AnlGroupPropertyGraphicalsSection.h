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
        void setDurationColour(juce::Colour const& colour);
        void setBackgroundColour(juce::Colour const& colour);
        void setTextColour(juce::Colour const& colour);
        void setShadowColour(juce::Colour const& colour);
        void setFontName(juce::String const& name);
        void setFontStyle(juce::String const& style);
        void setFontSize(float size);
        void setUnit(juce::String const& unit);
        void setLabelJustification(Track::LabelLayout::Justification justification);
        void setLabelPosition(float position);
        void setLogScale(bool state);
        void showVisibilityInGroup();
        void showChannelLayout();
        void updateContent();
        void updateColourMap();
        void updateColours();
        void updateFont();
        void updateUnit();
        void updateLabel();
        void updateLogScale();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};

        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyDurationColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyFontName;
        PropertyList mPropertyFontStyle;
        PropertyList mPropertyFontSize;
        PropertyText mPropertyUnit;
        PropertyList mPropertyLabelJustification;
        PropertyNumber mPropertyLabelPosition;
        PropertyToggle mPropertyValueRangeLogScale;
        PropertyTextButton mPropertyChannelLayout;
        bool mChannelLayoutActionStarted{false};
        PropertyTextButton mPropertyShowInGroup;
        bool mShowInGroupActionStarted{false};

        LayoutNotifier mLayoutNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
