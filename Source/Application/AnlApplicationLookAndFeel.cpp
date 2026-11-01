#include "AnlApplicationLookAndFeel.h"
#include "../Document/AnlDocumentSection.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    setColourChart(ColourChart::Mode::night);
    setColour(ResizerBar::ColourIds::activeColourId, juce::Colours::transparentBlack);
    setColour(ResizerBar::ColourIds::inactiveColourId, juce::Colours::transparentBlack);
}

void Application::LookAndFeel::setColourChart(ColourChart const& colourChart)
{
    using Type = ColourChart::Type;
    setColour(Zoom::Ruler::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Zoom::Ruler::ColourIds::gridColourId, colourChart.get(Type::text));
    setColour(Zoom::Ruler::ColourIds::anchorColourId, colourChart.get(Type::accent));
    setColour(Zoom::Ruler::ColourIds::selectionColourId, colourChart.get(Type::accent));

    setColour(Transport::LoopBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Transport::LoopBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive));

    setColour(Transport::PlayheadBar::ColourIds::startPlayheadColourId, colourChart.get(Type::inactive));
    setColour(Transport::PlayheadBar::ColourIds::runningPlayheadColourId, colourChart.get(Type::accent));

    setColour(Track::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Track::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Track::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Group::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Group::Section::ColourIds::overflyColourId, colourChart.get(Type::accent).withAlpha(0.3f));

    setColour(Document::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));
    Misc::LookAndFeel::setColourChart(colourChart);
}

void Application::LookAndFeel::drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, juce::Rectangle<int> const& textArea, juce::TextLayout& textLayout)
{
    auto constexpr cornerSize = 4.0f;

    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::outlineColourId));
    g.drawRoundedRectangle(alert.getLocalBounds().toFloat(), cornerSize, 2.0f);

    auto const bounds = alert.getLocalBounds().reduced(1);
    g.reduceClipRegion(bounds);

    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

    auto const getIcon = [&]()
    {
        switch(alert.getAlertType())
        {
            case juce::AlertWindow::QuestionIcon:
                return ScalableImage::fromSvg(AnlIconsData::Question_svg, AnlIconsData::Question_svgSize);
            case juce::AlertWindow::WarningIcon:
                return ScalableImage::fromSvg(AnlIconsData::Alert_svg, AnlIconsData::Alert_svgSize);
            case juce::AlertWindow::InfoIcon:
                return ScalableImage::fromSvg(AnlIconsData::Info_svg, AnlIconsData::Info_svgSize);
            case juce::AlertWindow::NoIcon:
                return ScalableImage();
        }
        return ScalableImage();
    };

    auto constexpr iconSize = 180.0f;
    auto const iconPixelSize = juce::roundToInt(iconSize * g.getInternalContext().getPhysicalPixelScaleFactor());
    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::outlineColourId));
    g.drawImage(getIcon().getImage(iconPixelSize, iconPixelSize), {-48.0f, -20.0f, iconSize, iconSize}, juce::RectanglePlacement::stretchToFit, true);

    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::textColourId));
    textLayout.draw(g, textArea.withX(84).withTop(12).toFloat());
}

juce::Button* Application::LookAndFeel::createDocumentWindowButton(int buttonType)
{
    using TitleBarButtons = juce::DocumentWindow::TitleBarButtons;
    if(buttonType == TitleBarButtons::closeButton)
    {
        return std::make_unique<Icon>(ScalableImage::fromSvg(AnlIconsData::Cancel_svg, AnlIconsData::Cancel_svgSize)).release();
    }
    return LookAndFeel_V4::createDocumentWindowButton(buttonType);
}

ANALYSE_FILE_END
