#include "AnlApplicationLookAndFeel.h"
#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Document/AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    auto const backgroundColour = juce::Colours::grey.darker();
    auto const rulerColour = juce::Colours::grey;
    auto const textColour = juce::Colours::white;
    auto const thumbColour = juce::Colours::white;
    
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::ColouredPanel::ColourIds::backgroundColourId, backgroundColour);

    setColour(Layout::PropertySection::ColourIds::separatorColourId, juce::Colours::transparentBlack);
    setColour(Layout::StrechableContainer::Section::resizerActiveColourId, rulerColour.brighter());
    setColour(Layout::StrechableContainer::Section::resizerInactiveColourId, backgroundColour.darker());
    
    setColour(Zoom::Ruler::backgroundColourId, backgroundColour.darker());
    setColour(Zoom::Ruler::tickColourId, textColour);
    setColour(Zoom::Ruler::textColourId, textColour);
    setColour(Zoom::Ruler::anchorColourId, thumbColour);
    setColour(Zoom::Ruler::selectionColourId, thumbColour);
    
    setColour(Analyzer::Thumbnail::backgroundColourId, backgroundColour.darker());
    setColour(Analyzer::Thumbnail::textColourId, textColour);
    setColour(Analyzer::Section::sectionColourId, juce::Colours::black);
    
    setColour(Document::Section::backgroundColourId, backgroundColour);
    
    setColour(Document::Playhead::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Document::Playhead::playheadColourId, thumbColour);
    
    // juce::ResizableWindow
    setColour(juce::ResizableWindow::ColourIds::backgroundColourId, backgroundColour.darker());
    
    // juce::TextButton
    setColour(juce::TextButton::ColourIds::buttonColourId, backgroundColour);
    setColour(juce::TextButton::ColourIds::buttonOnColourId, backgroundColour.brighter());
    
    // juce::TextButton
    setColour(juce::Slider::ColourIds::backgroundColourId, backgroundColour);
    setColour(juce::Slider::ColourIds::thumbColourId, thumbColour);
    
    // juce::ComboBox
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::arrowColourId, textColour);
    
    // juce::ScrollBar
    setColour(juce::ScrollBar::ColourIds::backgroundColourId, backgroundColour.darker());
    setColour(juce::ScrollBar::ColourIds::thumbColourId, thumbColour);
    
    // juce::AlertWindow
    setColour(juce::AlertWindow::ColourIds::backgroundColourId, backgroundColour.darker());
}

bool Application::LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
}

void Application::LookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(isMouseDown);
    g.fillAll(scrollbar.findColour(juce::ScrollBar::ColourIds::backgroundColourId));
    
    auto const thumbBounds = isScrollbarVertical ?  juce::Rectangle<int>{x, thumbStartPosition, width, thumbSize} : juce::Rectangle<int>{thumbStartPosition, y, thumbSize, height};
    
    auto const colour = scrollbar.findColour(juce::ScrollBar::ColourIds::thumbColourId);
    g.setColour(isMouseOver ? colour.brighter (0.25f) :colour);
    g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 4.0f);
}

void Application::LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
    juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(box.findColour(juce::ComboBox::ColourIds::backgroundColourId, true));
    g.fillRect(bounds);
    g.setColour(box.findColour(box.hasKeyboardFocus(true) ? juce::ComboBox::ColourIds::focusedOutlineColourId :juce::ComboBox::ColourIds::outlineColourId, true));
    g.drawRect(bounds.reduced(1));
    
    g.setColour(box.findColour(juce::ComboBox::ColourIds::arrowColourId).withAlpha(box.isEnabled() ? 1.0f : 0.2f));
    auto const arrawBounds = bounds.removeFromRight(height / 2).withSizeKeepingCentre(height / 2, height / 2).reduced(2).toFloat();
    juce::Path p;
    p.addTriangle(arrawBounds.getTopLeft(), arrawBounds.getTopRight(), arrawBounds.getBottomLeft().withX(arrawBounds.getCentreX()));
    g.fillPath(p);
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

    auto getIcon = [&]()
    {
        switch (alert.getAlertType())
        {
            case juce::AlertWindow::QuestionIcon:
                return juce::ImageCache::getFromMemory(BinaryData::question_png, BinaryData::question_pngSize);
            case juce::AlertWindow::WarningIcon:
                return juce::ImageCache::getFromMemory(BinaryData::alert_png, BinaryData::alert_pngSize);
            case juce::AlertWindow::InfoIcon:
                return juce::ImageCache::getFromMemory(BinaryData::information_png, BinaryData::information_pngSize);
            case juce::AlertWindow::NoIcon:
                return juce::Image();
        }
    };
    
    g.drawImage(getIcon(), {-48.0f, -20.0f, 180.0f, 180.0f});
    
    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::textColourId));
    textLayout.draw(g, textArea.withX(84).withTop(12).toFloat());
}

int Application::LookAndFeel::getAlertWindowButtonHeight()
{
    return 36;
}

ANALYSE_FILE_END

