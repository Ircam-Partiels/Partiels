#include "AnlApplicationLookAndFeel.h"
#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Document/AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    auto const backgroundColour = juce::Colours::black;
    auto const rulerColour = juce::Colours::grey;
    auto const textColour = juce::Colours::white;
    auto const thumbColour = juce::Colours::blue;
    
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::ColouredPanel::ColourIds::backgroundColourId, backgroundColour);

    setColour(Layout::PropertySection::ColourIds::separatorColourId, juce::Colours::transparentBlack);
    setColour(Layout::StrechableContainer::Section::resizerActiveColourId, rulerColour.brighter());
    setColour(Layout::StrechableContainer::Section::resizerInactiveColourId, rulerColour.darker());
    
    setColour(Zoom::Ruler::backgroundColourId, rulerColour);
    setColour(Zoom::Ruler::tickColourId, backgroundColour);
    setColour(Zoom::Ruler::textColourId, textColour);
    setColour(Zoom::Ruler::anchorColourId, thumbColour);
    setColour(Zoom::Ruler::selectionColourId, thumbColour);
    
    setColour(Analyzer::Thumbnail::backgroundColourId, rulerColour);
    
    setColour(Document::Section::sectionColourId, backgroundColour);
    setColour(Document::Playhead::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Document::Playhead::playheadColourId, thumbColour);
    
    // juce::ComboBox::LookAndFeelMethods
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::arrowColourId, juce::Colours::white);
    
    setColour(juce::ScrollBar::ColourIds::backgroundColourId, rulerColour);
}

bool Application::LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
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

ANALYSE_FILE_END

