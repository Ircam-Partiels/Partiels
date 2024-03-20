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
    setColour(Zoom::Ruler::ColourIds::anchorColourId, colourChart.get(Type::active));
    setColour(Zoom::Ruler::ColourIds::selectionColourId, colourChart.get(Type::active));

    setColour(Transport::LoopBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Transport::LoopBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive));

    setColour(Transport::PlayheadBar::ColourIds::startPlayheadColourId, colourChart.get(Type::inactive));
    setColour(Transport::PlayheadBar::ColourIds::runningPlayheadColourId, colourChart.get(Type::active));

    setColour(Track::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Track::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Track::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Group::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Group::Section::ColourIds::overflyColourId, colourChart.get(Type::inactive).withAlpha(0.4f));

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
                return juce::ImageCache::getFromMemory(AnlIconsData::question_png, AnlIconsData::question_pngSize);
            case juce::AlertWindow::WarningIcon:
                return juce::ImageCache::getFromMemory(AnlIconsData::warning_png, AnlIconsData::warning_pngSize);
            case juce::AlertWindow::InfoIcon:
                return juce::ImageCache::getFromMemory(AnlIconsData::info_png, AnlIconsData::info_pngSize);
            case juce::AlertWindow::NoIcon:
                return juce::Image();
        }
        return juce::Image();
    };

    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::outlineColourId));
    g.drawImage(getIcon(), {-48.0f, -20.0f, 180.0f, 180.0f}, juce::RectanglePlacement::stretchToFit, true);

    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::textColourId));
    textLayout.draw(g, textArea.withX(84).withTop(12).toFloat());
}

juce::Button* Application::LookAndFeel::createDocumentWindowButton(int buttonType)
{
    using TitleBarButtons = juce::DocumentWindow::TitleBarButtons;
    if(buttonType == TitleBarButtons::closeButton)
    {
        return std::make_unique<Icon>(juce::ImageCache::getFromMemory(AnlIconsData::cancel_png, AnlIconsData::cancel_pngSize)).release();
    }
    return LookAndFeel_V4::createDocumentWindowButton(buttonType);
}

ANALYSE_FILE_END
