#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class GraphicPresetContent
    : public juce::Component
    {
    public:
        GraphicPresetContent();
        ~GraphicPresetContent() override;

        // juce::Component
        void resized() override;

    private:
        void updateFromGlobalPreset();
        void setColourMap(Track::ColourMap const& colourMap);
        void setForegroundColour(juce::Colour const& colour);
        void setDurationColour(juce::Colour const& colour);
        void setBackgroundColour(juce::Colour const& colour);
        void setTextColour(juce::Colour const& colour);
        void setShadowColour(juce::Colour const& colour);
        void setFontName(juce::String const& name);
        void setFontStyle(juce::String const& style);
        void setFontSize(float size);
        void setLineWidth(float width);
        void setUnit(juce::String const& unit);
        void setLabelJustification(Track::LabelLayout::Justification justification);
        void setLabelPosition(float position);

        Accessor::Listener mListener{typeid(*this).name()};
        PropertyList mPropertyColourMap;
        PropertyColourButton mPropertyForegroundColour;
        PropertyColourButton mPropertyDurationColour;
        PropertyColourButton mPropertyBackgroundColour;
        PropertyColourButton mPropertyTextColour;
        PropertyColourButton mPropertyShadowColour;
        PropertyList mPropertyFontName;
        PropertyList mPropertyFontStyle;
        PropertyList mPropertyFontSize;
        PropertyNumber mPropertyLineWidth;
        PropertyText mPropertyUnit;
        PropertyList mPropertyLabelJustification;
        PropertyNumber mPropertyLabelPosition;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicPresetContent)
    };

    class GraphicPresetPanel
    : public HideablePanelTyped<GraphicPresetContent>
    {
    public:
        GraphicPresetPanel();
        ~GraphicPresetPanel() override = default;
    };
} // namespace Application

ANALYSE_FILE_END
